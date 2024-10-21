// Kompajlirati sa 'gcc thread_template.c -lpthread -lrt -Wall'
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h> // potrebno za mlockall()
#include <unistd.h> // potrebno za sysconf(int name);
#include <malloc.h>
#include <sys/time.h> // potrebno za getrusage
#include <sys/resource.h> // potrebno za getrusage
#include <pthread.h>
#include <limits.h>
   
#define PRE_ALLOCATION_SIZE (10*1024*1024) /* 100MB pagefault free buffer */
#define MY_STACK_SIZE       (100*1024)      /* 100 kB dodatak za stek */
static pthread_mutex_t mtx;
static pthread_mutexattr_t mtx_attr;
   
static void setprio(int prio, int sched) {
	struct sched_param param;
   	// podesavanje prioriteta i schedulera
   	// vise o tome kasnije
   	param.sched_priority = prio;
   	if (sched_setscheduler(0, sched, &param) < 0)
   		perror("sched_setscheduler");
}
   
void show_new_pagefault_count(const char* logtext, 
		const char* allowed_maj,
		const char* allowed_min) {
	// ispis pagefaultova!
   	static int last_majflt = 0, last_minflt = 0;
   	struct rusage usage;
   
   	getrusage(RUSAGE_SELF, &usage);
   
   	printf("%-30.30s: Pagefaults, Major:%ld (Allowed %s), " \
   	       "Minor:%ld (Allowed %s)\n", logtext,
   	       usage.ru_majflt - last_majflt, allowed_maj,
   	       usage.ru_minflt - last_minflt, allowed_min);
   	
   	last_majflt = usage.ru_majflt; 
   	last_minflt = usage.ru_minflt;
}
   
static void prove_thread_stack_use_is_safe(int stacksize, int do_log) {
	//gurni stek u RAM
	volatile char buffer[stacksize];
   	int i;
   
   	for (i = 0; i < stacksize; i += sysconf(_SC_PAGESIZE)) {
   		/* "Touch" za cijeli stek od programske niti */
   		buffer[i] = i;
   	}
	if(do_log)
		show_new_pagefault_count("Caused by using thread stack", "0", "0");
}
   
/*************************************************************/
/* Funkcija programske niti koja koristi dijeljeni resurs */
static void *resource_thread_fn(void *args) {
   	struct timespec ts;
   	ts.tv_sec = 30;
   	ts.tv_nsec = 0;
   
   	setprio(sched_get_priority_max(SCHED_RR), SCHED_RR);
   
   	/* printf("I am an RT-thread with a stack that does not generate " \ */
   	/*        "page-faults during use, stacksize=%i\n", MY_STACK_SIZE); */
   
    //<do your RT-thing here>
  	printf("resource thread\n"); 
   	/* show_new_pagefault_count("Caused by creating thread", ">=0", ">=0"); */
   
   	prove_thread_stack_use_is_safe(MY_STACK_SIZE, 1);
   
   	/* spavati 30 sekundi */
   	clock_nanosleep(CLOCK_REALTIME, 0, &ts, NULL);
    //printf("shared value%i\n", shared_val);
   	return NULL;
}
/*************************************************************/
   
/*************************************************************/
/* Funkcija programske niti koja ne koristi dijeljeni resurs */
static void *non_res_thread_fn(void *args) {
   	struct timespec ts;
   	ts.tv_sec = 30;
   	ts.tv_nsec = 0;
   
   	setprio(sched_get_priority_max(SCHED_RR), SCHED_RR);
   
   	/* printf("I am an RT-thread with a stack that does not generate " \ */
   	/*        "page-faults during use, stacksize=%i\n", MY_STACK_SIZE); */
   
    //<do your RT-thing here>
  	printf("non resource thread\n"); 
   	/* show_new_pagefault_count("Caused by creating thread", ">=0", ">=0"); */
   
   	prove_thread_stack_use_is_safe(MY_STACK_SIZE, 1);
   
   	/* spavati 30 sekundi */
   	clock_nanosleep(CLOCK_REALTIME, 0, &ts, NULL);
    //printf("shared value%i\n", shared_val);
   	return NULL;
}
/*************************************************************/
   
static void error(int at) {
   	/* Ispisi gresku i izadji */
   	fprintf(stderr, "Some error occured at %d", at);
	exit(1);
}
   
static pthread_t start_rt_thread(void*(*rt_thread)(void*)) {
   	pthread_t thread;
   	pthread_attr_t attr;
   
   	/* inicijalizacija programske niti */
   	if (pthread_attr_init(&attr))
   		error(1);
   	/* inicijalizacija memorije potrebne za stek */
   	if (pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN + MY_STACK_SIZE))
   		error(2);
   	/* kreiranje programske niti */
   	pthread_create(&thread, &attr, rt_thread, NULL);
   	return thread;
}
   
static void configure_malloc_behavior(void) {
   	/* konfiguracija allociranja memorije */
   	if (mlockall(MCL_CURRENT | MCL_FUTURE))
   		perror("mlockall failed:");
   
   	/* sbrk nema nazad */
   	mallopt(M_TRIM_THRESHOLD, -1);
   
   	/* iskljuci mmap */
   	mallopt(M_MMAP_MAX, 0);
}
   
static void reserve_process_memory(int size) {
	// rezervisanje memorije, guranje svega u RAM
   	int i;
   	char *buffer;
   
   	buffer = malloc(size);
   
   	for (i = 0; i < size; i += sysconf(_SC_PAGESIZE)) {
   		buffer[i] = 0;
   	}
   	free(buffer);
}
   
int main(int argc, char *argv[]) {
   	/* show_new_pagefault_count("Initial count", ">=0", ">=0"); */
   
   	configure_malloc_behavior();
   
   	/* show_new_pagefault_count("mlockall() generated", ">=0", ">=0"); */
   
   	reserve_process_memory(PRE_ALLOCATION_SIZE);
   
   	/* show_new_pagefault_count("malloc() and touch generated", */ 
   	/* 			 ">=0", ">=0"); */
   
   	/* ponovna alokacija memorije nece izazvati nikakve pagefaultove */
   	/* reserve_process_memory(PRE_ALLOCATION_SIZE); */
   	/* show_new_pagefault_count("2nd malloc() and use generated", */ 
   	/* 			 "0", "0"); */
   
   	/* printf("\n\nLook at the output of ps -leyf, and see that the " \ */
   	/*        "RSS is now about %d [MB]\n", */
   	/*        PRE_ALLOCATION_SIZE / (1024 * 1024)); */
	pthread_mutexattr_init(&mtx_attr);
	pthread_mutexattr_setprotocol(&mtx_attr, PTHREAD_PRIO_PROTECT);
	pthread_mutex_init(&mtx, &mtx_attr);
   
   	start_rt_thread(resource_thread_fn);
   	start_rt_thread(resource_thread_fn);
   	start_rt_thread(non_res_thread_fn);
   
    //<do your RT-thing>
   
   	printf("Press <ENTER> to exit\n");
   	getchar();
   
	pthread_mutexattr_destroy(&mtx_attr);
	pthread_mutex_destroy(&mtx);
   	return 0;
}
