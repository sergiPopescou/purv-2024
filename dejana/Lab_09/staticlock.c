   #include <stdio.h>
   #include <sys/mman.h> // potrebno za mlockall()
   #include <unistd.h> // potrebno za sysconf(int name);
   #include <malloc.h>
   #include <sys/time.h> // potrebno za getrusage
   #include <sys/resource.h> // potrebno za getrusage
   

   #define SOMESIZE (100*1024) // 100kB
   

   int main(int argc, char* argv[])
   {
       // alociranje memorije 
       int i, page_size;
       char* buffer;
       struct rusage usage;
       

       // zakljucavanje svih trenutnih i buducih stvari u RAM
       if (mlockall(MCL_CURRENT | MCL_FUTURE ))
       {
           perror("mlockall failed:");
       }
       

       page_size = sysconf(_SC_PAGESIZE);// izvatiti stvarnu velicinu page-a
       buffer = malloc(SOMESIZE);
       

       // "Touch" dodjeljivanjem vrijednosti promjenjivoj, mapiramo je u RAM
       for (i=0; i < SOMESIZE; i+=page_size)
       {
           // Svaki upis generise pagefault.
           // Jednom kada je pagefault odradjen, promjenjiva ostaje 
           // zakljucana u memoriji.
           buffer[i] = 0;
           // uzeti informaciju o resursima i ispisati major i minor
           // pagefault-e
           getrusage(RUSAGE_SELF, &usage);
           printf("Major-pagefaults:%d, Minor Pagefaults:%d\n", usage.ru_majflt, usage.ru_minflt);
       }
       // posto ostajemo u main programu, buffer ostaje ziv, a zbog 
       // "touch"-a i instrukcije o zakljucavanju mlockall() ne izlazi
       // iz RAM-a sto dovodi do toga da nema vise major pagefault-ova
       

       //<do your RT-thing>
       

       return 0;
   }
