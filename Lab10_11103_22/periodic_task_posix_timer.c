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

void task1(void);
void task2(void);
static sigset_t sigset1;
static sigset_t sigset2;
static void wait_next_activation(int i)
{
    int dummy;

	if(i == 1)
    	sigwait(&sigset1, &dummy);
	else if(i==2)
		sigwait(&sigset2, &dummy); 
}
int start_periodic_timer(uint64_t offs, int period, int i)
{
    struct itimerspec t;
    struct sigevent sigev;
    timer_t timer;
    const int signal = SIGALRM;
    int res;

    t.it_value.tv_sec = offs / 1000000;
    t.it_value.tv_nsec = (offs % 1000000) * 1000;
    t.it_interval.tv_sec = period / 1000000;
    t.it_interval.tv_nsec = (period % 1000000) * 1000;
	
	// ovaj dio ostaje isti
	if(i==1) {
    	sigemptyset(&sigset1);
    	sigaddset(&sigset1, signal);
    	sigprocmask(SIG_BLOCK, &sigset1, NULL);
	}
	if(i==2) {
    	sigemptyset(&sigset2);
    	sigaddset(&sigset2, signal);
    	sigprocmask(SIG_BLOCK, &sigset2, NULL);
	}

	// alociranje i popunjavanje sigevent strukture
    memset(&sigev, 0, sizeof(struct sigevent));
    sigev.sigev_notify = SIGEV_SIGNAL; // kada vrijeme istekne 
									   // okinuce se signal, jer se ostatak
									   // koda oslanja na signale
    sigev.sigev_signo = signal;	// koji signal? pa SIGALRM, naravno
	
	// kreiranje tajmera
    res = timer_create(CLOCK_MONOTONIC, &sigev, &timer);
    if (res < 0) {
        perror("Timer Create");

	exit(-1);
    }
	// pokretanje tajmera 
    return timer_settime(timer, 0 /*TIMER_ABSTIME*/, &t, NULL);
}

static void job_body(void)
{
    static int cnt;
    static uint64_t start;
    uint64_t t;
    struct timeval tv;

    if (start == 0) {
        gettimeofday(&tv, NULL);
	start = tv.tv_sec * 1000ULL + tv.tv_usec / 1000ULL;
    }
        
    gettimeofday(&tv, NULL);
    t = tv.tv_sec * 1000ULL + tv.tv_usec / 1000ULL;
    if (cnt && (cnt % 100) == 0) {
        printf("Avg time: %f\n", (double)(t - start) / (double)cnt);
    }
    cnt++;
}

int main(int argc, char *argv[])
{
    int res1, res2;

    res1 = start_periodic_timer(2000000, 60000, 1);
    res2 = start_periodic_timer(2000000, 80000, 2);
    if (res1 < 0 || res2 < 0) {
        perror("Start Periodic Timer");

        return -1;
    }

	while(1) {
		wait_next_activation(1);
		task1();
		wait_next_activation(2);
		task2();
	}
}

void task1(void) {
	int i,j;
 
  	for (i=0; i<4; i++) {
    	for (j=0; j<1000; j++) ;
    	printf("1");
    	fflush(stdout);
  	}
}

void task2(void) {
	int i,j;

	for (i=0; i<6; i++) {
		for (j=0; j<10000; j++) ;
    	printf("2");
    	fflush(stdout);
 	}
}
