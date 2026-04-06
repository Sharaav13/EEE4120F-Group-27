#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    int nprocs, rank;
    
    /* Initialize the MPI environment */
    MPI_Init(&argc, &argv);
    
    /* Get the number of processes */
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    
    /* Get the rank of the process */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    /* Each process prints its rank */
    printf("Hello World from process %d\n", rank);
    
    /* Only master process (rank 0) does this */
    if (rank == 0) {
        printf("Number of processes = %d\n", nprocs);
    }
    
    /* Finalize the MPI environment */
    MPI_Finalize();
    
    return 0;
}