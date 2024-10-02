#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>


static struct timespec r1;
static struct timespec r2;
static int period1;
static int period2;

#define NSEC_PER_SEC 1000000000ULL


// 1.task

static inline void timespec_add_us_task1(struct timespec *t, uint64_t d)
{
    d *= 1000;
    d += t->tv_nsec;
    while (d >= NSEC_PER_SEC) {
        d -= NSEC_PER_SEC;
		    t->tv_sec += 1;
    }
    t->tv_nsec = d;
}

int start_periodic_timer_task1(uint64_t offs, int t)
{
    clock_gettime(CLOCK_REALTIME, &r1);
    timespec_add_us_task1(&r1, offs);

    period1 = t;

    return 0;
}


static void wait_next_activation_task1(void)
{
    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &r1, NULL);
    timespec_add_us_task1(&r1, period1);


}



// 2. task
static inline void timespec_add_us_task2(struct timespec *t, uint64_t d)
{
    d *= 1000;
    d += t->tv_nsec;
    while (d >= NSEC_PER_SEC) {
        d -= NSEC_PER_SEC;
            t->tv_sec += 1;
    }
    t->tv_nsec = d;
}

int start_periodic_timer_task2(uint64_t offs, int t)
{
    clock_gettime(CLOCK_REALTIME, &r2);
    timespec_add_us_task2(&r2, offs);

    period2 = t;

    return 0;
}

static void wait_next_activation_task2(void)
{
    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &r2, NULL);
    timespec_add_us_task2(&r2, period2);

}


static void task1(void)
{

    int i,j;
 
    for (i=0; i<4; i++) {
        for (j=0; j<1000; j++) ;
        printf("1");
        fflush(stdout);
    }
    struct timespec r;
    clock_gettime(CLOCK_REALTIME, &r);
   
}

void task2(void)
{

    int i,j;

    for (i=0; i<6; i++) {
        for (j=0; j<10000; j++) ;
        printf("2");
        fflush(stdout);
    }
    struct timespec r;
    clock_gettime(CLOCK_REALTIME, &r);
}



int main(int argc, char *argv[])
{
    
    
    int res1, res2;

    res1 = start_periodic_timer_task1(0, 60000);
    res2 = start_periodic_timer_task2(0, 80000);
     
    if (res1 < 0 || res2 < 0) {
        perror("Start Periodic Timer");

        return -1;
    }


    while(1) {

        
        if((r1.tv_sec > r2.tv_sec) || ((r1.tv_sec == r2.tv_sec) && (r1.tv_nsec > r2.tv_nsec))){
            wait_next_activation_task2();
            task2();
        }
       
        wait_next_activation_task1();
        task1();

        if((r2.tv_sec > r1.tv_sec) || ((r2.tv_sec == r1.tv_sec) && (r2.tv_nsec > r1.tv_nsec))){
           
            wait_next_activation_task1();
            task1();
        }

        wait_next_activation_task2();
        task2();

    }

    return 0;
}
    
