#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sys/time.h"

#define N 1
#define M 2
#define NP 3
#define TD 4
#define H 5
#define NBPROC 6
#define WAIT 7

#define SEQ 0
#define PAR 1

void printSeperator()
{
	printf("\n--------------------------------------------------------------\n");
}

int check(int argc, char** argv)
{
	int n,       // le nombre de lignes
		m,       // le nombre de colonnes
		np,      // le nombre de pas de temps
		nbproc;  // le nombre de processus à utiliser
	n = atoi(argv[N]);
	m = atoi(argv[M]);
	np = atoi(argv[NP]);
	nbproc = atoi(argv[NBPROC]);

	if (argc < 7)
	{
		printf("Wrong number of argument\n");
		return 1;
	}

	if (n < 1 || m < 1) {
		printf("Invalid Array Length. Valid Range: n~[1-inf[, m~[1-inf[");
		return 1;
	}

	if (np < 0) {
		printf("Nombre de pas inferieur a 0.");
		return 1;
	}

	if (nbproc < 1) {
		printf("Invalid amount of Processors. Valid Range: [1-64]");
		return 1;
	}

	return 0;
}

int exec(int choice, char* n0, char* m0, char* np0, char* nbproc0, char* wait0, char* tdhStr0, char* cStr0)
{
	printf("================================================== Time Of %s\n", (choice == SEQ ? "Seq" : "MPI"));

	char n[strlen(n0)];
	char m[strlen(m0)];
	char np[strlen(np0)];
	char nbproc[strlen(nbproc0)];
	char wait[strlen(wait0)];
	char tdhStr[strlen(tdhStr0)];
	char cStr[strlen(cStr0)];
	strcpy(n, n0);
	strcpy(m, m0);
	strcpy(np, np0);
	strcpy(nbproc, nbproc0);
	strcpy(wait, wait0);
	strcpy(tdhStr, tdhStr0);
	strcpy(cStr, cStr0);

	int nprocI = atoi(nbproc);
	int nI = atoi(n);

	if (nI - 1 < nprocI)
	{
		sprintf(nbproc, "%d", nI - 1);
	}


	int length = 0;
	char* choiceAsCommand;

	if (choice == PAR)
	{
		length += strlen("mpirun -np ");
		length += strlen(nbproc);
		length += strlen(" ./lab3_mpi ");

		choiceAsCommand = (char *)malloc(length + 1);
		snprintf(choiceAsCommand, length + 1, "mpirun -np %s ./lab3_mpi ", nbproc);

		length = 0;
	}
	else
	{
		choiceAsCommand = (char *)malloc(strlen("./lab3_seq "));
		choiceAsCommand = "./lab3_seq ";
	}

	// Length of Command
	length += strlen(choiceAsCommand);
	length += strlen(n);
	length += strlen(m);
	length += strlen(np);
	length += strlen(tdhStr);
	length += strlen(cStr);
	length += strlen(nbproc);
	length += strlen(wait);
	length += strlen(" ") * 6;

	// Command itself
	char * command = (char *)malloc(length);
	strcpy(command, choiceAsCommand);
	strcat(command, n);
	strcat(command, " ");
	strcat(command, m);
	strcat(command, " ");
	strcat(command, np);
	strcat(command, " ");
	strcat(command, tdhStr);
	strcat(command, " ");
	strcat(command, cStr);
	strcat(command, " ");
	strcat(command, nbproc);
	strcat(command, " ");
	strcat(command, wait);

	// Output of Command
	printf("Commande Utilisée: %s\n", command);

	// Execute
	int status = system(command);
	printf("%s status:%d\n", (choice == SEQ ? "Seq" : "MPI"), status);

	return status;
}

void clearMessage(char* arr)
{
	for (int i = 0; i < strlen(arr); i++)
		arr[i] = 0;
}

int doManipulations()
{
	// KB: 1,5 m (height) x 1 m (width)
	char curManipMsg[50];
	char tdhStr[50];
	char cStr[50];

	//
	// Manipulation 1
	// Description: Exécuter le programme avec un h = 0,1, un td = 0,0002 et 100 pas de temps.
	// h = 0.1f and l = 1   -> l/h = n -> 1/0.1f = n
	// h = 0.1f and L = 1.5 -> L/h = m -> 1.5/0.1f = m
	printf("\n");
	printSeperator();
	printf("\nMANIPULATION 1\n");
	// N = 10, M = 15, NP = 100, TD = 0.0002, tdhStr: 0.0002/(0.1f)^2= 0.02, cStr: 1-4(0.02) = 0.92, NbProc = 64, Wait = 0
	exec(SEQ, "10", "15", "100", "64", "0", "0.02", "0.92");
	exec(PAR, "10", "15", "100", "64", "0", "0.02", "0.92");

	//
	// Manipulation 2
	// Description: Répéter l’exécution du programme plusieurs fois. Est-ce qu’on obtient les
	// mêmes résultats (temps d’exécution et les températures) ?
	// Réponse partielle: temps d'exécution variant très peu, température identique
	printf("\n");
	printSeperator();
	printf("\nMANIPULATION 2\n");
	char iStr[2];
	for (int i = 1; i <= 5; ++i)
	{
		clearMessage(curManipMsg);
		sprintf(iStr, "%d", i);
		strcpy(curManipMsg, "#");
		strcat(curManipMsg, iStr);
		printf("%s", curManipMsg);
		// N = 10, M = 15, NP = 100, TD = 0.0002, tdhStr: 0.0002/(0.1f)^2= 0.02, cStr: 1-4(0.02) = 0.92, NbProc = 64, Wait = 0
		exec(PAR, "10", "15", "100", "64", "0", "0.02", "0.92");
		exec(SEQ, "10", "15", "100", "64", "0", "0.02", "0.92");
	}

	//
	// Manipulation 3
	// Description: Tracer la courbe du temps d’exécution en fonction de la taille du problème
	// (en faisant varier d’abord le nombre de subdivisions et ensuite le nombre de pas de temps.
	// 1 graphique pour chacun, donc 2 graphiques).
	printf("\n");
	printSeperator();
	printf("\nMANIPULATION 3\n");
	char hStr[50];

	float h = 0.1f;
	// Variation de H de 0.1f, 0.2, 0.3, 0.4, 0.5]
	for (int i = 1; i <= 5; ++i)
	{
		clearMessage(curManipMsg);
		sprintf(hStr, "%7.5f", h*i);
		strcpy(curManipMsg, "H=");
		strcat(curManipMsg, hStr);
		printf("%s", curManipMsg);
		sprintf(tdhStr, "%7.5f", (0.0002) / ((h*i)*(h*i)));
		sprintf(cStr, "%7.5f", 1 - 4 * ((0.0002) / ((h*i)*(h*i))));
		// N = 10, M = 15, NP = 100, TD = 0.0002, NbProc = 64, Wait = 0
		exec(PAR, "10", "15", "100", "64", "0", tdhStr, cStr);
		exec(SEQ, "10", "15", "100", "64", "0", tdhStr, cStr);
	}
	// Variation de td de 5, 10, 20, 50, 100
	double td[5] = { 0.00005, 0.0001,0.0002,0.001,0.002 };
	char tdStr[15];
	for (int i = 0; i < 5; ++i)
	{
		clearMessage(curManipMsg);
		sprintf(tdStr, "%f", td[i]);
		strcpy(curManipMsg, "td=");
		strcat(curManipMsg, tdStr);
		printf("%s", curManipMsg);
		sprintf(tdhStr, "%7.5f", td[i] / (0.1f*0.1f)); // 0.0002/(0.1f)^2
		sprintf(cStr, "%7.5f", 1 - 4 * (td[i] / (0.1f*0.1f))); // 1-4(0.02)
		// N = 10, M = 15, NP = 100, NbProc = 64, Wait = 0
		exec(PAR, "10", "15", "100", "64", "0", tdhStr, cStr);
		exec(SEQ, "10", "15", "100", "64", "0", tdhStr, cStr);
	}

	//
	// Manipulation 4
	// Description: Faire une extrapolation du temps d’exécution en fonction
	// de la taille du problème (d’abord pour m, ensuite pour n et finalement
	// pour np. 1 graphique pour chacun, donc 3 graphiques). Vous devez ignorer
	// les contraintes de taille de plaque pour cet exercice.
	printf("\n");
	printSeperator();
	printf("\nMANIPULATION 4\n");
	char nOrMStr[5];
	// Variation de N = 10, 50, 100, 500, 1000
	int nValues[5] = { 10,50,100,500,1000 };
	for (int i = 0; i < 5; ++i)
	{
		clearMessage(curManipMsg);
		sprintf(nOrMStr, "%d", nValues[i]);
		strcpy(curManipMsg, "N=");
		strcat(curManipMsg, nOrMStr);
		printf("%s", curManipMsg);
		exec(PAR, nOrMStr, "15", "100", "64", "0", "0.02", "0.92");
		exec(SEQ, nOrMStr, "15", "100", "64", "0", "0.02", "0.92");
	}
	// Variation de M = 10, 50, 100, 500, 1000
	int mValues[5] = { 10,50,100,500,1000 };
	for (int i = 0; i < 5; ++i)
	{
		clearMessage(curManipMsg);
		sprintf(nOrMStr, "%d", mValues[i]);
		strcpy(curManipMsg, "M=");
		strcat(curManipMsg, nOrMStr);
		printf("%s", curManipMsg);
		exec(PAR, "10", nOrMStr, "100", "64", "0", "0.02", "0.92");
		exec(SEQ, "10", nOrMStr, "100", "64", "0", "0.02", "0.92");
	}
	// Variation de td de 5, 10, 20, 50, 100
	for (int i = 0; i < 5; ++i)
	{
		clearMessage(curManipMsg);
		sprintf(hStr, "%7.5f", h*i);
		strcpy(curManipMsg, "td=");
		strcat(curManipMsg, tdStr);
		printf("%s", curManipMsg);
		sprintf(tdStr, "%f", td[i]);
		sprintf(tdhStr, "%7.5f", td[i] / (0.1f*0.1f)); // 0.0002/(0.1f)^2
		sprintf(cStr, "%7.5f", 1 - 4 * (td[i] / (0.1f*0.1f))); // 1-4(0.02)
		exec(PAR, "10", "15", "100", "64", "0", tdhStr, cStr);
		exec(SEQ, "10", "15", "100", "64", "0", tdhStr, cStr);
	}

	//
	// Manipulation 5
	// Description: Fixer les tailles du problème à m = 200,
	// n = 300 et le nombre de pas = 100. Faites varier le
	// nombre de processus P
	printf("\n");
	printSeperator();
	printf("\nMANIPULATION 5\n");
	char pStr[3];
	// Variation de P = 4, 8, 16, 32, 64
	int pValues[5] = { 4,8,16,32,64 };
	for (int i = 0; i < 5; ++i)
	{
		clearMessage(curManipMsg);
		sprintf(pStr, "%d", pValues[i]);
		strcpy(curManipMsg, "P=");
		strcat(curManipMsg, pStr);
		printf("%s", curManipMsg);
		exec(PAR, "300", "200", "100", pStr, "0", "0.02", "0.92");
		exec(SEQ, "300", "200", "100", pStr, "0", "0.02", "0.92");
	}

	//
	// Manipulation 6
	// Description: Fixer le nombre de processus à 8,
	// le nombre de pas de temps à 100 et faites varier
	// la taille du problème (le nombre de subdivisions).
	// On suggère de faire varier h de 0,15 à 0,00125.
	// Tracer l’accélération en fonction de h (1 graphique).
	printf("\n");
	printSeperator();
	printf("\nMANIPULATION 6\n");
	float hValues[5] = { 0.15,0.05,0.01,0.005,0.00125 };
	// Variation de H de 0.15 à 0.00125
	for (int i = 0; i < 5; ++i)
	{
		float h = hValues[i];
		clearMessage(curManipMsg);
		sprintf(hStr, "%7.5f", h);
		strcpy(curManipMsg, "H=");
		strcat(curManipMsg, hStr);
		printf("%s", curManipMsg);
		sprintf(tdhStr, "%7.8f", 0.0000002 / (h*h)); // 0.0002/(0.1f)^2
		sprintf(cStr, "%7.8f", 1 - 4 * (0.0000002 / (h*h))); // 1 - 4(0.02)
		exec(PAR, "10", "15", "100", "8", "0", tdhStr, cStr);
		exec(SEQ, "10", "15", "100", "8", "0", tdhStr, cStr);
	}

	return 0;
}

int main(int argc, char** argv)
{
	doManipulations();
	//    printSeperator();

	//    if (check(argc, argv) != 0)
	//        return 1;

	//    //char wait[strlen(argv[WAIT])];

	//    float h,       // la taille d’un côté d’une subdivision
	//          td,      // le temps discrétisé
	//          tdh,
	//          c;

	//    td      = atof(argv[TD]);
	//    h       = atof(argv[H]);
	//    tdh = (td / (h * h));
	//    c = 1.0f - 4.0f * tdh;

	//    char tdhStr[50];
	//    char cStr[50];

	//    // TODO: Convert correctly to String
	//    sprintf(tdhStr, "%.7f", tdh);
	//    sprintf(cStr, "%.7f", c);

	//    //if(argc == 8)
	//    //	wait = argv[WAIT];
	//    //else
	//    //	wait = "5";

	//    //
	//    // Sequential
	//    exec(SEQ, argv[N], argv[M], argv[NP], argv[NBPROC], argv[WAIT], tdhStr, cStr);

	//    // Seperator
	//    printSeperator();

	//    //
	//    // Parralel
	//    exec(PAR, argv[N], argv[M], argv[NP], argv[NBPROC], argv[WAIT], tdhStr, cStr);

	//    // Seperator
	//    printSeperator();

	return 0;
}