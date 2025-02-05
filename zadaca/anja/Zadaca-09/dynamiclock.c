/* Dinamicko alociranje memorije pri zakljucanoj memoriji
 * Da bismo mogli dinamicki alocirati memoriju, moramo 
 * kontrolisati sistemske pozive sbrk() i mmap().
 *
 * Posto se mmap() ne moze kontrolisati bilo bi dobro bar onemoguciti ga
 * Poziv sbrk() podize i spusta svoju adresu po memoriji. Bilo bi dobro
 * da bar ne moze da spusta.
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>       // potrebno za mlockall()
#include <unistd.h>         // potrebno za sysconf(int name);
#include <malloc.h>
#include <sys/time.h>       // potrebno za getrusage
#include <sys/resource.h>   // potrebno za getrusage
   
#define SOMESIZE (10*1024*1024) // 10MB

int main(int argc, char* argv[])
{
       struct rusage usage;
       
       /* Zakljucava sve trenutno mapirane stranice ali i sve buduce alokacije
        * memorije u RAM tako da ne mogu biti izbacene na disk */
       if (mlockall(MCL_CURRENT | MCL_FUTURE ))
       {
           perror("mlockall failed:");
       }

       mallopt(M_TRIM_THRESHOLD, -1);   // iskljuciti spustanje sbrk
       mallopt(M_MMAP_MAX, 0);          // iskljuciti koristenje mmap
       

       int page_size = sysconf(_SC_PAGESIZE);
        /* Alocira memoriju od 10MB u virtuelnom adresnom prostoru procesa.
         * Po alokaciji, stranice koje čine taj blok memorije nisu fizički mapirane u 
         * RAM dok se ne "dodirnu" (ili upotrebe).
         */
       char* buffer  = malloc(SOMESIZE);
       

       getrusage(RUSAGE_SELF, &usage);
       printf("Major-pagefaults:%d, Minor Pagefaults:%d\n", usage.ru_majflt, usage.ru_minflt);
       

       // Touch all
       for (int i = 0; i < SOMESIZE; i+=page_size)
       {
           buffer[i] = 0; // svaki upis generise pagefault te OS mapira tu stranicu iz virtuelnog u fizicki RAM
           
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
