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
//  PART 2: Minimum Energy Consumption Freight Route Optimization using MPI
// =========================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>
#include <limits.h>
#include "mpi.h"

#define MAX_N 10

// ============================================================================
// Global variables
// ============================================================================

int n;                      // number of cities; -1 signals an error / exit
int adj[MAX_N][MAX_N];

// ============================================================================
// Timer: returns time in seconds (wall-clock)
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

// ============================================================================
// Branch-and-Bound recursive solver
//
//  path[]       : current partial path (1-indexed city numbers)
//  path_len     : number of cities visited so far
//  visited[]    : boolean array of visited cities (0-indexed)
//  current_cost : accumulated energy cost so far
//  best_cost    : pointer to the best cost found by THIS process so far
//  best_path[]  : best path found by THIS process so far
// ============================================================================

void branch_and_bound(int *path, int path_len, int *visited,
                      int current_cost,
                      int *best_cost, int *best_path)
{
    if (path_len == n) {
        if (current_cost < *best_cost) {
            *best_cost = current_cost;
            memcpy(best_path, path, n * sizeof(int));
        }
        return;
    }

    int last = path[path_len - 1] - 1;  // 0-indexed

    for (int next = 0; next < n; next++) {
        if (visited[next]) continue;

        int new_cost = current_cost + adj[last][next];

        if (new_cost >= *best_cost) continue;  // prune

        visited[next]    = 1;
        path[path_len]   = next + 1;  // 1-indexed

        branch_and_bound(path, path_len + 1, visited, new_cost,
                         best_cost, best_path);

        visited[next] = 0;
    }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv)
{
    int rank, nprocs;
    int opt;
    int i, j;
    char *input_file  = NULL;
    char *output_file = NULL;
    FILE *infile  = NULL;
    FILE *outfile = NULL;
    int success_flag = 1;

    double t_init_start = MPI_Wtime();  // MPI_Wtime available after MPI_Init on all processes

    // -------------------------------------------------------------------------
    // Initialise MPI
    // -------------------------------------------------------------------------
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    // -------------------------------------------------------------------------
    // Rank 0: parse arguments and load the graph
    // -------------------------------------------------------------------------
    if (rank == 0) {
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
                for (i = 1; i < n; i++) {
                    for (j = 0; j < i; j++) {
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

        if (!success_flag) n = -1;  // signal error to all workers
    }

    // -------------------------------------------------------------------------
    // Broadcast n first — value of -1 means abort
    // -------------------------------------------------------------------------
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (n == -1) {
        MPI_Finalize();
        return 0;
    }

    // -------------------------------------------------------------------------
    // Broadcast the adjacency matrix to all processes
    // -------------------------------------------------------------------------
    MPI_Bcast(&adj[0][0], MAX_N * MAX_N, MPI_INT, 0, MPI_COMM_WORLD);

    double t_init_end = MPI_Wtime();
    double t_init = t_init_end - t_init_start;

    if (rank == 0)
        printf("Running with %d MPI processes on a graph with %d nodes\n", nprocs, n);

    // =========================================================================
    // Parallelisation strategy
    // =========================================================================
    // The route always starts at City 1 (0-indexed: city 0).
    // We expand the first level of the B&B tree — the (n-1) choices for
    // the second city — and distribute them across processes using a simple
    // cyclic (round-robin) assignment:
    //
    //   process r handles first-level children where  child_index % nprocs == r
    //
    // Each process runs its own sequential Branch-and-Bound on its subset
    // of sub-trees.  After all processes finish, we use MPI_Reduce to find
    // the global minimum cost, then MPI_Bcast / direct send to share the
    // winning path so that rank 0 can write the output.
    // =========================================================================

    double t_comp_start = MPI_Wtime();

    int local_best_cost = INT_MAX;
    int local_best_path[MAX_N];
    memset(local_best_path, 0, sizeof(local_best_path));

    // Number of first-level sub-trees = n - 1  (cities 1 .. n-1 in 0-indexed)
    int num_children = n - 1;

    for (i = 0; i < num_children; i++)
    {
        // Cyclic distribution: this process only handles sub-trees assigned to it
        if (i % nprocs != rank) continue;

        int path[MAX_N];
        int visited[MAX_N];
        memset(visited, 0, sizeof(visited));

        // City 1 is the fixed start (0-indexed: city 0)
        visited[0] = 1;
        path[0]    = 1;

        int next      = i + 1;          // 0-indexed second city
        int step_cost = adj[0][next];

        if (step_cost >= local_best_cost) continue;  // prune

        visited[next] = 1;
        path[1]       = next + 1;       // 1-indexed for output

        branch_and_bound(path, 2, visited, step_cost,
                         &local_best_cost, local_best_path);
    }

    double t_comp_end = MPI_Wtime();
    double t_comp = t_comp_end - t_comp_start;

    // -------------------------------------------------------------------------
    // Reduction step 1: find the global minimum cost
    // -------------------------------------------------------------------------
    int global_best_cost = INT_MAX;
    MPI_Reduce(&local_best_cost, &global_best_cost, 1,
               MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);

    // -------------------------------------------------------------------------
    // Reduction step 2: the process that owns the best path sends it to rank 0
    //
    // Strategy: rank 0 broadcasts global_best_cost; the process whose
    // local_best_cost matches sends its path to rank 0.
    // (If two processes tie on cost, the one with the lower rank wins —
    //  both paths are equally optimal.)
    // -------------------------------------------------------------------------
    MPI_Bcast(&global_best_cost, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int global_best_path[MAX_N];
    memset(global_best_path, 0, sizeof(global_best_path));

    if (rank == 0) {
        if (local_best_cost == global_best_cost) {
            // Rank 0 already has the best path
            memcpy(global_best_path, local_best_path, n * sizeof(int));
        } else {
            // Receive from whichever worker has it
            MPI_Status status;
            MPI_Recv(global_best_path, n, MPI_INT,
                     MPI_ANY_SOURCE, 99, MPI_COMM_WORLD, &status);
        }
    } else {
        if (local_best_cost == global_best_cost) {
            // This worker has the best path — send to rank 0
            MPI_Send(local_best_path, n, MPI_INT, 0, 99, MPI_COMM_WORLD);
        }
    }

    // -------------------------------------------------------------------------
    // Timing summary (each process reports; rank 0 also writes the output file)
    // -------------------------------------------------------------------------
    double t_total = t_init + t_comp;

    // Print timing from every process so the user can see load balance
    for (int r = 0; r < nprocs; r++) {
        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == r) {
            printf("Process %d | Tinit: %.6f s | Tcomp: %.6f s | Ttotal: %.6f s\n",
                   rank, t_init, t_comp, t_total);
        }
    }

    // -------------------------------------------------------------------------
    // Rank 0 writes the result
    // -------------------------------------------------------------------------
    if (rank == 0) {
        printf("\n=== Results ===\n");
        printf("Optimal route:");
        for (i = 0; i < n; i++) {
            printf(" %d", global_best_path[i]);
            if (i < n - 1) printf(" ->");
        }
        printf("\nMinimum energy consumption: %d kWh\n", global_best_cost);

        fprintf(outfile, "Optimal route (minimum energy = %d kWh):\n", global_best_cost);
        for (i = 0; i < n; i++) {
            fprintf(outfile, "%d", global_best_path[i]);
            if (i < n - 1) fprintf(outfile, " -> ");
        }
        fprintf(outfile, "\n");
        fclose(outfile);
    }

    MPI_Finalize();
    return 0;
}
