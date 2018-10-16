 
# Some work from school
## mpi
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

## openMP
Same as mpi

## mpi_unstable
In this case the dimantion of the matrix are unkown so we need to asigne "block" to minimise comunication between cores

## opencl
Kinda like mpi algo 1, but using the GPU.

## embeded
TODO
