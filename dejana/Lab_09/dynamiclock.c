   #include <stdlib.h>
   #include <stdio.h>
   #include <sys/mman.h> // potrebno za mlockall()
   #include <unistd.h> // potrebno za sysconf(int name);
   #include <malloc.h>
   #include <sys/time.h> // potrebno za getrusage
   #include <sys/resource.h> // potrebno za getrusage
   

   #define SOMESIZE (10*1024*1024) // 10MB
   

   int main(int argc, char* argv[])
   {
       // omogucavanje alociranja memorije dinamicki
       int i, page_size;
       char* buffer;
       struct rusage usage;
       

       //  zakljucavanje svih trenutnih i buducih stvari u RAM
       if (mlockall(MCL_CURRENT | MCL_FUTURE ))
       {
           perror("mlockall failed:");
       }
       

       // iskljuciti spustanje sbrk
       mallopt (M_TRIM_THRESHOLD, -1);
       

       // iskljuciti koristenje mmap
       mallopt (M_MMAP_MAX, 0);
       

       page_size = sysconf(_SC_PAGESIZE);
       buffer = malloc(SOMESIZE);
       

       getrusage(RUSAGE_SELF, &usage);
       printf("Major-pagefaults:%d, Minor Pagefaults:%d\n", usage.ru_majflt, usage.ru_minflt);
       

       // dotakni sve
       for (i=0; i < SOMESIZE; i+=page_size)
       {
           buffer[i] = 0;
           getrusage(RUSAGE_SELF, &usage);
           printf("Major-pagefaults:%d, Minor Pagefaults:%d\n", usage.ru_majflt, usage.ru_minflt);
       }
       free(buffer);
       // cak i kada smo buffer poslali u vjecna lovista, sbrk se nece
       // spustiti i predati memoriju, i sva buduca, dinamicka alociranja
       // sa malloc i slicinm komandama ce koristiti zauzeti pool memorije
       // znaci nema ni minor pagefoulta!
       

       //<do your RT-thing>
       

       return 0;
   }
