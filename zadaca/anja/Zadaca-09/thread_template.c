// Kompajlirati sa 'gcc thread_template.c -lpthread -lrt -Wall'
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>        // potrebno za mlockall()
#include <unistd.h>          // potrebno za sysconf(int name);
#include <malloc.h>
#include <sys/time.h>        // potrebno za getrusage
#include <sys/resource.h>    // potrebno za getrusage
#include <pthread.h>
#include <limits.h>
#include <sched.h>
   
#define PRE_ALLOCATION_SIZE (10*1024*1024)  /* 100MB pagefault free buffer */
#define MY_STACK_SIZE       (100*1024)      /* 100 kB dodatak za stek */

static pthread_mutex_t mtx;
static pthread_mutexattr_t mtx_attr;
static int shared_val = 0;

typedef struct params{
	int sleepTime, priority, additionalStackSize, sharedValIncrement;
} params;

static void error(int at)
{
   	fprintf(stderr, "Some error occured at %d", at);
   	exit(1);
}

static void setprio(int prio, int sched)
{
   	struct sched_param param;

   	param.sched_priority = prio;
    
   	if (sched_setscheduler(0, sched, &param) < 0)
   		perror("sched_setscheduler");
}

static void *non_res_thread_fn(void *args)
{
    params* localArgs = (params*)args;
    
    setprio(localArgs->priority, SCHED_RR);
    
   	struct timespec ts;
   	ts.tv_sec = localArgs->sleepTime;
   	ts.tv_nsec = 0;

    //<do your RT-thing here>

    prove_thread_stack_use_is_safe(MY_STACK_SIZE + localArgs->additionalStackSize, 1);
    clock_nanosleep(CLOCK_REALTIME, 0, &ts, NULL);
    
    return NULL;
}

static void *resource_thread_fn(void *args)
{
    params* localArgs = (params*)args;
    
    setprio(localArgs->priority, SCHED_RR);
    
    struct timespec ts;
   	ts.tv_sec = localArgs->sleepTime;
	ts.tv_nsec = 0;
    
    
    // Niska prioritetna nit zaključava mutex i drži ga dugo
    if (localArgs->priority == sched_get_priority_min(SCHED_RR))
    {
        pthread_mutex_lock(&mtx);

        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start); // Početno vreme
        while (1)
        {
            clock_gettime(CLOCK_MONOTONIC, &end);
            if ((end.tv_sec - start.tv_sec) >= 5) // Zadržavanje mutex-a 5 sekundi
                break;
        }

        pthread_mutex_unlock(&mtx);
        
    }// Visoka prioritetna nit pokušava da zaključa mutex
    else if (localArgs->priority == sched_get_priority_max(SCHED_RR))
    {
        pthread_mutex_lock(&mtx);

        // Simulira brz rad na resursu
        shared_val += localArgs->sharedValIncrement;

        pthread_mutex_unlock(&mtx);
    }
    else // Srednja prioritetna nit zauzima CPU resurse, ali ne koristi mutex 
    {
        for (volatile int i = 0; i < 1000000000; i++)
            int t = i*(i+1);// Simulira CPU intenzivan posao (zauzima vreme)
    }
    
    return NULL;
}
   
static pthread_t start_rt_thread(void *args, int mod)
{
   	pthread_t thread;
   	pthread_attr_t attr;
   
   	/* inicijalizacija programske niti */
   	if (pthread_attr_init(&attr)) error(1);
    
   	/* Real-time thread se kreira sa specifičnom veličinom steka.
     * Inicijalizacija memorije potrebne za stek */
   	if (pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN + MY_STACK_SIZE)) error(2);
   	
    /* kreiranje programske niti */
    if (mod == 1)
        pthread_create(&thread, &attr, resource_thread_fn, args);
    else
        pthread_create(&thread, &attr, non_res_thread_fn, args);
   	
    return thread;
}

static void reserve_process_memory(int size)
{
   	char* buffer = malloc(size);
   
   	for (int i = 0; i < size; i += sysconf(_SC_PAGESIZE))
   		buffer[i] = 0;

   	free(buffer);
}
   
int main(int argc, char *argv[])
{
   	if (mlockall(MCL_CURRENT | MCL_FUTURE))
   		perror("mlockall failed:");

   	mallopt(M_TRIM_THRESHOLD, -1);
   	mallopt(M_MMAP_MAX, 0);
    
   	reserve_process_memory(PRE_ALLOCATION_SIZE);
   
   //<do your RT-thing>
   
    params HighPriority = {
        .sleepTime = 1,
        .priority = sched_get_priority_max(SCHED_RR),
        .additionalStackSize = 5,
        .sharedValIncrement = 1
    };

    params MediumPriority = {
        .sleepTime = 2,
        .priority = sched_get_priority_max(SCHED_RR)/2,
        .additionalStackSize = 5,
        .sharedValIncrement = 1
    };
    
    params LowPriority = {
        .sleepTime = 0,
        .priority = sched_get_priority_min(SCHED_RR),
        .additionalStackSize = 5,
        .sharedValIncrement = 1
    };

	pthread_mutexattr_init(&mtx_attr);
	pthread_mutexattr_setprotocol(&mtx_attr, PTHREAD_PRIO_NONE);
	pthread_mutex_init(&mtx,&mtx_attr);
    
    pthread_t thread2 = start_rt_thread(&LowPriority, 1);
    pthread_t thread1 = start_rt_thread(&HighPriority, 1);
    pthread_t thread3 = start_rt_thread(&MediumPriority, 1);
    
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    
    pthread_mutexattr_destroy(&mtx_attr);
	pthread_mutex_destroy(&mtx);
   
   	return 0;
}   

/*
 * Niska prioritetna nit:
     Zadržava mutex 5 sekundi simulacijom rada na resursu unutar beskonačne petlje. Ovo uzrokuje direktno blokiranje visoke prioritetne niti koja čeka na mutex.

 * Srednja prioritetna nit:
     Simulira CPU-intenzivan posao (ali ne koristi mutex) koristeći veliki broj iteracija. Zauzima CPU vreme i dodatno produžava čekanje visoke niti.

 * Visoka prioritetna nit:
     Pokušava da zaključa mutex. Međutim, mora čekati dok ga niska prioritetna nit ne oslobodi.
*/
