#include <stdio.h>
#include <sys/mman.h>        // potrebno za mlockall()
#include <unistd.h>          // potrebno za sysconf(int name);
#include <malloc.h>
#include <sys/time.h>        // potrebno za getrusage
#include <sys/resource.h>    // potrebno za getrusage
#define SOMESIZE (100*1024)  // 100kB = 102 400B
   
int main(int argc, char* argv[])
{
       struct rusage usage;
       
       /* Zakljucava sve trenutno mapirane stranice ali i sve buduce alokacije
        * memorije u RAM tako da ne mogu biti izbacene na disk */
       if (mlockall(MCL_CURRENT | MCL_FUTURE )) { perror("mlockall failed:"); }
       
       int page_size = sysconf(_SC_PAGESIZE);   // Dohvata veličinu memorijske stranice na trenutnom sistemu
       
       
       /* Alocira memoriju od 100 kB - 102 400B u virtuelnom adresnom prostoru procesa.
        * Po alokaciji, stranice koje čine taj blok memorije nisu fizički mapirane u 
        * RAM dok se ne "dodirnu" (ili upotrebe).
        */
       char* buffer  = malloc(SOMESIZE);

       // "Touch" dodjeljivanjem vrijednosti promjenjivoj, mapiramo je u RAM
       for (int i = 0; i < SOMESIZE; i+=page_size)
       {
           // Svaki upis generise pagefault. OS mapira tu stranicu iz virtuelnog u fizički RAM
           // Jednom kada je pagefault odradjen, promjenjiva ostaje 
           // zakljucana u memoriji.
           buffer[i] = 0;
           
           // uzeti informaciju o resursima i ispisati major i minor pagefault-e
           getrusage(RUSAGE_SELF, &usage);
           printf("Major-pagefaults:%d, Minor Pagefaults:%d\n", usage.ru_majflt, usage.ru_minflt);
       }
       // posto ostajemo u main programu, buffer ostaje ziv, a zbog 
       // "touch"-a i instrukcije o zakljucavanju mlockall() ne izlazi
       // iz RAM-a sto dovodi do toga da nema vise major pagefault-ova
       

       //<do your RT-thing>
       

       return 0;
}
