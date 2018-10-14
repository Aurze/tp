PROG = lab1
CXX = g++
CFLAGS = -g -fPIC -m64 -O3 -Wall -std=gnu11
LFLAGS= -fPIC -m64 -Wall
CC = mpicc

all: $(PROG)

lab1.o: lab1.c
	$(CC) $(CFLAGS) -c lab1.c

lab1 : lab1.o
	$(CC) $(LFLAGS) lab1.o -o lab1

run:
	mpirun --hostfile hostfile -np 24 lab1

ps:
	ps -fu $$USER

clean:
	/bin/rm -f *~
	/bin/rm -f *.o

test-mpi:
	mpirun --hostfile hostfile -np 4 lab1
