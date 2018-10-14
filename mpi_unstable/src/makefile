PROG = lab3 lab3_mpi lab3_seq
CXX = g++
CFLAGS = -g -fPIC -m64 -O3 -Wall -std=gnu11
LFLAGS= -fPIC -m64 -Wall
CC = mpicc

all: $(PROG)

lab3.o: lab3.c
	$(CC) $(CFLAGS) -c lab3.c

lab3_mpi.o: lab3_mpi.c
	$(CC) $(CFLAGS) -c lab3_mpi.c

lab3_seq.o: lab3_seq.c
	$(CC) $(CFLAGS) -c lab3_seq.c

lab3 : lab3.o
	$(CC) $(LFLAGS) lab3.o -o lab3

lab3_mpi : lab3_mpi.o
	$(CC) $(LFLAGS) lab3_mpi.o -o lab3_mpi

lab3_seq : lab3_seq.o
	$(CC) $(LFLAGS) lab3_seq.o -o lab3_seq

run:
	lab3
	mpirun --hostfile hostfile -np 24 lab3_mpi

ps:
	ps -fu $$USER

clean:
	/bin/rm -f *~
	/bin/rm -f *.o

test-mpi:
	mpirun --hostfile hostfile -np 4 lab3