/* periodicni posix */
/* zadatak: implementirati app sa 2 periodicna taska iz periodic_tasks.c
   sa vremenima 60 ms i 80 ms
   periodizacija preko periodicnog posiksa */

//arm-linux-gnueabihf-gcc periodic_task_1.c -std=gnu99 -Wall -o periodic_task_1
//gcc -Wall periodic_task_posix_timer.c -lrt -o naziv_izlaznog_fajla
// -lrt linkuje librt biblioteku u kojoj se nalaze timer_create i timer_settime
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>


static sigset_t sigset_t1; /* skup signala, cuva koji se signal ceka 
						   unutar komentara */
static sigset_t sigset_t2;
FILE *log_file;

/* 
	fja koja ceka sljedecu aktivaciju zadatka 
   	dummy - tu ce biti smjesten signal
   	sigwait - ceka da se desi signal i onda vraca njegov broj 
*/
static void wait_next_activation(uint8_t task_num) 
{
    int dummy; 
    if(task_num == 1)
    {
    	sigwait(&sigset_t1, &dummy);
    }
    else
    {
    	sigwait(&sigset_t2, &dummy);
    }
    
}

/*
	fja pokrece periodicni tajmer
	offs - offset je prvobitno kasnjenje, prije aktivacije
	period - period ponavljanja tajmera
	sve je u us
*/
int start_periodic_timer(uint64_t offs, int period, uint8_t task_num)
{
    struct itimerspec t; /* vrijeme prvog pok. i perioda*/
    struct sigevent sigev;
    timer_t timer;
    const int signal = SIGALRM;
    int res;

    /* kad se prvi put aktivira tajmer */
    t.it_value.tv_sec = offs / 1000000;  
    t.it_value.tv_nsec = (offs % 1000000) * 1000; 

    /* periodicni interval izmedju aktivacija */
    t.it_interval.tv_sec = period / 1000000;
    t.it_interval.tv_nsec = (period % 1000000) * 1000;
	
	// ovaj dio ostaje isti

	if(task_num == 1)
	{
		/* inic prazan skup */
	    sigemptyset(&sigset_t1); 
		/* SIGALARM dodaje */
	    sigaddset(&sigset_t1, signal); 
	    /* blokira SIGALARM tako da se on tek u sigwait obradi */
	    sigprocmask(SIG_BLOCK, &sigset_t1, NULL); 
	}
	else 
	{
		sigemptyset(&sigset_t2);
		sigaddset(&sigset_t2, signal);
		sigprocmask(SIG_BLOCK, &sigset_t2, NULL);
	}

	/* alociranje i popunjavanje sigevent strukture */
    memset(&sigev, 0, sizeof(struct sigevent));

    /* kada vrijeme istekne okinuce se signal,
    jer se ostatak koda oslanja na signale */
    sigev.sigev_notify = SIGEV_SIGNAL; 
    sigev.sigev_signo = signal;	/* koji signal? pa SIGALRM, naravno */
	
	/* kreiranje tajmera */
    res = timer_create(CLOCK_MONOTONIC, &sigev, &timer);
    if (res < 0) 
    {
        perror("Timer Create");

	exit(-1);
    }
	/* pokretanje tajmera */
    return timer_settime(timer, 0 /*TIMER_ABSTIME*/, &t, NULL);
}

/* fja koja se izvrsava periodicno kada tajmer generise signal */
/* iz fajla periodic_tasks.c */
void task1(void)
{
  int i,j;
 
  for (i=0; i<4; i++) {
    for (j=0; j<1000; j++) ;
    printf("1");
    fflush(stdout);
  }
}

void task2(void)
{
  int i,j;

  for (i=0; i<6; i++) {
    for (j=0; j<10000; j++) ;
    printf("2");
    fflush(stdout);
  }
}


int main(int argc, char *argv[])
{
    int res1, res2;
    signal(SIGINT, signal_handler);

    log_file = fopen("log.txt", "w");
	if (log_file == NULL) 
	{
    	perror("Error opening file");
    	return -1;
	}
    /* postavlja da se aktivira poslije 2s, a T aktivacije je 60ms i 80ms */
    res1 = start_periodic_timer(2000000, 60000, 1);
    res2 = start_periodic_timer(2000000, 80000, 2);

    if (res1 < 0 || res2 < 0) 
    {
        perror("Start Periodic Timer");

        return -1;
    }

    while(1) 
    {
        wait_next_activation(1); /* cekaj sigalarm od tajmera */
        task1();  /* izvrsi posao */
        wait_next_activation(2);
        task2();
    }

    fclose(log_file);

    return 0;
}

void signal_handler(int signal) 
{
    if (signal == SIGINT) 
    {
    	fclose(log_file);
        exit(0);
    }
}
