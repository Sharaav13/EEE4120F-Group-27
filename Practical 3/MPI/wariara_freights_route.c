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
#include <limits.h>
#include "mpi.h"

#define MAX_N 10

// ============================================================================
// Global variables
// ============================================================================

int n; // If this is -1, it signals an error/exit
int adj[MAX_N][MAX_N];

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

// ============================================================================
// Greedy nearest-neighbour heuristic – provides a tight initial upper bound
// so that B&B prunes aggressively from the very first branch.
// Each process runs this independently (no communication needed).
// ============================================================================

int greedy_bound(int *out_path)
{
    int visited[MAX_N] = {0};
    out_path[0] = 0;
    visited[0]  = 1;
    int total   = 0;

    for (int step = 1; step < n; step++) {
        int prev = out_path[step - 1];
        int best_next = -1, best_edge = INT_MAX;
        for (int j = 0; j < n; j++) {
            if (!visited[j] && adj[prev][j] < best_edge) {
                best_edge = adj[prev][j];
                best_next = j;
            }
        }
        visited[best_next] = 1;
        out_path[step] = best_next;
        total += best_edge;
    }
    return total;
}

// ============================================================================
// Process-local B&B state  (each MPI process has its own private copy)
//
// local_best_cost / local_best_path: ALWAYS a consistent pair – updated
//   together only when THIS process finds a better complete route.
//   Used for result collection at the end.
//
// prune_bound: the global minimum cost known so far, tightened after every
//   branch via MPI_Allreduce.  Used for pruning so that discoveries on other
//   processes benefit all processes.  Updated independently of local_best_path
//   so the cost/path pair always remains consistent.
// ============================================================================

int local_best_cost;
int local_best_path[MAX_N];
int prune_bound;

// ============================================================================
// Recursive Branch-and-Bound kernel.
// Entirely local – no MPI calls inside; communication happens between
// top-level branches in main() to keep the implementation simple and clean.
// ============================================================================

void branch_and_bound(int *path, int path_len, int *visited, int current_cost)
{
    /* Base case: all cities visited → complete route */
    if (path_len == n) {
        if (current_cost < local_best_cost) {
            local_best_cost = current_cost;
            memcpy(local_best_path, path, n * sizeof(int));
        }
        return;
    }

    int prev = path[path_len - 1];

    for (int next = 0; next < n; next++) {
        if (visited[next]) continue;

        int new_cost = current_cost + adj[prev][next];

        if (new_cost >= prune_bound) continue;   /* prune using global bound */

        visited[next]  = 1;
        path[path_len] = next;

        branch_and_bound(path, path_len + 1, visited, new_cost);

        visited[next] = 0;   /* backtrack */
    }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv)
{
    /* Start Tinit immediately – covers MPI_Init, arg parse, file I/O,
       broadcasts, and greedy initialisation.                              */
    double t_init_start = gettime();

    int rank, nprocs;
    int opt;
    int i, j;
    char *input_file  = NULL;
    char *output_file = NULL;
    FILE *infile      = NULL;
    FILE *outfile     = NULL;
    int   success_flag = 1; // 1 = good, 0 = error/help encountered

    // Initialize MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);


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


    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    
    if (n == -1) {
        MPI_Finalize();
        return 0; 
    }

    MPI_Bcast(&adj[0][0], MAX_N * MAX_N, MPI_INT, 0, MPI_COMM_WORLD);

    // =========================================================================
    // TODO: compute solution to minimum energy consumption problem here
    //       and write to output file.
    //       Be careful on which process rank writes to the output file!
    // =========================================================================

    /*
     * DECOMPOSITION STRATEGY
     * ----------------------
     * The search tree is rooted at City 0 (City 1 in 1-indexed terms).
     * There are (n-1) first-level branches – one per possible second city.
     * We distribute these branches across processes using static striping:
     *
     *   Process r handles branches:  first = r+1, r+1+nprocs, r+1+2*nprocs, ...
     *
     * Each process runs a fully sequential DFS/B&B on its assigned subtrees
     * using only process-local state (local_best_cost, local_best_path).
     *
     * BOUND SHARING (cross-process pruning)
     * ---------------------------------------
     * After finishing each top-level branch, every process participates in
     * an MPI_Allreduce(MPI_MIN) to share the current best cost globally.
     * This lets processes prune using bounds discovered by other processes,
     * improving efficiency without requiring communication inside the
     * recursive kernel.
     *
     * RESULT COLLECTION
     * -----------------
     * After all branches are processed, a final MPI_Allreduce finds the
     * global minimum cost.  MPI_MINLOC on an {int cost, int rank} pair
     * identifies which process holds the winning path.  That process
     * broadcasts the path to rank 0, which writes the output file.
     *
     * TIMING
     * ------
     * Tinit = t_init_start  →  t_comp_start   (setup + broadcasts + greedy)
     * Tcomp = t_comp_start  →  t_comp_end     (parallel B&B only)
     */

    /* Each process seeds its local best from the greedy heuristic */
    local_best_cost = greedy_bound(local_best_path);

    /* Share the tightest greedy bound across all processes so pruning
       is as aggressive as possible before the first branch starts.
       Only prune_bound is updated here – local_best_cost/path remain
       a consistent pair representing what THIS process actually found. */
    prune_bound = local_best_cost;
    MPI_Allreduce(MPI_IN_PLACE, &prune_bound, 1, MPI_INT,
                  MPI_MIN, MPI_COMM_WORLD);

    /* ------------------------------------------------------------------ */
    double t_comp_start = gettime();
    double Tinit = t_comp_start - t_init_start;
    /* ------------------------------------------------------------------ */

    /*
     * Main parallel loop: each process works on its stripe of first-level
     * branches.  Between branches, all processes synchronise their best
     * bound via MPI_Allreduce so that new discoveries prune future work.
     */
    for (int first = 1; first < n; first++)
    {
        /* Static stripe: this process owns branches where
           (first - 1) % nprocs == rank                      */
        if ((first - 1) % nprocs == rank)
        {
            int path[MAX_N];
            int visited[MAX_N];
            memset(visited, 0, n * sizeof(int));

            path[0]        = 0;
            path[1]        = first;
            visited[0]     = 1;
            visited[first] = 1;

            int cost = adj[0][first];

            if (cost < prune_bound) {
                branch_and_bound(path, 2, visited, cost);
                /* If B&B found a better route, tighten our prune_bound too
                   so the allreduce shares the tightest possible value */
                if (local_best_cost < prune_bound)
                    prune_bound = local_best_cost;
            }
        }

        /*
         * After every branch, share the tightest cost found so far globally.
         * Only prune_bound is updated here – local_best_cost and local_best_path
         * remain a consistent pair representing only what THIS process found.
         * This is the fix for the path corruption bug: previously local_best_cost
         * was overwritten by allreduce, causing it to drift out of sync with
         * local_best_path and producing wrong paths at result collection.
         */
        MPI_Allreduce(MPI_IN_PLACE, &prune_bound, 1, MPI_INT,
                      MPI_MIN, MPI_COMM_WORLD);
    }

    /* ------------------------------------------------------------------ */
    double t_comp_end = gettime();
    double Tcomp = t_comp_end - t_comp_start;
    /* ------------------------------------------------------------------ */

    /*
     * RESULT COLLECTION
     * Each process reports local_best_cost, which is ALWAYS a consistent pair
     * with local_best_path (only updated when THIS process finds a better route,
     * never overwritten by allreduce).  MPI_MINLOC finds the winning rank and
     * that process broadcasts its path.
     */
    int local_pair[2]  = { local_best_cost, rank };
    int global_pair[2] = { 0, 0 };

    MPI_Allreduce(local_pair, global_pair, 1, MPI_2INT,
                  MPI_MINLOC, MPI_COMM_WORLD);

    int winner_rank = global_pair[1];
    int global_best = global_pair[0];

    /* The winning process broadcasts its path to all (including rank 0) */
    int final_path[MAX_N];
    if (rank == winner_rank)
        memcpy(final_path, local_best_path, n * sizeof(int));

    MPI_Bcast(final_path, n, MPI_INT, winner_rank, MPI_COMM_WORLD);

    /* ---- Only rank 0 writes the output file and prints the summary ---- */
    if (rank == 0) {
        for (i = 0; i < n; i++) {
            fprintf(outfile, "%d", final_path[i] + 1);
            if (i < n - 1) fprintf(outfile, " ");
        }
        fprintf(outfile, "\n");
        fprintf(outfile, "Minimum energy cost: %d kWh\n", global_best);
        fclose(outfile);

        printf("Running with %d MPI processes on a graph with %d nodes\n",
               nprocs, n);
        printf("Optimal route  : ");
        for (i = 0; i < n; i++) printf("%d ", final_path[i] + 1);
        printf("\nMinimum energy : %d kWh\n", global_best);
        printf("Tinit          : %.6f s\n", Tinit);
        printf("Tcomp          : %.6f s\n", Tcomp);
        printf("Ttotal         : %.6f s\n", Tinit + Tcomp);
    }

    MPI_Finalize();
    return 0;
}
