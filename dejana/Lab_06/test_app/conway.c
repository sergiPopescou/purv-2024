#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/ioctl.h>


#define VELICINA_TABELE 10

#define WR_VALUE _IOW('a','a',PODACI_ZA_KERNEL*)
#define RD_VALUE _IOR('a','b',PODACI_ZA_KERNEL*)



typedef struct Celija 
{
    uint16_t stanje_trenutno; /* ziva = 1, mrtva = 0 */
    uint16_t stanje_naredno; /* stanje celije nakon sto evoluira */
    sem_t celija_semafor;    /* svaka celija ima svoj semafor */
} CELIJA;

sem_t sinh_semafor;        
int zavrsene_celije = 0;
bool zavrsi = false;  // Signal za završetak niti

pthread_t niti[VELICINA_TABELE][VELICINA_TABELE];

typedef struct podaci_za_kernel
{
    int8_t stanja_celija_za_kernel[10][10];  
    int32_t broj_zivih_celija;
    int32_t broj_umrlih_celija;
    int32_t broj_ozivljenih_celija;
    int32_t broj_generacija_celija;
} PODACI_ZA_KERNEL;

PODACI_ZA_KERNEL pzk;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexPZK = PTHREAD_MUTEX_INITIALIZER;

CELIJA tabela[VELICINA_TABELE][VELICINA_TABELE];

void inicijalizuj_tabelu()
{
    for (int i = 0; i < VELICINA_TABELE; i++) 
    {
        for (int j = 0; j < VELICINA_TABELE; j++) 
        {
            tabela[i][j].stanje_trenutno = rand() % 2;  /* nasumicno postavlja stanja celija */
            tabela[i][j].stanje_naredno = 0;   
            sem_init(&tabela[i][j].celija_semafor, 0, 1);    
        }
    }
}

/* funkcija za prosljedivanje niti */
void* evoluiraj(void* args)
{
    int* koordinate = (int*)args;
    int x = koordinate[0];
    int y = koordinate[1];
    free(args);

    while (!zavrsi)
    {
        int br_susjeda_x = 0, br_susjeda_y = 0;
        int zivi_susjedi = 0;

        for (int i = -1; i <= 1; i++)
        {
            for (int j = -1; j <= 1; j++)
            {
                if (i == 0 && j == 0)
                {
                    continue;
                }

                br_susjeda_x = x + i; 
                br_susjeda_y = y + j; 

                if (br_susjeda_x < VELICINA_TABELE && br_susjeda_x >= 0
                    && br_susjeda_y < VELICINA_TABELE && br_susjeda_y >= 0)
                {
                    zivi_susjedi += tabela[br_susjeda_x][br_susjeda_y].stanje_trenutno;
                }
            }
        }

        if (tabela[x][y].stanje_trenutno == 0 && zivi_susjedi == 3)
        {
            tabela[x][y].stanje_naredno = 1;
            pthread_mutex_lock(&mutexPZK);
            pzk.broj_ozivljenih_celija += 1;
            pthread_mutex_unlock(&mutexPZK);
        }
        else if (tabela[x][y].stanje_trenutno)
        {
            if (zivi_susjedi > 3 || zivi_susjedi < 2)
            {
                tabela[x][y].stanje_naredno = 0;
                pthread_mutex_lock(&mutexPZK);
                pzk.broj_umrlih_celija += 1;
                pthread_mutex_unlock(&mutexPZK);
            }
            else
            {
                tabela[x][y].stanje_naredno = 1;
            }
        }

        pthread_mutex_lock(&mutex);
        zavrsene_celije++;
        if (zavrsene_celije == VELICINA_TABELE * VELICINA_TABELE)
        {
            sem_post(&sinh_semafor);
        }
        pthread_mutex_unlock(&mutex);                                                                

        sem_wait(&tabela[x][y].celija_semafor);
    }

    return NULL;
}

void azuriraj_tabelu()
{
    pthread_mutex_lock(&mutexPZK);
    pzk.broj_zivih_celija = 0;
    pzk.broj_umrlih_celija = 0;
    pzk.broj_ozivljenih_celija = 0;
    pthread_mutex_unlock(&mutexPZK);

    for (int i = 0; i < VELICINA_TABELE; i++) 
    {
        for (int j = 0; j < VELICINA_TABELE; j++) 
        {
            pthread_mutex_lock(&mutex);
            tabela[i][j].stanje_trenutno = tabela[i][j].stanje_naredno; /* */
            pzk.stanja_celija_za_kernel[i][j] = tabela[i][j].stanje_trenutno;

            if (tabela[i][j].stanje_trenutno == 1)
            {
                pzk.broj_zivih_celija += 1;
            }

            pthread_mutex_unlock(&mutex);

            sem_post(&tabela[i][j].celija_semafor);
        }
    }
}

void prikazi_tabelu()
{
    for (int i = 0; i < VELICINA_TABELE; i++) 
    {
        for (int j = 0; j < VELICINA_TABELE; j++) 
        {
            printf(tabela[i][j].stanje_trenutno ? "[#]" : "[ ]");
        }
        printf("\n");
    }
    printf("\n");
}

void kreiraj_niti()
{
    for (int i = 0; i < VELICINA_TABELE; i++) 
    {
        for (int j = 0; j < VELICINA_TABELE; j++) 
        {
            int* args = (int*)malloc(2 * sizeof(int));  /* koordinate */
            args[0] = i;
            args[1] = j;
            pthread_create(&niti[i][j], NULL, evoluiraj, args);
        }
    }
}

void zavrsi_niti()
{
    zavrsi = true;
    for (int i = 0; i < VELICINA_TABELE; i++) 
    {
        for (int j = 0; j < VELICINA_TABELE; j++) 
        {
            sem_post(&tabela[i][j].celija_semafor);
            pthread_join(niti[i][j], NULL);
        }
    }
}

void oslobodi_resurse() 
{
    printf("\nOslobadjam resurse...\n");
    for (int i = 0; i < VELICINA_TABELE; i++) 
    {
        for (int j = 0; j < VELICINA_TABELE; j++) 
        {
            sem_destroy(&tabela[i][j].celija_semafor);
        }
    }
    sem_destroy(&sinh_semafor);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&mutexPZK);
}

void signal_handler(int signal) 
{
    if (signal == SIGINT) 
    {
        zavrsi_niti();
        oslobodi_resurse();
        exit(0);
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("Nedovoljno argumenata proslijedjeno. ERROR. \n");
        return EXIT_FAILURE;
    }

    int vrijeme_pauze = atoi(argv[1]);
    if (vrijeme_pauze <= 0)
    {
        printf("Vrijeme pauze mora biti >= 0.\n");
        return EXIT_FAILURE;
    }

    int fd;
    printf("Opening device...\n");
    fd = open("/dev/ioctl_driver", O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }
    printf("Device opened successfully.\n");

    srand(time(NULL));
    signal(SIGINT, signal_handler);

    sem_init(&sinh_semafor, 0, 0);
    inicijalizuj_tabelu();
    kreiraj_niti();

    while (1) 
    {
        sem_wait(&sinh_semafor);

        pthread_mutex_lock(&mutex);
        zavrsene_celije = 0;
        pthread_mutex_unlock(&mutex);

        prikazi_tabelu();
        azuriraj_tabelu();

        pthread_mutex_lock(&mutexPZK);
        pzk.broj_generacija_celija++;
        pthread_mutex_unlock(&mutexPZK);

        pthread_mutex_lock(&mutex);
        printf("Writing cell states...\n");
        if (ioctl(fd, WR_VALUE, &pzk) < 0) 
        {
            perror("Failed to write cell states");
            close(fd);
            return -1;
        }
        printf("Cell states written successfully. I sleep now.\n");
        pthread_mutex_unlock(&mutex);

        sleep(vrijeme_pauze);

        pthread_mutex_lock(&mutex);
        printf("Reading cell states...\n");
        if (ioctl(fd, RD_VALUE, &pzk) < 0) 
        {
            perror("Failed to read central cell state");
            close(fd);
            return -1;
        }

        printf("Cell states read successfully.\n");
        printf("Statistika: zive %d umrle %d ozivljene %d generacija %d \n \n", pzk.broj_zivih_celija, pzk.broj_umrlih_celija, 
                pzk.broj_ozivljenih_celija, pzk.broj_generacija_celija);
        pthread_mutex_unlock(&mutex);
        sleep(1);
        system("clear");

    }

    zavrsi_niti();
    oslobodi_resurse();
    return 0;
}
