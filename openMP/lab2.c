#include "sys/time.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ROW 10
#define COL 10
#define WAIT 50000 // 50 Milliseconds
//#define WAIT 1 // 50 Milliseconds

void print(int table[ROW][COL])
{
    for (int i = 0; i < ROW; ++i) {
        for (int j = 0; j < COL; ++j) {
            printf("%7i |", table[i][j]);
        }
        printf("\n");
    }
}

void algo1_seq(int table[ROW][COL], int nombreIterations, int valeurInitiale)
{
    // init le tableau
    for (int i = 0; i < ROW; ++i) {
        for (int j = 0; j < COL; ++j) {
            table[i][j] = valeurInitiale;
        }
    }

    // Calcule
    for (int k = 0; k < nombreIterations; ++k) {
        for (int i = 0; i < ROW; ++i) {
            for (int j = 0; j < COL; ++j) {
                usleep(WAIT);
                table[i][j] = table[i][j] + i + j;
            }
        }
    }
}

void algo1_openMP(int table[ROW][COL], int nombreIterations, int valeurInitiale)
{
    //init le tableau
    for (int i = 0; i < ROW; ++i) {
        for (int j = 0; j < COL; ++j) {
            table[i][j] = valeurInitiale;
        }
    }

#pragma omp parallel for schedule(static)
    for (int i = 0; i < ROW * COL; ++i) {
        for (int k = 0; k < nombreIterations; ++k) {
            usleep(WAIT);
            int row = i % ROW;
            int col = i / COL;
            table[row][col] = table[row][col] + row + col;
        }
    }
}

void algo1(int nombreIterations, int valeurInitiale)
{

    //Seq

    double timeStart, timeEnd, Texec;
    struct timeval tp;
    gettimeofday(&tp, NULL); // Debut du chronometre
    timeStart = (double)(tp.tv_sec) + (double)(tp.tv_usec) / 1e6;

    int table[ROW][COL];

    algo1_seq(table, nombreIterations, valeurInitiale);

    gettimeofday(&tp, NULL); // Fin du chronometre
    timeEnd = (double)(tp.tv_sec) + (double)(tp.tv_usec) / 1e6;
    Texec = timeEnd - timeStart; //Temps d'execution en secondes
    printf("\n====================\n");
    printf("=    Sequentielle  =\n");
    printf("====================\n");
    print(table);
    printf("Temps %f\n", Texec);

    //OpenMP

    double timeStartOpenMP, timeEndOpenMP, TexecOpenMP;
    struct timeval tpOpenMP;
    gettimeofday(&tpOpenMP, NULL); // Debut du chronometre
    timeStartOpenMP = (double)(tpOpenMP.tv_sec) + (double)(tpOpenMP.tv_usec) / 1e6;

    algo1_openMP(table, nombreIterations, valeurInitiale);

    gettimeofday(&tpOpenMP, NULL); // Fin du chronometre
    timeEndOpenMP = (double)(tpOpenMP.tv_sec) + (double)(tpOpenMP.tv_usec) / 1e6;
    TexecOpenMP = timeEndOpenMP - timeStartOpenMP; //Temps d'execution en secondes

    printf("\n====================\n");
    printf("=      OpenMP      =\n");
    printf("====================\n");
    print(table);
    printf("Temps OpenMP %f\n", TexecOpenMP);

    printf("Diff : %f\n", Texec - TexecOpenMP);
    printf("Acceleration : %f%%\n", Texec / TexecOpenMP);
}

void algo2_seq(int table[ROW][COL], int nombreIterations, int valeurInitiale)
{
    //init le tableau
    for (int i = 0; i < ROW; ++i) {
        for (int j = 0; j < COL; ++j) {
            table[i][j] = valeurInitiale;
        }
    }

    // calcule
    // on utilise la colonne precedante
    for (int k = 0; k < nombreIterations; ++k) {
        for (int i = 0; i < ROW; ++i) {
            for (int j = COL - 1; j >= 0; --j) {
                usleep(WAIT);
                if (j == COL - 1)
                    table[i][j] = table[i][j] + i;
                else
                    table[i][j] = table[i][j] + table[i][j + 1];
            }
        }
    }
}

void algo2_openMP(int table[ROW][COL], int nombreIterations, int valeurInitiale)
{
    int tbl[nombreIterations + 1][ROW][COL];

    //init le tableau
    for (int i = 0; i < ROW; ++i) {
        for (int j = 0; j < COL; ++j) {
            tbl[0][i][j] = valeurInitiale;
        }
    }

    int bornCol = COL - 1; // 9

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
    
    for (int col = bornCol; col >= -nombreIterations; --col) {
        int max = fmax(1, -1 * col);
        int min = fmin(nombreIterations, bornCol - col);
#pragma omp parallel for collapse(2)
        for (int k = max; k <= min; ++k) {
            for (int row = 0; row < ROW; ++row) {
                usleep(WAIT);
                if (col + k == bornCol)
                    tbl[k][row][col + k] = tbl[k - 1][row][col + k] + row;
                else
                    tbl[k][row][col + k] = tbl[k - 1][row][col + k] + tbl[k][row][col + 1 + k];
            }
        }
    }

    for (int row = 0; row < ROW; ++row) {
        for (int col = 0; col < COL; ++col) {
            table[row][col] = tbl[nombreIterations][row][col];
        }
    }
}

void algo2(int nombreIterations, int valeurInitiale)
{

    //Seq

    double timeStart, timeEnd, Texec;
    struct timeval tp;
    gettimeofday(&tp, NULL); // Debut du chronometre
    timeStart = (double)(tp.tv_sec) + (double)(tp.tv_usec) / 1e6;

    int table[ROW][COL];

    algo2_seq(table, nombreIterations, valeurInitiale);

    gettimeofday(&tp, NULL); // Fin du chronometre
    timeEnd = (double)(tp.tv_sec) + (double)(tp.tv_usec) / 1e6;
    Texec = timeEnd - timeStart; //Temps d'execution en secondes
    printf("\n====================\n");
    printf("=    Sequentielle  =\n");
    printf("====================\n");
    print(table);
    printf("Temps %f\n", Texec);

    //OpenMP

    double timeStartOpenMP, timeEndOpenMP, TexecOpenMP;
    struct timeval tpOpenMP;
    gettimeofday(&tpOpenMP, NULL); // Debut du chronometre
    timeStartOpenMP = (double)(tpOpenMP.tv_sec) + (double)(tpOpenMP.tv_usec) / 1e6;

    algo2_openMP(table, nombreIterations, valeurInitiale);

    gettimeofday(&tpOpenMP, NULL); // Fin du chronometre
    timeEndOpenMP = (double)(tpOpenMP.tv_sec) + (double)(tpOpenMP.tv_usec) / 1e6;
    TexecOpenMP = timeEndOpenMP - timeStartOpenMP; //Temps d'execution en secondes

    printf("\n====================\n");
    printf("=      OpenMP      =\n");
    printf("====================\n");
    print(table);
    printf("Temps OpenMP %f\n", TexecOpenMP);

    printf("Diff : %f\n", Texec - TexecOpenMP);
    printf("Acceleration : %f%%\n", Texec / TexecOpenMP);
}

int main(int argc, char** argv)
{
    if (argc < 4) {
        printf("Wrong number of argument\n");
        return 1;
    }
    int numeroProbleme,
        valeurInitiale,
        nombreIterations;

    numeroProbleme = atoi(argv[1]);
    valeurInitiale = atoi(argv[2]);
    nombreIterations = atoi(argv[3]);

    switch (numeroProbleme) {
    case 1:
        algo1(nombreIterations, valeurInitiale);
        break;
    case 2:
        algo2(nombreIterations, valeurInitiale);
        break;
    }

    return 0;
}

