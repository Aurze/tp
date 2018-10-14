#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sys/time.h"
#include <mpi.h>

#define N 1
#define M 2
#define NP 3
#define TDH 4
#define C 5
#define NBPROC 6
#define WAIT 7
#define SUBMATRIXLENGTH 3
#define MASTER 0
#define FROM_MASTER 1
#define FROM_SLAVE 2

float** alloc_2d_float(int n, int m) {
    float** array;
    int i, j;
    array = (float **)malloc(sizeof(float *) * n);
    for (i = 0; i < n; i++) {
        array[i] = (float *)malloc(sizeof(float) * m);
    }

    return array;
}

int get_chunk_over(int n, int m, int nbproc)
{
    if (n > nbproc - 1)
        return n % (nbproc - 1);
    return 0;
}

int get_chunk_height(int n, int m, int nbproc)
{
    if (n > nbproc - 1)
        return n / (nbproc - 1);
    return 1;
}

int get_chunk_width(int n, int m, int nbproc)
{
    return m;
}

int get_n_proc(int n, int m, int nbproc)
{
    if (n > nbproc - 1)
        return nbproc - 1;
    return n;
}

int get_m_proc(int n, int m, int nbproc)
{
    return 1;
}

void mpi_impl(int argc, char** argv, int n, int m, int np, float tdh, float c, int nbproc, int wait)
{
    float u[n][m];// = alloc_2d_float(n, m);
    int p, rank;

    n = n - 2;
    m = m - 2;

    int chunk_height, chunk_width, chunk_over;

    chunk_over = get_chunk_over(n, m, nbproc);
    chunk_height = get_chunk_height(n, m, nbproc);
    chunk_width = get_chunk_width(n, m, nbproc - (n / chunk_height));


    MPI_Init(&argc, &argv);
    MPI_Status mystatus;
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Plaque Metallique au temps 0
    for (int i = 1; i < n + 1; ++i) {
        for (int j = 1; j < m + 1; ++j) {
            u[i - 1][j - 1] = (i*(n - i + 1))*(j*(m - j + 1));
        }
    }

    if (rank == MASTER)
    {
        // Start Timer
        double timeStart, timeEnd, Texec;
        struct timeval tp;
        gettimeofday(&tp, NULL); // Debut du chronometre
        timeStart = (double)(tp.tv_sec) + (double)(tp.tv_usec) / 1e6;

        int id = 1;
        //float val;
        float val[chunk_height + 1][chunk_width];

        id = 1;
        int n_proc = get_n_proc(n, m, nbproc);

        // printf("n_proc:%d chunk_height:%d\n", n_proc, chunk_height);
        //for (int j = 0; j < 1; ++j) {
        for (int i = 0; i < n_proc; ++i) {

            int over = 0;
            int row_start = 0;

            if (i < chunk_over) {
                over = 1;
                row_start = i*(chunk_height + 1);
            }
            else {
                over = 0;
                row_start = chunk_over*(chunk_height + 1)
                        + (i - chunk_over)*chunk_height;
            }
            //printf("recived from id:%d i:%d\n", id, i);
            //printf("recived from row_start:%d chunk_height:%d chunk_width:%d size:%d\n",row_start , chunk_height + over, chunk_width, (chunk_height + over) * chunk_width);
            MPI_Recv(&val, (chunk_height + over) * chunk_width, MPI_FLOAT, id, FROM_SLAVE, MPI_COMM_WORLD, &mystatus);
            //printf("recived from row_start:%d chunk_height:%d chunk_width:%d\n",row_start , chunk_height, chunk_width);

            //printf("i:%d\n", i);
            //printf("VAL:%d row_start:%d\n", id, row_start);
            for (int row = 0; row < chunk_height + over; ++row) {
                for (int col = 0; col < chunk_width; ++col) {
                    //printf("%8.3f", val[row][col]);
                    u[row_start + row][col] = val[row][col];
                }
                //printf("\n");
            }
//            printf("ID:%d\n", id);
//            for (int i = -1; i < n + 1; ++i) {
//                for (int j = -1; j < m + 1; ++j) {
//                    if (i == -1 || i == n || j == -1 || j == m) {
//                        printf("%8.3f", 0.0f);
//                    }
//                    else {
//                        printf("%8.3f", u[i][j]);
//                    }
//                }
//                printf("\n");
//            }
            ++id;
        }
        //}

        // End Timer
        gettimeofday(&tp, NULL); // Fin du chronometre
        timeEnd = (double)(tp.tv_sec) + (double)(tp.tv_usec) / 1e6;
        Texec = timeEnd - timeStart; //Temps d'execution en secondes
        //
        printf("\n");
        for (int i = -1; i < n + 1; ++i) {
            for (int j = -1; j < m + 1; ++j) {
                if (i == -1 || i == n || j == -1 || j == m) {
                    printf("%8.3f", 0.0f);
                }
                else {
                    printf("%8.3f", u[i][j]);
                }
            }
            printf("\n");
        }
        printf("Temps: %f secondes\n", Texec);
    }
    else
    {
        if (rank - 1 < chunk_over)
            chunk_height++;

        float last_val[chunk_height][chunk_width];
        float val[chunk_height][chunk_width];
        float up[chunk_width],
                bottom[chunk_width],
                left[chunk_height],
                right[chunk_height];

        for (int i = 0; i < chunk_height; ++i) {
            left[i] = 0;
            right[i] = 0;
        }

        for (int i = 0; i < chunk_width; ++i) {
            up[i] = 0;
            bottom[i] = 0;
        }

        int row = 0; //((rank-1) % n) * chunk_height
        int col = 0; //((rank-1) / m) * chunk_width;

        if (rank - 1 < chunk_over)
            row = ((rank - 1) % n)*(chunk_height);

        else
            row = chunk_over*(chunk_height + 1)
                    + (((rank - 1) % n) - chunk_over)*chunk_height;;

        //float val = u[row][col][0];
        // initialiser la grille
        for (int i = 0; i < chunk_height; ++i) {
            for (int j = 0; j < chunk_width; ++j) {
                last_val[i][j] = u[row + i][col + j];
            }
        }

        for (int k = 1; k <= np; ++k)
        {

            //printf("Debut de la boucle rank :%d k:%d\n", rank, k);
            // Recevoir les valeurs de l'itération précédente
            if (k != 1)
            {
                //printf("MPI_Recv rank :%d row:%d chunk_height:%d\n", rank, row, chunk_height);
                if (row != 0)
                    MPI_Recv(&up, chunk_width, MPI_FLOAT, rank - 1, FROM_SLAVE, MPI_COMM_WORLD, &mystatus);
                if (row + chunk_height < n)
                    MPI_Recv(&bottom, chunk_width, MPI_FLOAT, rank + 1, FROM_SLAVE, MPI_COMM_WORLD, &mystatus);
                //                if (col != 0)
                //                    MPI_Recv(&left,     chunk_height, MPI_FLOAT, rank - n, FROM_SLAVE, MPI_COMM_WORLD, &mystatus);
                //                if (col + chunk_width < m)
                //                    MPI_Recv(&right,	chunk_height, MPI_FLOAT, rank + n, FROM_SLAVE, MPI_COMM_WORLD, &mystatus);
            }
            else
            {
                if (row != 0)
                    for (int j = 0; j < chunk_width; ++j)
                        up[j] = u[row - 1][col + j];
                if (row + chunk_height < n)
                    for (int j = 0; j < chunk_width; ++j)
                        bottom[j] = u[row + chunk_height][col + j];
                //                if (col != 0)
                //                    for (int i = 0; i < chunk_height; ++i)
                //                        left[i] = u[row + i][col - 1][0];
                //                if (col + chunk_width < m-1)
                //                    for (int i = 0; i < chunk_height; ++i)
                //                        right[i] = u[row + i][col + chunk_width][0];
            }

            // Faire le calcul
            float val_up,
                    val_bottom,
                    val_left,
                    val_right;
            for (int i = 0; i < chunk_height; ++i) {
                for (int j = 0; j < chunk_width; ++j) {
                    if (i == 0)
                        val_up = up[j];
                    else
                        val_up = last_val[i - 1][j];

                    if (i == chunk_height - 1)
                        val_bottom = bottom[j];
                    else
                        val_bottom = last_val[i + 1][j];

                    if (j == 0)
                        val_left = left[i];
                    else
                        val_left = last_val[i][j - 1];

                    if (j == chunk_width - 1)
                        val_right = right[i];
                    else
                        val_right = last_val[i][j + 1];

                    usleep(wait);
                    val[i][j] = c*last_val[i][j] + tdh*(val_up + val_bottom + val_left + val_right);

                }
            }
            //printf("on va send rank :%d k:%d\n", rank, k);
            // Envoyer la valeur courante aux cellules voisines
            if (k != np)
            {
                // Top
                if (row != 0) {
                    float tmp[chunk_width];
                    for (int j = 0; j < chunk_width; ++j) {
                        tmp[j] = val[0][j];
                    }
                    MPI_Send(&tmp, chunk_width, MPI_FLOAT, rank - 1, FROM_SLAVE, MPI_COMM_WORLD);
                }
                // Bottom
                if (row + chunk_height < n) {
                    float tmp[chunk_width];
                    for (int j = 0; j < chunk_width; ++j) {
                        tmp[j] = val[chunk_height - 1][j];
                    }
                    MPI_Send(&tmp, chunk_width, MPI_FLOAT, rank + 1, FROM_SLAVE, MPI_COMM_WORLD);
                }
                //                // Right
                //                if (col + chunk_width < m){
                //                    float tmp[chunk_height];
                //                    for (int i = 0; i < chunk_height; ++i) {
                //                        tmp[i] = val[i][chunk_width - 1];
                //                    }
                //                    MPI_Send(&tmp, chunk_height, MPI_FLOAT, rank + n, FROM_SLAVE, MPI_COMM_WORLD);
                //                }
                //                // Left
                //                if (col != 0){
                //                    float tmp[chunk_height];
                //                    for (int i = 0; i < chunk_height; ++i) {
                //                        tmp[i] = val[i][0];
                //                    }
                //                    MPI_Send(&tmp, chunk_height, MPI_FLOAT, rank - n, FROM_SLAVE, MPI_COMM_WORLD);
                //                }
            }

            for (int i = 0; i < chunk_height; i++)
            {
                memcpy(&last_val[i], &val[i], sizeof(val[0]));
            }
        }
//        if (rank ==1){
//            for (int i = 0; i < chunk_height; ++i) {
//                for (int j = 0; j < chunk_width; ++j) {
//                    printf("%8.3f", val[row + i][col + j]);
//                }
//                printf("\n");
//            }
//        }
//        printf("Last send rank:%d send size :%d\n", rank , chunk_height * chunk_width);
        MPI_Send(&val, chunk_height * chunk_width, MPI_FLOAT, MASTER, FROM_SLAVE, MPI_COMM_WORLD);
    }

    MPI_Finalize();
}

int main(int argc, char** argv)
{
    int n,       // le nombre de lignes
            m,       // le nombre de colonnes
            np,      // le nombre de pas de temps
            nbproc,  // le nombre de processus a utiliser
            wait;

    float tdh, c;

    n = atoi(argv[N]);
    m = atoi(argv[M]);
    np = atoi(argv[NP]);
    tdh = atof(argv[TDH]);
    c = atof(argv[C]);
    nbproc = atoi(argv[NBPROC]);
    wait = atoi(argv[WAIT]);

    mpi_impl(argc, argv, n, m, np, tdh, c, nbproc, wait);

    return 0;
}
