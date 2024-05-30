#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#define CLEAN_SCREEN printf("\e[1;1H\e[2J"); 
char matrix[10][10] = {
	{' ', ' ', ' ', ' ', '#', ' ', ' ', ' ', ' '},
	{'#', '#', ' ', ' ', '#', ' ', '#', ' ', ' '},
	{'#', ' ', '#', ' ', ' ', ' ', ' ', ' ', ' '},
	{' ', ' ', ' ', ' ', ' ', ' ', ' ', '#', '#'},
	{' ', '#', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
	{'#', ' ', ' ', ' ', ' ', ' ', ' ', '#', ' '},
	{'#', '#', ' ', '#', ' ', '#', ' ', ' ', ' '},
	{' ', ' ', ' ', ' ', ' ', '#', ' ', ' ', ' '},
	{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
	{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
};

char pom_matrix[10][10];
void INThandler(int);

// default delay between matrix printouts in microseconds.
int delay_us = 200000;
void print_matrix();
int find_next_state(int i, int j);
void print_all_new_states();
void* cell_func(void *pParam);
void* sig_thread_func(void* pParam);
int thread_arg[100];

// priority of cells that are being calculated.
int next_priority = 1;

// number of cells with same priority, that are being calculated.
int num_of_todo = 1;
static sem_t semaphore;
pthread_mutex_t mutex;

int main(int argc, char* args[]) {
	if(argc > 1)
		delay_us = atoi(args[1]);

	sem_init(&semaphore, 0, 0);
	CLEAN_SCREEN
	print_matrix();
	usleep(delay_us);
	CLEAN_SCREEN

	pthread_t threads[100];
	pthread_t sig_thread;
	pthread_mutex_init(&mutex, NULL);
	pthread_create(&sig_thread, NULL, sig_thread_func, NULL);

	for(int i=0;i<100;i++) {
		thread_arg[i] = i;
		pthread_create(&threads[i], NULL, cell_func, (void*)&thread_arg[i]);
	}
	for(int i=0;i<100;i++) {
		pthread_join(threads[i], NULL);
	}
	sem_destroy(&semaphore);
	pthread_mutex_destroy(&mutex);
}

/* cell trhread body
 * after next state of current cell is calcluated
 * it waits for semaphore and is blocked.
 * After next state of cell with priority 28(last cell) is calculated,
 * matrix is printed, and
 * that thread posts semaphore 100 times(number of threads/cells),
 * and the cycle of calculating continues from priority 1
*/

void* cell_func(void *pParam) {
	int index = *(int*)pParam;
	int i = index/10;
	int j = index%10;
	int priority = i*2+j+1;

	while(1) {
		if(priority == next_priority && num_of_todo>0) {
			CLEAN_SCREEN
			pom_matrix[i][j] = find_next_state(i, j) ? '#' : ' ';
			if(priority == 28) {
				num_of_todo = 1;
				for(int l=0;l<10;l++) {
					for(int k=0;k<10;k++) {
						matrix[l][k] = pom_matrix[l][k];
					}
				}
				print_matrix();
				usleep(delay_us);
				fflush(stdout);
				for(int i=0;i<99;i++)
					sem_post(&semaphore);
				next_priority = 1;
				continue;
			}
			num_of_todo--;
			if(num_of_todo == 0) {
				int pom;
				pom = (priority+1)/2 + (priority+1)%2;
				if((priority+1) > 10)
					pom = 5;
				if((priority+1) > 20) {
					pom = (priority+1)/5;
				}
				if((priority+1) > 22) {
					pom = (priority+1)/7;
				}
				if((priority+1) == 25 || (priority+1) == 26)
					pom = 2;
				if(priority+1 == 27 || priority+1 == 28)
					pom = 1;
				next_priority++;
				num_of_todo=pom;
			}
			sem_wait(&semaphore);
		}
		else
			usleep(15000);
	}
	return 0;
}

void print_matrix() {
	for(int i=0;i<10;i++) {
		for(int j=0;j<10;j++) {
			printf("%c", matrix[i][j]);
		}
		printf("\n");
		fflush(stdout);
	}
}

/* returns 1 if (i, j) cell will be alive in next cycle, otherwise 0. */
int find_next_state(int i, int j) {
	int number_of_live_neighbours = 0;
	for(int k = i-1; k<=i+1;k++) {
		for(int l = j-1; l<=j+1;l++) {
			if(k>=0 && k<10 && l>=0 && l<10 && !(k==i && l==j) && matrix[k][l] == '#')
				number_of_live_neighbours++;
		}
	}
	if(number_of_live_neighbours < 2)
		return 0;
	if(number_of_live_neighbours == 2 && matrix[i][j] == '#')
		return 1;
	if(number_of_live_neighbours == 3)
		return 1;
	else return 0;
}

/* body of thread watching interrupt */
void* sig_thread_func(void* pParam) {
	int index = *(int*)pParam;
	index+=10;
	signal(SIGINT, INThandler);
	while(1)
		usleep(200000);
}

/* cleanup after interrupt */
void INThandler(int sig) {
	signal(sig, SIG_IGN);
	sem_destroy(&semaphore);
	pthread_mutex_destroy(&mutex);
	exit(0);
}
