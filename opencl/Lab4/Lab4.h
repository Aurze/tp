#pragma once

// Enable/Disable Features here
#define BENCHMARK
#define PRINT_INITIAL_MATRIX
#define PRINT_FINAL_MATRIX
#define PAUSE
//#define DEBUG

#pragma region Features

#ifdef PRINT_INITIAL_MATRIX
static const char* INITIAL_MATRIX_MESSAGE = "Matrice %s Initial :\n";
#endif
#ifdef PRINT_FINAL_MATRIX
static const char* FINAL_MATRIX_MESSAGE = "Matrice %s Final(K = %d) :\n";
#endif
#if defined(PRINT_INITIAL_MATRIX) || defined(PRINT_FINAL_MATRIX)
void print2DMatrix(int n, int m, int np, float*** arr);
void print1DMatrix(unsigned int n, unsigned int m, float* arr);
#endif

#pragma endregion

float seq(unsigned int m, unsigned int n, int np, float tdh, float c);
float open_cl(unsigned int m, unsigned int n, int np, float tdh, float c);