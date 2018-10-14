#include "sys/time.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ROW 8
#define COL 8
#define COLUMN 8
#define MASTER 0
#define FROM_MASTER 1
#define FROM_WORKER 2
#define WAIT 1000

void print(int table[ROW][COLUMN])
{
    for (int i = 0; i < ROW; ++i) {
        for (int j = 0; j < COLUMN; ++j) {
            printf("%-7i|", table[i][j]);
        }
        printf("\n");
    }
}

void algo1_mpi(int argc, char* argv[], int tbl1[ROW][COLUMN], int n, int p)
{
    int taskid,
        row,
        col;

    int a[ROW][COLUMN];

    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);

    if (taskid == MASTER) {

	double timeStart, timeEnd, Texec;
   	struct timeval tp;
   	gettimeofday(&tp, NULL); // Debut du chronometre
    	timeStart = (double)(tp.tv_sec) + (double)(tp.tv_usec) / 1e6;

        int id = 1;
        for (row = 0; row < ROW; row++) {
            for (col = 0; col < COL; col++) {
                MPI_Send(&row, 1, MPI_INT, id, FROM_MASTER, MPI_COMM_WORLD);
                MPI_Send(&col, 1, MPI_INT, id, FROM_MASTER, MPI_COMM_WORLD);
                MPI_Send(&n, 1, MPI_INT, id, FROM_MASTER, MPI_COMM_WORLD);
                ++id;
            }
        }

        id = 1;
        for (row = 0; row < ROW; row++) {
            for (col = 0; col < COL; col++) {
                MPI_Recv(&row, 1, MPI_INT, id, FROM_WORKER, MPI_COMM_WORLD, &status);
                MPI_Recv(&col, 1, MPI_INT, id, FROM_WORKER, MPI_COMM_WORLD, &status);
                MPI_Recv(&a[row][col], 1, MPI_INT, id, FROM_WORKER, MPI_COMM_WORLD, &status);
                ++id;
            }
        }

        /* Print results */
        print(a);

	gettimeofday(&tp, NULL); // Fin du chronometre
	timeEnd = (double)(tp.tv_sec) + (double)(tp.tv_usec) / 1e6;
	Texec = timeEnd - timeStart; //Temps d'execution en secondes

	printf("Temps %f\n", Texec);
    }
    else {
        int val = p;
        MPI_Recv(&row, 1, MPI_INT, MASTER, FROM_MASTER, MPI_COMM_WORLD, &status);
        MPI_Recv(&col, 1, MPI_INT, MASTER, FROM_MASTER, MPI_COMM_WORLD, &status);
        MPI_Recv(&n, 1, MPI_INT, MASTER, FROM_MASTER, MPI_COMM_WORLD, &status);

        for (int k = 1; k <= n; ++k) {
            usleep(WAIT);

            val = val + (row + col) * k;
        }

        MPI_Send(&row, 1, MPI_INT, MASTER, FROM_WORKER, MPI_COMM_WORLD);
        MPI_Send(&col, 1, MPI_INT, MASTER, FROM_WORKER, MPI_COMM_WORLD);
        MPI_Send(&val, 1, MPI_INT, MASTER, FROM_WORKER, MPI_COMM_WORLD);
    }

    MPI_Finalize();
}


// calcule
// on utilise la colonne de gauche + la cellule de l'iteration precedante
// on traverse le cube en diagonal
// on va donc utiliser tout les cores a la diagonal
//  
//  5 6 7 8 9
//  4 5 6 7 8
//k 3 4 5 6 7
//  2 3 4 5 6
//  1 2 3 4 5
//     col 

// Comme on boucle sur l'iteration on a pas besoin recevoir la cellule
// Juste resevoir la colonne de gauche
void algo2_mpi(int argc, char* argv[], int tbl1[ROW][COLUMN], int n, int p)
{
    int taskid,
        row,
        col;

    int a[ROW][COLUMN];

    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);

    if (taskid == MASTER) {

	double timeStart, timeEnd, Texec;
   	struct timeval tp;
   	gettimeofday(&tp, NULL); // Debut du chronometre
    	timeStart = (double)(tp.tv_sec) + (double)(tp.tv_usec) / 1e6;

        int id = 1;
        for (col = 0; col < COL; col++) {
            for (row = 0; row < ROW; row++) {
                MPI_Recv(&a[row][col], 1, MPI_INT, id, FROM_WORKER, MPI_COMM_WORLD, &status);
                ++id;
            }
        }

        /* Print results */
        print(a);

	gettimeofday(&tp, NULL); // Fin du chronometre
	timeEnd = (double)(tp.tv_sec) + (double)(tp.tv_usec) / 1e6;
	Texec = timeEnd - timeStart; //Temps d'execution en secondes

	printf("Temps %f\n", Texec);

    }

    else if (taskid < 9) {
        int val = p;

        // Calc
        for (int k = 1; k <= n; ++k) {
            usleep(WAIT);
            val = val + (taskid - 1) * k;
            // Send
            MPI_Send(&val, 1, MPI_INT, taskid + 8, FROM_WORKER, MPI_COMM_WORLD);
        }

        // Send
        MPI_Send(&val, 1, MPI_INT, MASTER, FROM_WORKER, MPI_COMM_WORLD);
    }
    else if (taskid < 57) {
        int val = p;
        int preVal = p;

        // Calc
        for (int k = 1; k <= n; ++k) {
            MPI_Recv(&preVal, 1, MPI_INT, taskid - 8, FROM_WORKER, MPI_COMM_WORLD, &status);
            usleep(WAIT);
            val = val + preVal * k;
            // Send
            MPI_Send(&val, 1, MPI_INT, taskid + 8, FROM_WORKER, MPI_COMM_WORLD);
        }

        // Send
        MPI_Send(&val, 1, MPI_INT, MASTER, FROM_WORKER, MPI_COMM_WORLD);
    }
    else {
        int val = p;
        int preVal = p;

        // Calc
        for (int k = 1; k <= n; ++k) {
            MPI_Recv(&preVal, 1, MPI_INT, taskid - 8, FROM_WORKER, MPI_COMM_WORLD, &status);
            usleep(WAIT);
            val = val + preVal * k;
        }

        // Send
        MPI_Send(&val, 1, MPI_INT, MASTER, FROM_WORKER, MPI_COMM_WORLD);
    }

    MPI_Finalize();
}

int main(int argc, char** argv)
{
    if (argc < 4) {
        printf("Wrong number of argument\n");
        return 1;
    }
    char* c;
    int p;
    int n;
    c = argv[1];
    p = atoi(argv[2]);
    n = atoi(argv[3]);

    int table[ROW][COLUMN];

    for (int i = 0; i < ROW; ++i) {
        for (int j = 0; j < COLUMN; ++j) {
            table[i][j] = p;
        }
    }

    switch (atoi(c)) {
    case 1:
        algo1_mpi(argc, argv, table, n, p);
        break;
    case 2:
        algo2_mpi(argc, argv, table, n, p);
        break;
    }
    return 0;
}

