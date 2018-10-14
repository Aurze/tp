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

void algo1(int tbl1[ROW][COLUMN], int n)
{

    int tbl2[ROW][COLUMN];

    for (int k = 1; k <= n; ++k) {
        for (int i = 0; i < ROW; ++i) {
            for (int j = 0; j < COLUMN; ++j) {
                usleep(WAIT);
                tbl2[i][j] = tbl1[i][j] + (i + j) * k;
            }
        }

        tbl1 = tbl2;
    }
    print(tbl1);
}

void algo2(int tbl1[ROW][COLUMN], int n)
{
    int tbl2[ROW][COLUMN];

    for (int k = 1; k <= n; ++k) {
        for (int j = 0; j < COLUMN; ++j) {
            for (int i = 0; i < ROW; ++i) {
                usleep(WAIT);
                if (j == 0) {
                    tbl2[i][j] = tbl1[i][j] + i * k;
                }
                else {
                    tbl2[i][j] = tbl1[i][j] + tbl2[i][j - 1] * k;
                }
            }
        }

        tbl1 = tbl2;
    }

    print(tbl1);
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

    double timeStart, timeEnd, Texec;
    struct timeval tp;
    gettimeofday(&tp, NULL); // Debut du chronometre
    timeStart = (double)(tp.tv_sec) + (double)(tp.tv_usec) / 1e6;
    // Inserer votre code ici

    switch (atoi(c)) {
    case 1:
        algo1(table, n);
        break;
    case 2:
        algo2(table, n);
        break;
    }

    gettimeofday(&tp, NULL); // Fin du chronometre
    timeEnd = (double)(tp.tv_sec) + (double)(tp.tv_usec) / 1e6;
    Texec = timeEnd - timeStart; //Temps d'execution en secondes

    printf("Temps %f\n", Texec);

    return 0;
}

