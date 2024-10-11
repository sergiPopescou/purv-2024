#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

int matrix[10][10] = {{0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
				  	  {1, 1, 0, 0, 1, 0, 1, 0, 0, 0},
				      {1, 0, 1, 0, 0, 0, 0, 0, 0, 0},
				      {0, 0, 0, 0, 0, 0, 0, 1, 1, 0},
				      {0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
				      {1, 0, 0, 0, 0, 0, 0, 1, 0, 0},
				      {1, 1, 0, 1, 0, 1, 0, 0, 0, 0},
				      {0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
				      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
				      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

int pom_matrix[10][10];
sem_t semaphores_enter[28];
sem_t semaphores_exit[28];
pthread_mutex_t mutex;
int sleep_usec = 300000; /* default sleep time between prints in microseconds */
int done_cells[28] ={0};

int find_new_state(int i, int j); /* function that returns 1 if next state of cell is alive, and 0 if not */
void* cell_func(void* arg); /* cell thread function */
void print_matrix(int matrix[10][10]);
int find_number_of_posts(int priority);

int main(int argc, char *argv[]) {
	int i,j;
	if(argc>1)
		sleep_usec = atoi(argv[1]);
	for(i=0;i<28;i++) {
		sem_init(&semaphores_enter[i], 0, 0);
	}
	for(i=0;i<28;i++) {
		sem_init(&semaphores_exit[i], 0, 0);
	}
	sem_post(&semaphores_enter[0]);
	pthread_t cells[100];
	pthread_mutex_init(&mutex, NULL);
	int args[100];

	for(i=0;i<10;i++) {
		for(j=0;j<10;j++) {
			int priority = (i*2+j+1);
			args[i*10+j] = (priority*100)+(i*10)+j; // priority + row + col, e.g priority 10, row 3, col 2 => arg = 1032
			pthread_create(&cells[i*10+j],NULL, &cell_func, (void*)(&args[i*10+j]));
		}
	}

	for(i=0;i<10;i++)
		for(j=0;j<10;j++)
			pthread_join(cells[i*10+j], NULL);

	for(i=0;i<28;i++)
		sem_destroy(&semaphores_enter[i]);
	pthread_mutex_destroy(&mutex);
	for(i=0;i<28;i++)
		sem_destroy(&semaphores_exit[i]);
	pthread_mutex_destroy(&mutex);

	return 0;
}

void* cell_func(void* arg) {
	int k,l;
	int priority = (*(int*)arg)/100;
	int i = ((*(int*)arg)/10)%10; // row
	int j = (*(int*)arg)%10; // col
	
	int number_of_posts_prev = find_number_of_posts(priority-1);
	int number_of_posts_this = find_number_of_posts(priority);
	int number_of_posts_next = find_number_of_posts(priority+1);

	while(1) {
		sem_wait(&semaphores_enter[priority-1]);
		pthread_mutex_lock(&mutex);
			if(priority != 1 && done_cells[priority-2] == number_of_posts_prev) {
				for(k=0;k<number_of_posts_prev;k++)
					sem_post(&semaphores_exit[priority-2]);
				done_cells[priority-2] = 0;
			}
		pthread_mutex_unlock(&mutex);
		int new_state = find_new_state(i, j);
		pom_matrix[i][j] = new_state;
		if(priority == 28) {
			for(k=0;k<10;k++)
				for(l=0;l<10;l++)
					matrix[k][l] = pom_matrix[k][l];
			for(k=0;k<28;k++)
				done_cells[k] = 0;
			printf("\e[1;1H\e[2J");
			print_matrix(matrix);
			fflush(stdout);
			usleep(sleep_usec);
			sem_post(&semaphores_enter[0]);
			continue;
		}
		pthread_mutex_lock(&mutex);
			if(++done_cells[priority-1] == number_of_posts_this) {
				for(k=0;k<number_of_posts_next;k++) {
					sem_post(&semaphores_enter[priority]);
				}
			}
		pthread_mutex_unlock(&mutex);
		sem_wait(&semaphores_exit[priority-1]);
	}
	return 0;
}

void print_matrix(int matrix[10][10]) {
	int i,j;
	for(i=0;i<10;i++) {
		for(j=0;j<10;j++)
			matrix[i][j] == 0 ? printf(" ") : printf("#");
		printf("\n");
	}
}

int find_number_of_posts(int priority) {
	if(priority == 1 || priority == 2 || priority == 27 || priority == 28)
		return 1;
	else if(priority == 3 || priority == 4 || priority == 26 || priority == 25)
		return 2;
	else if(priority == 5 || priority == 6 || priority == 23 || priority == 24)
		return 3;
	else if(priority == 7 || priority == 8 || priority == 22 || priority == 21)
		return 4;
	else return 5;
}

int find_new_state(int i, int j) {
	int ii,jj;
	int num_of_live_neighbours = 0;
	for(ii=i-1;ii<i+2;ii++) {
		for(jj=j-1;jj<j+2;jj++) {
			if(ii<0 || ii>9 || jj<0 || jj>9 || (ii==i && jj==j))
				continue;
			if(matrix[ii][jj] == 1)
				num_of_live_neighbours++;
		}
	}

	if(num_of_live_neighbours > 3 || num_of_live_neighbours < 2) {
		return 0;
	}
	else if(num_of_live_neighbours == 2 && matrix[i][j] == 1) {
		return 1;
	}
	else if(num_of_live_neighbours == 3) {
		return 1;
	}
	else
		return 0;
}
