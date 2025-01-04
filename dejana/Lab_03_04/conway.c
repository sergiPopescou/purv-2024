/*
 Laboratorijske vjezbe (3. i 4).
 Zadatak sa conway igrom zivota.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <string.h>

#define VELICINA_TABELE 10


typedef struct Celija 
{
    uint16_t stanje_trenutno; /* ziva = 1, mrtva = 0 */
    uint16_t stanje_naredno; /* stanje celije nakon sto evoluira */
    sem_t celija_semafor;    /* svaka celija ima svoj semafor */
}
CELIJA;

sem_t sinh_semafor;        
int zavrsene_celije = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

CELIJA tabela[VELICINA_TABELE][VELICINA_TABELE];

void inicijalizuj_tabelu ()
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
void* evoluiraj (void* args)
{
    int* koordinate = (int*)args;
    int x = koordinate[0];
    int y = koordinate[1];
    free(args);

    while(1)
    {
        int br_susjeda_x = 0, br_susjeda_y = 0;
        int zivi_susjedi = 0;
        
        for(int i = -1; i <= 1; i++)
        {
            for(int j = -1; j <= 1; j++)
            {
                if(i == 0 && j == 0)
                {
                    /* nastavi dalje, ti si ta celija */
                    continue;
                }

                br_susjeda_x = x + i; /* sada se pomijeraj po x-osi*/
                br_susjeda_y = y + j; /* sada po y-osi*/

                /* ako su susjedi unutar tabele */
                /* susjedi se racunaju samo ako su zivi, tj. 1 im je stanje i racuna se 8 susjeda Murovih 
                [ 1][ 2][ 3]
                [ 4][JA][ 5]
                [ 6][ 7][ 8] */

                if(br_susjeda_x < VELICINA_TABELE && br_susjeda_x >= 0
                   && br_susjeda_y < VELICINA_TABELE && br_susjeda_y >= 0 )
                   {
                        zivi_susjedi += tabela[br_susjeda_x][br_susjeda_y].stanje_trenutno;
                   }
            }
        }

        if(tabela[x][y].stanje_trenutno == 0 && zivi_susjedi == 3)
        {
            tabela[x][y].stanje_naredno = 1;
            //printf("Ozivljavam se. \n");
        }
        else
        {
            if(tabela[x][y].stanje_trenutno)
            {
                if(zivi_susjedi > 3 || zivi_susjedi < 2)
                {
                    tabela[x][y].stanje_naredno = 0;
                    //printf("Umirem. \n");
                }
                else
                {
                    tabela[x][y].stanje_naredno = 1;
                    //printf("Zivim dalje.\n");
                }
            }
        }

        pthread_mutex_lock(&mutex);
        zavrsene_celije++;

        if(zavrsene_celije == VELICINA_TABELE * VELICINA_TABELE)
        {
            sem_post(&sinh_semafor);
        }
        pthread_mutex_unlock(&mutex);                                                                

    }

    return NULL;
  
}

void azuriraj_tabelu()
{
    for (int i = 0; i < VELICINA_TABELE; i++) 
    {
        for (int j = 0; j < VELICINA_TABELE; j++) 
        {
            tabela[i][j].stanje_trenutno = tabela[i][j].stanje_naredno;
            sem_post(&tabela[i][j].celija_semafor);
        }
    }
}

void prikazi_tabelu() {
    for (int i = 0; i < VELICINA_TABELE; i++) 
    {
        for (int j = 0; j < VELICINA_TABELE; j++) 
        {
            if(tabela[i][j].stanje_trenutno)
            {
                printf("[#]");
            }
            else
            {
                printf("[ ]");
            }
            
        }
        printf("\n");
    }
    printf("\n");
}

void kreiraj_niti()
{
     pthread_t niti[VELICINA_TABELE][VELICINA_TABELE];

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

void oslobodi_resurse() 
{
    printf("\n Oslobadjam resurse...\n");
    for (int i = 0; i < VELICINA_TABELE; i++) 
    {
        for (int j = 0; j < VELICINA_TABELE; j++) 
        {
            sem_destroy(&tabela[i][j].celija_semafor);
        }
    }
    sem_destroy(&sinh_semafor);
    pthread_mutex_destroy(&mutex);
}

// signal handler za SIGINT
void signal_handler(int signal) 
{
    if (signal == SIGINT) 
    {
        oslobodi_resurse();
        exit(0);  // zatvara program
    }
}

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
	printf("Nedovoljno argumenata proslijedjeno. ERROR. \n");
	return EXIT_FAILURE;
    }

    int vrijeme_pauze = atoi(argv[1]);
    if(vrijeme_pauze <= 0)
    {
	printf("Vrijeme pauze mora biti >= 0.\n");
	return EXIT_FAILURE;
    }
    srand(time(NULL));
    /*struct sigaction sa;
    sa.sa_handler = signal_handler;  // Postavljamo handler
    sa.sa_flags = 0;                 // Nema dodatnih opcija
    sigemptyset(&sa.sa_mask);        // Ne blokiramo nijedan signal tokom obrade

    // Registracija handlera za SIGINT
    sigaction(SIGINT, &sa, NULL);
    */
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
        sleep(vrijeme_pauze);
	system("clear");
    }

    oslobodi_resurse();
    return 0;
}



