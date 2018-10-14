#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <sstream>

#include "Lab4.h"

#define M 1
#define N 2
#define NP 3
#define TD 4
#define H 5

#define SEQ 0
#define PAR 1
#define BOTH 2

#pragma region Print

#if defined(PRINT_INITIAL_MATRIX) || defined(PRINT_FINAL_MATRIX)
void print1DMatrix(unsigned int n, unsigned int m, float* arr) {
	return;
	auto size = static_cast<int>(n*m);
	for (auto i = 0; i < size; i++) {
		printf("%12.3f", arr[i]);
		if (i % m == m - 1)
			std::cout << std::endl;
	}
#ifdef PAUSE
	system("pause");
#endif
}

void print2DMatrix(int n, int m, int np, float*** arr) {
	return;
	for (auto i = 0; i < n; ++i) {
		for (auto j = 0; j < m; ++j) {
			printf("%8.3f", arr[i][j][np]);
		}
		printf("\n");
	}
#ifdef PAUSE
	system("pause");
#endif
}
#endif

void printTime(float seq, float par) {
	printf("Temps OpenCL:%10.10f\n", par);
	printf("Temps Sequentiel:%10.10f\n", seq);
	printf("Temps Acceleration:%10.10f\n", seq / par);
}

#pragma endregion

void check(int argc, char ** argv) {
	if(argc < H+1) {
		std::cout << "Invalid amount of arguments" << std::endl;
		std::cout << "lab4.exe m n np td h" << std::endl;
		std::cout << "m : Le nombre de colonnes de la matrice de travail" << std::endl;
		std::cout << "n : Le nombre de lignes de la matrice de travail" << std::endl;
		std::cout << "np : Le nombre d’itérations à effectuer" << std::endl;
		std::cout << "td : Taille d’un pas de temps discrétisé" << std::endl;
		std::cout << "h : Taille d’une subdivision" << std::endl;
		exit(1);
	}

	int n,       // le nombre de lignes 
		m,       // le nombre de colonnes 
		np;      // le nombre de pas de temps

	m = atoi(argv[M]);
	if(m < 3 || m > 10000) {
		std::cout << "Invalid value for argument 'm'. Valid Range is [3-10000]";
		exit(2);
	}
	n = atoi(argv[N]);
	if (n < 3 || n > 10000) {
		std::cout << "Invalid value for argument 'n'. Valid Range is [3-10000]";
		exit(2);
	}
	np = atoi(argv[NP]);
	if (np < 1 || np > 10000) {
		std::cout << "Invalid value for argument 'np'. Valid Range is [1-10000]";
		exit(2);
	}
}

void exec(int choice, unsigned int m, unsigned int n, int np, float tdh, float c) {
	if (choice == SEQ) {
		seq(m, n, np, tdh, c);
	}
	else if (choice == PAR) {
		open_cl(m, n, np, tdh, c);
	}
	else {
		float temps_seq = seq(m, n, np, tdh, c);
		float temps_par = open_cl(m, n, np, tdh, c);
		printTime(temps_seq, temps_par);
	}
}

void doManipulations(int choice, unsigned int n, unsigned int m, int np, float tdh, float c) {
	std::vector<int> nmValues = { 100, 500, 1000, 1500, 2000 };
	std::vector<int> nAndMValues = { 10, 100, 250, 500, 1000 };
	std::vector<int> npValues = { 100, 200, 300, 400 };
	std::vector<double> tdValues = { 0.00005, 0.0001, 0.0002, 0.001, 0.002 };
	std::vector<double> hValues = { 0.00001, 0.0005, 0.001, 0.05, 0.1 };
	auto defaultH = 0.1f;
	auto i = 0;
	
	//
	// N
	std::cout << "=============================================" << std::endl;
	std::cout << "                    N                        " << std::endl;
	std::cout << "=============================================" << std::endl;
	for (i = 0; i < nmValues.size(); ++i) {
		std::cout << "N=" << nmValues.at(i) << std::endl;
		exec(choice, m, nmValues[i], np, tdh, c);
#ifdef PAUSE
		system("pause");
#endif
	}

	//
	// M
	std::cout << "=============================================" << std::endl;
	std::cout << "                    M                        " << std::endl;
	std::cout << "=============================================" << std::endl;
	for (i = 0; i < nmValues.size(); ++i) {
		std::cout << "M=" << nmValues.at(i) << std::endl;
		exec(choice, nmValues.at(i), n, np, tdh, c);
#ifdef PAUSE
		system("pause");
#endif
	}

	//
	// M
	std::cout << "=============================================" << std::endl;
	std::cout << "                    N*M                      " << std::endl;
	std::cout << "=============================================" << std::endl;
	for (i = 0; i < nAndMValues.size(); ++i) {
		std::cout << "N&M=" << nAndMValues.at(i) << std::endl;
		exec(choice, nAndMValues.at(i), nAndMValues.at(i), np, tdh, c);
#ifdef PAUSE
		system("pause");
#endif
	}

	//
	// NP
	std::cout << "=============================================" << std::endl;
	std::cout << "                    NP                       " << std::endl;
	std::cout << "=============================================" << std::endl;
	for (i = 0; i < npValues.size(); ++i) {
		std::cout << "NP=" << npValues.at(i) << std::endl;
		exec(choice, m, n, npValues.at(i), tdh, c);
#ifdef PAUSE
		system("pause");
#endif
	}

	//
	// TD -> TDH & C
	std::cout << "=============================================" << std::endl;
	std::cout << "                 TD -> TDH, C                " << std::endl;
	std::cout << "=============================================" << std::endl;
	float tdhValue = 0.0f;
	float cValue = 0.0f;
	for (i = 0; i < tdValues.size(); ++i) {
		std::cout << "TD=" << tdValues.at(i) << std::endl;
		tdhValue = tdValues.at(i) / (defaultH*defaultH);
		cValue = 1 - 4 * tdhValue;
		exec(choice, m, n, np, tdhValue, cValue);
#ifdef PAUSE
		system("pause");
#endif
	}

	//
	// H
	std::cout << "=============================================" << std::endl;
	std::cout << "                       H                     " << std::endl;
	std::cout << "=============================================" << std::endl;
	for (i = 0; i < hValues.size(); ++i) {
		std::cout << "TD=" << hValues.at(i) << std::endl;
		tdhValue = tdValues.at(i) / (hValues.at(i)*hValues.at(i));
		cValue = 1 - 4 * tdhValue;
		exec(choice, m, n, np, tdhValue, cValue);
#ifdef PAUSE
		system("pause");
#endif
	}
}

//
// Main Entry
int main(int argc, char** argv) {

#ifdef BENCHMARK
	// Manipulations
	doManipulations(BOTH, 150, 100, 100, 0.1f, 0.2f);
#else

	// Check arguments
	check(argc, argv);

	int n,       // le nombre de lignes 
		m,       // le nombre de colonnes 
		np;      // le nombre de pas de temps       

	float td,    // Taille d'un temps discrétisé
		h,       // Taille d'une subdivision
		tdh,     // TD/H^2
		c;       // 1-4(TD/H^2)

	m = atoi(argv[M]);
	n = atoi(argv[N]);
	np = atoi(argv[NP]);
	td = atof(argv[TD]);
	h = atof(argv[H]);

	tdh = td / (h * h);
	c = 1 - 4 * tdh;

	// Sequentiel
	auto temps_seq = seq(m, n, np, tdh, c);

	// Parallele
	auto temps_par = open_cl(m, n, np, tdh, c);
	
	// Temps
	printTime(temps_seq, temps_par);

#endif

	system("pause");

	return 0;
}