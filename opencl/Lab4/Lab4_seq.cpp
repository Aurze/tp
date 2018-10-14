#include <windows.h>
#include <sstream>

#include "Lab4.h"

// ==================================================================
// SEQUENTIAL
// ==================================================================
float ***alloc_3d_float(int n, int m, int np) {
	float*** array;
	int i, j;
	array = static_cast<float ***>(malloc(sizeof(float **)* n));
	for (i = 0; i < n; i++) {
		array[i] = static_cast<float **>(malloc(sizeof(float *)* m));

		for (j = 0; j < m; j++)
			array[i][j] = static_cast<float *>(malloc(sizeof(float)* np));
	}

	return array;
}

void free_3d_float(float*** array, int n, int m, int np) {
	int i, j;
	for (i = 0; i < n; i++) {
		for (j = 0; j < m; j++)
			free(array[i][j]);
		free(array[i]);
	}
	free(array);
}

#pragma region Init

void initBoard(int n, int m, int np, float*** u) {
	// Plaque Metallique au temps 0
	for (auto i = 0; i < n; ++i) {
		for (auto j = 0; j < m; ++j) {
			for (auto k = 0; k < np + 1; ++k) {
				if (i == 0
					|| j == 0
					|| i == n - 1
					|| j == m - 1)
					u[i][j][k] = 0.0f;
				else
					u[i][j][k] = (i*(n - i - 1.0f))*(j*(m - j - 1.0f));
			}
		}
	}
}

#pragma endregion

// Implementation Sequentiel
float seq(unsigned int m, unsigned int n, int np, float tdh, float c) {
	// Intialiser la matrice
	auto u = alloc_3d_float(n, m, np + 1);

	// Init Board
	initBoard(n, m, np, u);
	
#ifdef PRINT_INITIAL_MATRIX
	printf(INITIAL_MATRIX_MESSAGE, "Sequentiel");
	print2DMatrix(n, m, 0, u);
#endif

	// Start Timer
	SYSTEMTIME time;
	GetSystemTime(&time);
	auto startTime = (time.wSecond * 1000) + time.wMilliseconds;

	//
	// Plaque Metallique au temps k
	// Effectuer le traitement sequentiel
	for (auto k = 0; k < np; ++k) {
		for (auto i = 1; i < static_cast<int>(n) - 1; ++i) {
			for (auto j = 1; j < static_cast<int>(m) - 1; ++j) {
				u[i][j][k + 1] = c * u[i][j][k]
					+ tdh*
					(u[i - 1][j][k]
						+ u[i + 1][j][k]
						+ u[i][j - 1][k]
						+ u[i][j + 1][k]);

			}
		}
	}

	// End Timer
	GetSystemTime(&time);
	auto endTime = (time.wSecond * 1000) + time.wMilliseconds;
	auto diff = endTime - startTime;
	//

#ifdef PRINT_FINAL_MATRIX
	printf(FINAL_MATRIX_MESSAGE, "Sequentiel", np);
	print2DMatrix(n, m, np, u);
#endif

	free_3d_float(u, n, m, np + 1);

	return diff;
}