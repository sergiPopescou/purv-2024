#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#define R 10
#define K 10
#define NUM_OF_PRIORITIES 28


int matrix[][K] = {
                      {1, 0, 1, 0, 1, 1, 1, 0, 0, 1}, 
                      {0, 1, 1, 1, 1, 0, 1, 0, 0, 0}, 
                      {1, 1, 0, 1, 0, 1, 0, 0, 1, 0},
                      {1, 0, 1, 0, 1, 1, 1, 0, 1, 0}, 
                      {0, 1, 0, 1, 0, 1, 0, 1, 0, 1}, 
                      {1, 1, 0, 1, 0, 1, 0, 0, 1, 0},
                      {1, 0, 0, 1, 0, 1, 1, 1, 1, 1}, 
                      {0, 1, 0, 1, 0, 1, 0, 1, 1, 0},
                      {1, 1, 1, 1, 0, 1, 1, 0, 0, 0},
                      {1, 1, 0, 1, 0, 1, 0, 0, 1, 0}
                  };
/*
int matrix[][K] = {
                      {0, 1, 0, 0, 0, 0, 0, 0, 0, 0}, 
                      {0, 1, 0, 0, 0, 0, 0, 0, 0, 0}, 
                      {0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
                      {0, 0, 0, 0, 0, 0, 1, 1, 0, 0}, 
                      {0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
                      {0, 0, 1, 0, 0, 0, 0, 0, 0, 0}, 
                      {0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
                      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
                  };


*/
int matrix[R][K];
int new_matrix[R][K] = {0};

/* structure that stores the indexes and priority of each cell or thread that corresponds to it */
struct index_prior{
	int i;
	int j;
	int pr;
}index_prior;


sem_t semaphore[NUM_OF_PRIORITIES];


/* printing matrix */
void print_matrix(int matrix[R][K]){
	for(int i = 0; i < R; i++){
    	for(int j = 0; j < K; j++){
      	if(matrix[i][j] == 1){
        	printf("o ");
      	} else {
        	printf(". ");
      	}
    }
    printf("\n");
  }
}


/*  function that, based on the value of the cell and on the basis of the number of living and dead neighbors, calculates the new value of the cell, according to the rules of the game of life */
int alive_cells(int cell, int neighbourgs[], int dim_neighbourgs){
  int i;
  int alive_neighbourgs = 0;
  for(i = 0; i < dim_neighbourgs; i++){
    if(neighbourgs[i]){
      alive_neighbourgs++;
    }
  }

  if(cell){
    if(alive_neighbourgs == 2 || alive_neighbourgs == 3){
      return 1;
    }
  }

  if(!cell){
    if(alive_neighbourgs == 3){
      return 1;
    }
  }

  return 0;
}

// function that counts the number of sem_post calls based on priority
int calculate_posts(int priority) {
    // Logic for determining the number of sem_post calls
    if (priority == 0 || priority == 25 || priority == 26) {
        return 1;
    } else if (priority == 1 || priority == 2 || priority == 23 || priority == 24) {
        return 2;
    } else if (priority == 3 || priority == 4 || priority == 21 || priority == 22) {
        return 3;
    } else if (priority == 5 || priority == 6 || priority == 19 || priority == 20) {
        return 4;
    } else {
        return 5;
    }
}
/* function that implements the creation of a new cell state in the matrix according to the rules of the game*/
void* evolution(void* arg){
	
	struct index_prior a = *(struct index_prior *)arg;	
	
	// all threads except the one honored by the previous thread are waiting
	// at the very beginning, only the thread of priority zero can enter, because it is respected in the main function
	sem_wait(&semaphore[a.pr]);

	int n = 0;
	int cell = matrix[a.i][a.j];
	int neighbourgs[R*K-1] = {0};
	for(int k = a.i-1; k <= a.i+1; k++){
    	for(int z = a.j-1; z <= a.j+1; z++){
            if(!(k == a.i && z == a.j)){    //eliminates the cell itself as its own neighbor
                if((k >= 0 && k < R) && (z >= 0 && z < K)){
                      neighbourgs[n++] = matrix[k][z];  //all neighbors of the cells of the corresponding thread are placed in the neighbors array
                }
            }
        }
    }

    //alive_cells function, based on the cell and its neighbors, returns the new state of the cell and places it in a new grid that is gradually filled
    new_matrix[a.i][a.j] = alive_cells(cell, neighbourgs, n);


		// Dinamičko oslobađanje semafora
    int num_posts = calculate_posts(a.pr);
    for (int i = 0; i < num_posts; i++) {
        sem_post(&semaphore[a.pr + 1]);
    }

	free(arg);

}

/* function that calculates the priority matrix based on the network cell */
	// an empty priority matrix is ​​passed as an argument and then filled
void calculate_priorities(int matrix_of_priorities[R][K]){
    
    int pr;

    for(int i = 0; i < R; i++){
      pr = i * 2;
      for(int j = 0; j < K; j++){
        matrix_of_priorities[i][j] = pr++;
      }
    }
}

/* function using the live and dead cells of the network through the command line to initialize the game of life */ 
void input_matrix(){
	printf("Unesi inicijalni raspored mreze: \n");
	for(int i = 0; i < R; i++){
		for(int j = 0; j < K; j++){
			printf("pozicija [%d][%d]: ", i, j);
			int elem;
			do{
				scanf("%d", &elem);
				if(elem == 1 || elem == 0){
					break;
				}
				printf("Unesi ponovo poziciju [%d][%d]: ", i, j);
			} while(1);
			matrix[i][j] = elem;
		}
	}
}

/* Function which initializes cells matrix with random states. */
void init_matrix(int mat[R][K]){
     int i,j;
     srand(time(NULL));
     for(i=0;i<R;i++)
          for(j=0;j<K;j++)
               mat[i][j] = (rand() % 2 == 0) ? 1 : 0;
}


int main(int argc, char* argv[]){

	/* the sleep time of the main thread (1 parameter) is passed to the main function as a command line argument*/
  if(argc < 2){
 		printf("Nije proslijedjen ulazni parametar!\n");
    return 1;
  }

  int sleep_time;
  sleep_time= atoi(argv[1]);				
    
  //input_matrix();

 	system("clear");									
	print_matrix(matrix);
	sleep(sleep_time);
	
	/*one thread is formed for each element of the network, ie here is a reference to the thread for now*/
	pthread_t th[R][K];
	

	/* calculation of the priority matrix */
	int matrix_of_priorities[R][K];
  calculate_priorities(matrix_of_priorities);



  /*  infinite while loop that allows the program to run until it is manually terminated*/
	while(1){

		for(int i = 0; i < NUM_OF_PRIORITIES; i++){
			sem_init(&semaphore[i], 0, 0);
		}
		sem_post(&semaphore[0]);				//we respect the zero semaphore, ie the first thread that has a priority of zero
		
		/*creating a thread and passing a parameter in the form of a struct index_prior structure that contains thread indexes and its priority */
		for(int i = 0; i < R; i++){
			for(int j = 0; j < K; j++){
				struct index_prior *a = malloc(sizeof(struct index_prior));
				a->i = i;
				a->j = j;
				a->pr = matrix_of_priorities[i][j];
				if(pthread_create(&th[i][j], NULL, &evolution, a) != 0){
					perror("failed to create\n");
					return 1;
				}
			}
		}

		for(int i = 0; i < R; i++){ 
			for(int j = 0; j < K; j++){
				if(pthread_join(th[i][j], NULL) != 0){
					perror("failed to join\n");
					return 2;
				}
			}
		}

		for(int i = 0; i < R; i++){
	        for(int j = 0; j < K; j++){
	          matrix[i][j] = new_matrix[i][j];
	        }
	    }

	    system("clear");
	    print_matrix(matrix);
	    fflush(stdout);
	    sleep(sleep_time);
	}

	for(int i = 0; i < NUM_OF_PRIORITIES; i++){
		sem_destroy(&semaphore[i]);
	}

	return 0;
}
