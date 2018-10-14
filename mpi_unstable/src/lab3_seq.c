#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sys/time.h"

#define N 1
#define M 2
#define NP 3
#define TDH 4
#define C 5
#define NBPROC 6
#define WAIT 7


float ***alloc_3d_float(int n, int m, int np) {
    float*** array;
    int i, j;
    array = (float ***)malloc(sizeof(float **) * n);
    for (i = 0 ;  i < n; i++) {
        array[i] = (float **)malloc(sizeof(float *) * m);

        for (j = 0; j < m; j++)
            array[i][j] = (float *)malloc(sizeof(float) * np);
    }

    return array;
}

void seq(int n, int m, int np, float tdh, float c, int nbproc, int wait)
{
    float*** u = alloc_3d_float(n, m, np + 1);
    //float u[n][m][np + 1];
	// Plaque Métallique au temps 0
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < m; ++j) {
			for (int k = 0; k < np + 1; ++k) {
				if (i == 0
						|| j == 0
						|| i == n -1
						|| j == m -1)
					u[i][j][k] = 0.0f;
				else
					u[i][j][k] = (i*(n - i - 1))*(j*(m - j - 1));
			}
		}
	}
	
	// Start Timer
	double timeStart, timeEnd, Texec;
	struct timeval tp;
	gettimeofday (&tp, NULL); // Debut du chronometre
	timeStart = (double) (tp.tv_sec) + (double) (tp.tv_usec) / 1e6;
	//

	// Plaque Métallique au temps k
	for (int k = 0; k < np; ++k) {
		for(int i = 1; i < n - 1; ++i) {
			for (int j = 1; j < m - 1; ++j) {
				usleep(wait);
				u[i][j][k + 1] = c * u[i][j][k]
						+ tdh*
							( u[i - 1][j][k]
							+ u[i + 1][j][k]
							+ u[i][j - 1][k]
							+ u[i][j + 1][k]);

			}
		}
	}

	// End Timer
	gettimeofday (&tp, NULL); // Fin du chronometre
	timeEnd = (double) (tp.tv_sec) + (double) (tp.tv_usec) / 1e6;
	Texec = timeEnd - timeStart; //Temps d'execution en secondes
	//

	printf("K=%d:\n", np);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            printf("%8.3f", u[i][j][np]);
		}
		printf("\n");
	}
	printf("Temps: %f secondes\n", Texec);

}


int main(int argc, char** argv)
{
	int n,       // le nombre de lignes
		m,       // le nombre de colonnes
		np,      // le nombre de pas de temps
		nbproc,  // le nombre de processus à utiliser
		wait;

	float tdh, c;

	n       = atoi(argv[N]);
	m       = atoi(argv[M]);
	np      = atoi(argv[NP]);
	tdh      = atof(argv[TDH]);
	c       = atof(argv[C]);
	nbproc  = atoi(argv[NBPROC]);
	wait    = atoi(argv[WAIT]);


	seq(n, m, np, tdh, c, nbproc, wait);

	return 0;
}
