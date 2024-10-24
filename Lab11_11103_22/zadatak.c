#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <sched.h>
#include <linux/sched.h>
#include <unistd.h>

void task1(void);
void task2(void);
void task3(void);
static sigset_t sigset1;
static sigset_t sigset2;
static sigset_t sigset3;
static void wait_next_activation(int i)
{
    int dummy;

	if(i == 1)
    	sigwait(&sigset1, &dummy);
	else if(i==2)
		sigwait(&sigset2, &dummy); 
	else if(i==3)
		sigwait(&sigset3, &dummy); 
}
int start_periodic_timer(uint64_t offs, int period, int i)
{
    struct itimerspec t;
    struct sigevent sigev;
    timer_t timer;
    const int signall = SIGALRM;
    int res;

    t.it_value.tv_sec = offs / 1000000;
    t.it_value.tv_nsec = (offs % 1000000) * 1000;
    t.it_interval.tv_sec = period / 1000000;
    t.it_interval.tv_nsec = (period % 1000000) * 1000;
	
	// ovaj dio ostaje isti
	if(i==1) {
    	sigemptyset(&sigset1);
    	sigaddset(&sigset1, signall);
    	sigprocmask(SIG_BLOCK, &sigset1, NULL);
	}
	else if(i==2) {
    	sigemptyset(&sigset2);
    	sigaddset(&sigset2, signall);
    	sigprocmask(SIG_BLOCK, &sigset2, NULL);
	}
	else if(i==3) {
    	sigemptyset(&sigset3);
    	sigaddset(&sigset3, signall);
    	sigprocmask(SIG_BLOCK, &sigset3, NULL);
	}

	// alociranje i popunjavanje sigevent strukture
    memset(&sigev, 0, sizeof(struct sigevent));
    sigev.sigev_notify = SIGEV_SIGNAL; // kada vrijeme istekne 
									   // okinuce se signal, jer se ostatak
									   // koda oslanja na signale
    sigev.sigev_signo = signall;	// koji signal? pa SIGALRM, naravno
	
	// kreiranje tajmera
    res = timer_create(CLOCK_MONOTONIC, &sigev, &timer);
    if (res < 0) {
        perror("Timer Create");

	exit(-1);
    }
	// pokretanje tajmera 
    return timer_settime(timer, 0 /*TIMER_ABSTIME*/, &t, NULL);
}

int main(int argc, char *argv[])
{
    int res1, res2, res3;
	pid_t child1, child2, child3;
	int status;

    res1 = start_periodic_timer(40000, 40000, 1);
    res2 = start_periodic_timer(80000, 80000, 2);
    res3 = start_periodic_timer(120000, 120000, 3);
    if (res1 < 0 || res2 < 0 || res3 < 0) {
        perror("Start Periodic Timer");

        return -1;
    }

	child1 = fork();
	if(child1<0) {
		perror("Fork");
		return -1;
	}
	if(child1 == 0) {
		//child code
		int pmin = sched_get_priority_min(SCHED_DEADLINE);
		struct sched_param param; 
		param.sched_priority = pmin + 11; 
		sched_setscheduler(0, SCHED_DEADLINE, &param);

		while(1) {
			task1();
			wait_next_activation(1);
		}
		return 33;
	}

	child2 = fork();
	if(child2<0) {
		perror("Fork");
		return -1;
	}
	if(child2 == 0) {
		//child code
		int pmin = sched_get_priority_min(SCHED_DEADLINE);
		struct sched_param param; 
		param.sched_priority = pmin + 12; 
		sched_setscheduler(0, SCHED_DEADLINE, &param);

		while(1) {
			task2();
			wait_next_activation(2);
		}
		return 33;
	}

	child3 = fork();
	if(child3<0) {
		perror("Fork");
		return -1;
	}
	if(child3 == 0) {
		//child code
		int pmin = sched_get_priority_min(SCHED_DEADLINE);
		struct sched_param param; 
		param.sched_priority = pmin + 13; 
		sched_setscheduler(0, SCHED_DEADLINE, &param);

		while(1) {
			task3();
			wait_next_activation(3);
		}
		return 33;
	}

	wait(&status);
	wait(&status);
	wait(&status);

	return 1;
}

// periodicni zadatak na 40 milisekundi
void task1(void) {
	int i,j;
 
	for (i=0; i<4; i++) {
    	for (j=0; j<1000; j++) ;
    	printf("1");
    	fflush(stdout);
	}
}

// periodicni zadatak na 80 milisekundi
void task2(void) {
	int i,j;

	for (i=0; i<6; i++) {
  		for (j=0; j<10000; j++) ;
  		printf("2");
  		fflush(stdout);
	}
}

// periodicni zadatak na 120 milisekundi
void task3(void) {
	int i,j;

	for (i=0; i<6; i++) {
  		for (j=0; j<100000; j++) ;
  		printf("3");
  		fflush(stdout);
	}
}
