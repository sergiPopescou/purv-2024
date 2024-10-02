#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>


/* Definitions of ioctl commands */
#define WR_VALUE _IOW('a', 'a', struct CELL_NEIGHBORS*)
#define RD_VALUE _IOR('a', 'b', int*)

#define R 3
#define K 3

/* Initial game of life */
int matrix[R][K] = {
	{0, 1, 0}, 
	{0, 1, 0},
	{0, 1, 0}
};

/* Struct where data about cells from user space is stored, and then passed to kernel space using ioctl */
struct CELL_NEIGHBORS {
	int cell;
	int neighbors[R*K-1];
	int dim_neighbors;
};


/* Function that prints cells */
void print_matrix(int matrix[R][K]){
  int i, j;
  for(i = 0; i < R; i++){
      for(j = 0; j < K; j++){
        if(matrix[i][j] == 1){
          printf("* ");
        } else {
          printf("  ");
        }
    }
    printf("\n");
  }
}


int main(){

	int fd;
	fd = open("/dev/ext_device", O_RDWR);
	if(fd < 0){
		printf("Cannot open device file...\n");
		return 0;
	}

    printf("Initial arrangemenet of cells in game of life:\n");
	print_matrix(matrix);
	usleep(400000);

    while(1){

        int new_matrix[R][K] = {0};

        int i, j, k, z;
        for(i = 0; i < R; i++){
            for(j = 0; j < K; j++){

                int n;
                struct CELL_NEIGHBORS data; 
                data.cell = matrix[i][j]; 
                for(n = 0; n < R; n++){
                	data.neighbors[n] = 0;
                }    
      
                for(k = i-1; k <= i+1; k++){
                    for(z = j-1; z <= j+1; z++){
                        if(!(k == i && z == j)){         // eliminites cell as its own neighbout
                            if((k >= 0 && k < R) && (z >= 0 && z < K)){
                            // adds cells neighbour into array that is later to be analyzed
                                data.neighbors[n++] = matrix[k][z];  
                            }
                        }
                    }
                }
                data.dim_neighbors = n;


                int novo_stanje = 0;
                printf("Writing new state into driver\n");
                /* IOCTL call that sends new_matrix to kernel space*/
                ioctl(fd, WR_VALUE, (struct CELL_NEIGHBORS*)&data);
				usleep(400000);
                printf("Reading new state from driver \n");
                novo_stanje = 0;
                /* IOCTL call that receives new_matrix from kernel space*/
                ioctl(fd, RD_VALUE, (int *) &novo_stanje);
               	new_matrix[i][j] = novo_stanje;
               	usleep(400000);
               	
            }
        }

        for(i = 0; i < R; i++){
            for(j = 0; j < K; j++){
                matrix[i][j] = new_matrix[i][j];
            }
        }

         system("clear");
         printf("New arrangement of cells in matrix:\n");
         print_matrix(matrix);
		 usleep(400000);
         fflush(stdout);
    }

    close(fd);

	return 0;
}



