#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define SIZE 10
#define ALIVE 1
#define DEAD 0

// Struktura koja opisuje svaku celiju
typedef struct {
    int x, y;
    int current_state;
    int next_state;
} Cell;

// Tabela igre
Cell grid[SIZE][SIZE];

// Barijera za sinhronizaciju niti
pthread_barrier_t barrier; 

// Funkcija za brojanje živih suseda
int count_alive_neighbors(int x, int y) 
{
    int alive_neighbors = 0;
	
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
			
			/* Ne racunamo celije koje izlaze van opsega table i samu celiju (x,y)*/
            if ( (x+i)<0 || (x+i)>=SIZE || (y+j)<0 || (y+j)>=SIZE || ( j==0 && i==0 ) ) continue;
			
            int nx = x + i;
            int ny = y + j;
			
            if (grid[nx][ny].current_state == ALIVE) {
                alive_neighbors++;
            }
        }
    }
    return alive_neighbors;
}

void print_grid();
// Inicijalizacija tabele sa slučajnim stanjima
void initialize_grid();

// Funkcija koju će svaka nit (ćelija) izvršavati
void* cell_routine(void* arg) {
    Cell* cell = (Cell*)arg;

    while (1) {

        int alive_neighbors = count_alive_neighbors(cell->x, cell->y);

        if (cell->current_state == ALIVE) {
			// Živa ćelija sa manje od 2 živa suseda ili vise od 3 ziva susjeda umire
            if (alive_neighbors < 2 || alive_neighbors > 3) {
                cell->next_state = DEAD;
		    // Živa ćelija sa 2 ili 3 živa suseda preživljava.
            } else if (alive_neighbors == 2 || alive_neighbors == 3){
                cell->next_state = ALIVE;
            }
        } else {
			// Mrtva ćelija sa tačno 3 živa suseda oživljava
            if (alive_neighbors == 3) {
                cell->next_state = ALIVE;
            } else {
                cell->next_state = DEAD;
            }
        }

        // Sinhronizacija sa drugim ćelijama
        pthread_barrier_wait(&barrier);

        // Ažuriranje stanja nakon sinhronizacije
        cell->current_state = cell->next_state;
	
        // Sinhronizacija pre sledećeg ciklusa
        pthread_barrier_wait(&barrier);
    }

    return NULL;
}

int main()
{
    pthread_t threads[SIZE][SIZE];
	
	/*Inicijalizacija barijere tako da barijera ceka na dolazak 101 niti 
	 * Barijera funkcioniše tako što blokira svaku nit koja dođe do nje, 
	 * sve dok sve niti ne dostignu tu istu barijeru. Kada sve niti dostignu barijeru, 
	 * one se "oslobode" i nastavljaju sa izvršavanjem.
	*/
	
    pthread_barrier_init(&barrier, NULL, SIZE * SIZE + 1); // 100 celija + 1 za glavnu nit

    initialize_grid();
	print_grid();
	sleep(5); 

    // Kreiranje niti za svaku ćeliju
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            pthread_create(&threads[i][j], NULL, cell_routine, (void*)&grid[i][j]);
        }
    }

    // Glavna nit koja kontroliše ispis i pauzu
    while (1) {
        pthread_barrier_wait(&barrier); // Čeka da sve niti završe izračunavanje sledećeg stanja
        pthread_barrier_wait(&barrier); // Čeka da se niti sinhronizuju pre sledećeg ciklusa
		print_grid();
        sleep(5); 
    }

    // Uništavanje barijere (iako je ovde nepotrebno zbog beskonačne petlje)
    pthread_barrier_destroy(&barrier);

    return 0;
}

void print_grid() 
{
    system("clear"); // Brisanje ekrana

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
			if (grid[i][j].current_state == ALIVE)
				printf("%c",'#');
			else 
				printf("%c",'-');
		}
		printf("\n");
	}
}

// Inicijalizacija tabele sa slučajnim stanjima
void initialize_grid()
{
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++)
		{
            grid[i][j].x = i;
            grid[i][j].y = j;
            grid[i][j].current_state = rand() % 2;
            grid[i][j].next_state = DEAD;
        }
    }
}