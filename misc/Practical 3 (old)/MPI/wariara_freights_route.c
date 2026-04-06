// =========================================================================
// Practical 3: Minimum Energy Consumption Freight Route Optimization
// =========================================================================
//
// GROUP NUMBER:
//
// MEMBERS:
//   - Member 1 Name, Student Number
//   - Member 2 Name, Student Number

// ========================================================================
//  PART 2: Minimum Energy Consumption Freight Route Optimization using OpenMPI
// =========================================================================


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>
#include "mpi.h"
#include <limits.h>

#define MAX_N 10

 typedef struct {
        int path[MAX_N];      // ordered list of cities visited so far
        int path_length;      // how many cities are in path
        int cost_so_far;      // accumulated energy cost so far
        int visited[MAX_N];   // visited[i] = 1 if city i is already in path
    } Subproblem;
// ============================================================================
// Global variables
// ============================================================================

int n; // If this is -1, it signals an error/exit
int best_cost=INT_MAX;;
int adj[MAX_N][MAX_N];
int best_path[MAX_N];

void branch_and_bound(Subproblem sp);
// ============================================================================
// Timer: returns time in seconds
// ============================================================================

double gettime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// ============================================================================
// Usage function
// ============================================================================


void Usage(char *program) {
  printf("Usage: mpirun -np <num> %s [options]\n", program);
  printf("-i <file>\tInput file name\n");
  printf("-o <file>\tOutput file name\n");
  printf("-h \t\tDisplay this help\n");
}


int main(int argc, char **argv)
{
    int rank, nprocs;
    int opt;
    int i, j;
    char *input_file = NULL;
    char *output_file = NULL;
    FILE *infile = NULL;
    FILE *outfile = NULL;
    int success_flag = 1; // 1 = good, 0 = error/help encountered

    // Initialize MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    // Every process that runs gets a unique rank


    if (rank == 0) { //Only master process handles file I/O
        n = -1; 

        while ((opt = getopt(argc, argv, "i:o:h")) != -1)
        {
            switch (opt)
            {
                case 'i':
                    input_file = optarg;
                    break;

                case 'o':
                    output_file = optarg;
                    break;

                case 'h':
                    Usage(argv[0]);
                    success_flag = 0; 
                    break;

                default:
                    Usage(argv[0]);
                    success_flag = 0;
            }
        }

        
    
        if (success_flag) {
            infile = fopen(input_file, "r");
            if (infile == NULL) {
                fprintf(stderr, "Error: Cannot open input file '%s'\n", input_file);
                perror("");
                success_flag = 0;
            } else {
                
                fscanf(infile, "%d", &n);

                for (i = 1; i < n; i++)
                {
                    for (j = 0; j < i; j++)
                    {
                        fscanf(infile, "%d", &adj[i][j]);
                        adj[j][i] = adj[i][j];
                    }
                }
                fclose(infile);
            }
        }
        if (success_flag) {
            outfile = fopen(output_file, "w");
            if (outfile == NULL) {
                fprintf(stderr, "Error: Cannot open output file '%s'\n", output_file);
                perror("");
                success_flag = 0;
            }
        }

    }


    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD); //sends data from rank 0 to all other processes simultaneously,
    //  everyprocess has an identical copy of n and the full adjacency matrix.

    
    if (n == -1) {
        MPI_Finalize();
        return 0; 
    }

   

  
    MPI_Bcast(&adj[0][0], MAX_N * MAX_N, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {

    /* Initialise best_cost to largest possible integer 
       so any complete route will beat it */
    best_cost = INT_MAX;

    /* Set up the initial subproblem — start at city 0 (1-indexed: city 1) */
    Subproblem sp;
    sp.path[0] = 0;        /* start at city 0 */
    sp.path_length = 1;    /* one city committed so far */
    sp.cost_so_far = 0;    /* no cost yet */

    /* Mark all cities as unvisited, then mark city 0 as visited */
    int i;
    for (i = 0; i < n; i++) {
        sp.visited[i] = 0;
    }
    sp.visited[0] = 1;

    /* Run branch and bound from this starting subproblem */
    branch_and_bound(sp);   

    /* Print result — convert from 0-indexed to 1-indexed for output */
    printf("Best cost: %d\n", best_cost);
    printf("Best route: ");
    for (i = 0; i < n; i++) {
        printf("%d ", best_path[i] + 1);  /* +1 converts to 1-indexed */
    }
    printf("\n");
}
    
    printf("Process %d received adjacency matrix:\n", rank);
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            printf("%d ", adj[i][j]);
        }
        printf("\n");
    }
    printf("\n");

        
    // TODO: compute solution to minimum energy consumption problem here and write to output file
    // Be careful on which process rank writes to the output file to avoid conflicts!
    
    

   
    
/*
 branch_and_bound(Subproblem sp)
    if sp.path_length == n         ← base case: complete route
        update best if sp.cost_so_far < best
        return
    
    for each unvisited city i:
        if sp.cost_so_far + adj[last_city][i] >= best_bound
            skip (prune this branch)
        else
            add city i to sp
            branch_and_bound(sp)   ← recurse deeper
            remove city i from sp  ← backtrack

    MPI_Finalize();
    return 0;

*/


}


void branch_and_bound(Subproblem sp) {
    int i;
    int last_city;
    int edge_cost;

    /* Base case: complete route — all cities have been visited */
    if (sp.path_length == n) {

        /* Update best if this complete route beats the current best */
        if (sp.cost_so_far < best_cost) {
            best_cost = sp.cost_so_far;

            /* Save this route as the new best path */
            for (i = 0; i < n; i++) {
                best_path[i] = sp.path[i];
            }
        }
        return;
    }
    
    /* Get the last city added to the partial route */
    last_city = sp.path[sp.path_length - 1];

    /* Try extending the route with each unvisited city */
    for (i = 0; i < n; i++) {

        /* Skip cities already in the partial route */
        if (sp.visited[i]) continue;

        /* Calculate the cost of travelling to city i from last_city */
        edge_cost = adj[last_city][i];

        /* Prune: if adding this edge already exceeds best known cost, skip */
        if (sp.cost_so_far + edge_cost >= best_cost) continue;

        /* Add city i to the partial route */
        sp.path[sp.path_length] = i;
        sp.visited[i] = 1;
        sp.cost_so_far += edge_cost;
        sp.path_length++;

        /* Recurse deeper into this branch */
        branch_and_bound(sp);

        /* Backtrack: remove city i from the partial route */
        sp.path_length--;
        sp.cost_so_far -= edge_cost;
        sp.visited[i] = 0;
    }


}
