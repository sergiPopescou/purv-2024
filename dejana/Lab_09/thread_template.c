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
   #include <time.h>
   
   #define PRE_ALLOCATION_SIZE (10*1024*1024) /* 100MB pagefault free buffer */ /* vel. mem. bafera*/
   #define MY_STACK_SIZE       (100*1024)      /* 100 kB dodatak za stek za niti*/
   
   #define MIN_PRIORITY 8
   #define MAX_PRIORITY 1
   #define FUNC(x,y) (x*y)

   static pthread_mutexattr_t mtx_attr;

   static pthread_mutex_t mtx;
   static int shared_val = 0;
   FILE* log_file = NULL;

   void print_timestamp(const char* message, int priority) 
   {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    fprintf(log_file, "[%ld.%09ld] [Prioritet: %d] %s\n", ts.tv_sec, ts.tv_nsec, priority, message);
   }

   typedef struct thread_param_shared
   {
      int sleep_secs;
      int priority;
      int add_stack_space;
      int do_log;
      int shared_val_inc;
   } thread_param_shared;

   typedef struct thread_param_non_shared
   {
      int sleep_secs;
      int priority;
      int add_stack_space;
      int do_log;
   } thread_param_non_shared;

   /* prio je prioritet, a sched je scheduler koji treba koristiti
      npr. SCHED_RR za round-robin*/

   static void setprio(int prio, int sched)
   {
   	struct sched_param param;
   	// podesavanje prioriteta i schedulera
   	// vise o tome kasnije
   	param.sched_priority = prio;
   	if (sched_setscheduler(0, sched, &param) < 0)
   		perror("sched_setscheduler");
   }
   
   /* logtext - opisuje situaciju koja se desava
      allowed_maj - max broj dozvoljenih major pagefault-ova
      allowed_min - maksimalan broj dozvoljenih minor pagefault-ova */
   void show_new_pagefault_count(const char* logtext, 
   			      const char* allowed_maj,
   			      const char* allowed_min)
   {
	   // ispis pagefaultova!
   	static int last_majflt = 0, last_minflt = 0;
   	struct rusage usage; 
   
   	getrusage(RUSAGE_SELF, &usage); /* dobija info o koriscenju resursa */
   
   	printf("%-30.30s: Pagefaults, Major:%ld (Allowed %s), " \
   	       "Minor:%ld (Allowed %s)\n", logtext,
   	       usage.ru_majflt - last_majflt, allowed_maj,
   	       usage.ru_minflt - last_minflt, allowed_min);
      /* izracunava razliku izmedju trenutnih i prethodnih vrijednosti
      pagefault-ova (major i minor) */
   	
      /* azuriranje internih brojaca*/
   	last_majflt = usage.ru_majflt; 
   	last_minflt = usage.ru_minflt;
   }
   
   /* osigurava da je stek za nit u ram-u i da nem generise pagefault 
      stacksize je velicina steka, do_log - da li ce ispisivati 
      log tj. informacije o pagefault-ovima */
   static void prove_thread_stack_use_is_safe(int stacksize, int do_log)
   {
	   //gurni stek u RAM
   	volatile char buffer[stacksize];
   	int i;
   
   	for (i = 0; i < stacksize; i += sysconf(_SC_PAGESIZE)) {
   		/* "Touch" za cijeli stek od programske niti */
   		buffer[i] = i; /* ovo radi da osigura da je sve u ram ucitano */
   	}
	 if(do_log)
		show_new_pagefault_count("Caused by using thread stack", "0", "0");
   }

   /*************************************************************/
   static void* resource_thread_fn(void* args)
   {
      thread_param_shared params = *(thread_param_shared*)args;
      struct timespec ts;
      ts.tv_sec = 20;
      ts.tv_nsec = 0;
   
      setprio(params.priority, SCHED_RR); /* prioritet i round robin*/
   
      if(params.do_log)
      {
         printf("I am an RT- (resource_thread_fn) with a stack that does not generate " \
               "page-faults during use, stacksize=%i\n", MY_STACK_SIZE);
      }
   
      //<do your RT-thing here>

      print_timestamp("Zapocinje izvrsavanje niti.", params.priority);
      if(params.priority == MAX_PRIORITY)
      {
         clock_nanosleep(CLOCK_REALTIME, 0, &ts, NULL);
         printf("Nit sa MAX_PRIORITY zavrsava spavanje. \n");
      }
      ts.tv_sec = params.sleep_secs;

      pthread_mutex_lock(&mtx);
      print_timestamp("Zakljucan mutex.", params.priority);

      // Pristupanje zajednickom resursu.
      shared_val += params.shared_val_inc;
      printf("[Prioritet: %d] Pristup shared_val: Nova vrijednost = %d\n", params.priority, shared_val);

      for (int i = 0; i < 100000000; i++) 
      {
         int result = FUNC(i, i + 1); 
      }

      print_timestamp("Otkljucan mutex.", params.priority);
      pthread_mutex_unlock(&mtx);

      print_timestamp("Zavrseno izvrsavanje niti.", params.priority);
      
      show_new_pagefault_count("Caused by creating thread", ">=0", ">=0");
      
      prove_thread_stack_use_is_safe(MY_STACK_SIZE, 1);
   
      printf("Sada spava %d sekundi. \n", params.sleep_secs);
      clock_nanosleep(CLOCK_REALTIME, 0, &ts, NULL);
      printf("Nit je odspavala svoje vrijeme. \n");
      return NULL;
   }
   
   /*************************************************************/
   
   static void* non_resource_thread_fn(void *args)
   {
      thread_param_non_shared params = *(thread_param_non_shared*)args;

      struct timespec ts;
      ts.tv_sec = 20;
      ts.tv_nsec = 0;

      setprio(params.priority, SCHED_RR);
      ts.tv_sec = params.sleep_secs;

      print_timestamp("Zapocinje izvrsavanje niti. Nit je non-shared.", params.priority);
   
      if(params.do_log)
         printf("I am an RT-thread (non-shared) with a stack that does not generate page-faults during use, stacksize=%i\n", params.add_stack_space);
   
      printf("[Prioritet: %d] Pristup shared_val - ne pristupam: %d\n", params.priority, shared_val);

      printf("Sada spava %d sekundi. \n", params.sleep_secs);
      clock_nanosleep(CLOCK_REALTIME, 0, &ts, NULL);
      printf("Nit je odspavala svoje vrijeme. \n");

      return NULL;


   }

   /*************************************************************/
   static void error(int at)
   {
   	/* Ispisi gresku i izadji */
   	fprintf(stderr, "Some error occured at %d", at); /* at je broj koraka ili mjesto u kodu 
                                                         gdje je doslo do greske */
   	exit(1);
   }
   
   /* kreira novu nit sa podesenim atributima */
   static pthread_t start_rt_thread(void* args, int is_shared)
   {
   	pthread_t thread;
   	pthread_attr_t attr;
   
   	/* inicijalizacija programske niti */
   	if (pthread_attr_init(&attr))
   		error(1);
   	/* inicijalizacija memorije potrebne za stek */
   	if (pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN + MY_STACK_SIZE))
   		error(2);
   	/* kreiranje programske niti */
      if(is_shared)
      {
   	  pthread_create(&thread, &attr, resource_thread_fn, (void*)args);
      }
      else
      {
         pthread_create(&thread, &attr, &non_resource_thread_fn, (void*)args);
      }
   	return thread;
   }
   
   /* kako ce se ponasati alokacija memorije za proces */
   static void configure_malloc_behavior(void)
   {
   	/* konfiguracija allociranja memorije */
   	if (mlockall(MCL_CURRENT | MCL_FUTURE)) /* zakljucava memoriju procesa */
   		perror("mlockall failed:");
   
   	/* sbrk nema nazad */
   	mallopt(M_TRIM_THRESHOLD, -1); /* onemogucava oslobadjanje memorije */
   
   	/* iskljuci mmap */
   	mallopt(M_MMAP_MAX, 0); /* iskljucuje mapiranje memorije pomocu nmap */
   }
   
   /* rezervise memoriju za proces i omogucava da se ucita memorija u ram */
   /* size je broj bajtova za memorisanje  */
   static void reserve_process_memory(int size)
   {
	   // rezervisanje memorije, guranje svega u RAM
   	int i;
   	char *buffer;
   
   	buffer = malloc(size);
   
   	for (i = 0; i < size; i += sysconf(_SC_PAGESIZE)) {
   		buffer[i] = 0;
   	}
   	free(buffer); 
   }
   
   int main(int argc, char *argv[])
   {
      log_file = fopen("output.log", "w");  // Otvorite fajl za pisanje

       if (!log_file) 
       {
           perror("Error opening log file");
           return -1;
       }

      pthread_mutexattr_init(&mtx_attr);
      pthread_mutexattr_setprotocol(&mtx_attr, PTHREAD_PRIO_PROTECT);
      pthread_mutex_init(&mtx, &mtx_attr);

   	show_new_pagefault_count("Initial count", ">=0", ">=0");
   
   	configure_malloc_behavior();
   
   	show_new_pagefault_count("mlockall() generated", ">=0", ">=0");
   
   	reserve_process_memory(PRE_ALLOCATION_SIZE);
   
   	show_new_pagefault_count("malloc() and touch generated", 
   				 ">=0", ">=0");
   
   	/* ponovna alokacija memorije nece izazvati nikakve pagefaultove */
   	reserve_process_memory(PRE_ALLOCATION_SIZE);
   	show_new_pagefault_count("2nd malloc() and use generated", 
   				 "0", "0");
   
   	printf("\n\nLook at the output of ps -leyf, and see that the " \
   	       "RSS is now about %d [MB]\n",
   	       PRE_ALLOCATION_SIZE / (1024 * 1024));
   
      thread_param_shared thp1 = 
      {
         .sleep_secs = 5,
         .priority = 8,
         .add_stack_space = MY_STACK_SIZE,
         .do_log = 1,
         .shared_val_inc = 3
      };
      thread_param_shared thp2 = 
      {
         .sleep_secs = 2,
         .priority = 1,
         .add_stack_space = MY_STACK_SIZE,
         .do_log = 1,
         .shared_val_inc = 6
      };

      thread_param_shared thp3 = 
      {
         .sleep_secs = 4,
         .priority = 5,  // Srednji prioritet
         .add_stack_space = MY_STACK_SIZE,
         .do_log = 1,
         .shared_val_inc = 2
      };

      printf("Pokrece se nit sa prioritetom %d.\n", thp1.priority);
      pthread_t th1 = start_rt_thread(&thp1,1);

      printf("Pokrece se nit sa prioritetom %d.\n", thp2.priority);
      pthread_t th2  = start_rt_thread(&thp2,1);

      pthread_t th3 = start_rt_thread(&thp3,1);

      printf("Pokrece se nit sa prioritetom %d.\n", thp3.priority);
     

      thread_param_non_shared thp4 =
      {
         .sleep_secs = 3,
         .priority = 2,
         .add_stack_space = MY_STACK_SIZE,
         .do_log = 0
      };
      printf("Pokrece se nit sa prioritetom %d.\n", thp4.priority);
      pthread_t th4 = start_rt_thread(&thp4,1);
   
   //<do your RT-thing>
   
      pthread_join(th1, NULL);
      pthread_join(th2, NULL);
      pthread_join(th3, NULL);
      pthread_join(th4, NULL);
      printf("Kraj izvrsavanja! \n");
      printf("Press <ENTER> to exit\n");
   	getchar();
      
      pthread_mutexattr_destroy(&mtx_attr);
      pthread_mutex_destroy(&mtx);

   	return 0;
   }
   
   /*
      minor pgf = stranica je ucitana u memoriju, ali nije mapirana u adresni prostor
      procesa (samo se treba azurirati page table)

      major pgf = stranica nije ucitana u ram, duze traje dosta

      sbrk - povecavanje ili smanjivanje heap-a i svaka stranica kojoj e prvi put 
      pristupi, ona ce izazvati minor pagefault
   */
