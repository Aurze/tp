# MPI

## Probleme
2 algo run in sequential + parallel
both algo run on a matrix m*n (8*8) for k iteration

algo 1: only use the same cell from previouse iteration (k-1)
algo 2: use same cell from previouse iteration [m, n, k-1] plus previouse columne, same iteration [m-1, n, k]

to be able to use a mximum core, we will calcule "in diagonal" :

  5 6 7 8 9
  4 5 6 7 8
k 3 4 5 6 7
  2 3 4 5 6
  1 2 3 4 5
     m 

## Files
par containe the parallel algo 1 & 2
seq containe the sequential algo 1 & 2
     
## Run in server
`mpirun -n 10 hello 3 4 5`
