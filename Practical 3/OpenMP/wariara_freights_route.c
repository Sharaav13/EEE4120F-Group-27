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
//  PART 1: Minimum Energy Consumption Freight Route Optimization using OpenMP
// =========================================================================


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <limits.h>
#include <string.h>
#include <omp.h>

#define MAX_N 10

// ============================================================================
// Global variables
// ============================================================================

int procs = 1;

int n;
int adj[MAX_N][MAX_N];

// ============================================================================
// Thread-local B&B state
// Declared as threadprivate so every thread gets its own independent copy,
// matching exactly the process-local variables used in the MPI version.
// ============================================================================

int local_best_cost;
int local_best_path[MAX_N];

#pragma omp threadprivate(local_best_cost, local_best_path)

// ============================================================================
// Shared result – written only during the final reduction (critical section)
// ============================================================================

int best_cost;
int best_path[MAX_N];

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
  printf("Usage: %s [options]\n", program);
  printf("-p <num>\tNumber of processors/threads to use\n");
  printf("-i <file>\tInput file name\n");
  printf("-o <file>\tOutput file name\n");
  printf("-h \t\tDisplay this help\n");
}

// ============================================================================
// Greedy nearest-neighbour heuristic – gives a tight initial upper bound so
// that B&B prunes aggressively right from the very first branch.
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
// Recursive Branch-and-Bound kernel.
//
// This function is IDENTICAL to the MPI version:
//   - Reads and writes only thread-local state (local_best_cost,
//     local_best_path) – no locks, no shared variable access.
//   - Signature, base case, pruning check, and backtracking are the same.
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

        if (new_cost >= local_best_cost) continue;   /* prune */

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
    /* ------------------------------------------------------------------ */
    /*  Tinit starts here – covers arg parsing, file I/O, thread setup     */
    /* ------------------------------------------------------------------ */
    double t_init_start = gettime();

    int opt;
    int i, j;
    char *input_file  = NULL;
    char *output_file = NULL;
    FILE *infile      = NULL;
    FILE *outfile     = NULL;
    int   success_flag = 1;

    while ((opt = getopt(argc, argv, "p:i:o:h")) != -1)
    {
        switch (opt)
        {
            case 'p':
            {
                procs = atoi(optarg);
                break;
            }

            case 'i':
            {
                input_file = optarg;
                break;
            }

            case 'o':
            {
                output_file = optarg;
                break;
            }

            case 'h':
            {
                Usage(argv[0]);
                success_flag = 0;
                break;
            }

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

    if (!success_flag) return 1;

    printf("Running with %d processes/threads on a graph with %d nodes\n", procs, n);

    /* Tell OpenMP how many threads to use */
    omp_set_num_threads(procs);

    /* Seed best_cost with a greedy solution before launching threads */
    best_cost = greedy_bound(best_path);

    double t_init_end = gettime();
    double Tinit = t_init_end - t_init_start;

    /* ------------------------------------------------------------------ */
    /*  Tcomp: parallel Branch-and-Bound                                   */
    /* ------------------------------------------------------------------ */
    double t_comp_start = gettime();

    /*
     * DECOMPOSITION STRATEGY
     * ----------------------
     * The root is always city 0 (City 1, 1-indexed). We distribute the
     * (n-1) first-level branches across threads using dynamic scheduling,
     * mirroring the static stripe used in the MPI version but with better
     * load balancing on a shared-memory machine.
     *
     * THREAD-LOCAL STATE
     * ------------------
     * Each thread initialises its own local_best_cost / local_best_path
     * (threadprivate globals) from the shared greedy bound.  The B&B
     * kernel reads and writes only these thread-local variables – no
     * locks needed inside the recursion.
     *
     * RESULT REDUCTION
     * ----------------
     * After the parallel for (implicit barrier), each thread compares its
     * local_best_cost against the shared best_cost inside a critical
     * section, matching the MPI_Allreduce + MPI_MINLOC pattern used in
     * the MPI version.
     *
     * SYNCHRONISATION POINTS
     * ----------------------
     * 1. Implicit barrier at the end of #pragma omp for
     * 2. #pragma omp critical for the final merge into best_cost/best_path
     */

    #pragma omp parallel default(none) shared(best_cost, best_path, adj, n)
    {
        /* Initialise this thread's local bound from the shared greedy result */
        local_best_cost = best_cost;
        memcpy(local_best_path, best_path, n * sizeof(int));

        /* Distribute first-level branches across threads */
        #pragma omp for schedule(dynamic, 1)
        for (int first = 1; first < n; first++)
        {
            int path[MAX_N];
            int visited[MAX_N];
            memset(visited, 0, n * sizeof(int));

            path[0]        = 0;
            path[1]        = first;
            visited[0]     = 1;
            visited[first] = 1;

            int cost = adj[0][first];

            if (cost < local_best_cost) {
                branch_and_bound(path, 2, visited, cost);
            }
        }
        /* Implicit barrier: all threads finished their branches */

        /* Reduce thread-local results into the shared global best */
        #pragma omp critical
        {
            if (local_best_cost < best_cost) {
                best_cost = local_best_cost;
                memcpy(best_path, local_best_path, n * sizeof(int));
            }
        }
    }

    double t_comp_end = gettime();
    double Tcomp = t_comp_end - t_comp_start;

    /* ------------------------------------------------------------------ */
    /*  Output                                                              */
    /* ------------------------------------------------------------------ */

    for (i = 0; i < n; i++) {
        fprintf(outfile, "%d", best_path[i] + 1);
        if (i < n - 1) fprintf(outfile, " ");
    }
    fprintf(outfile, "\n");
    fprintf(outfile, "Minimum energy cost: %d kWh\n", best_cost);
    fclose(outfile);

    printf("Optimal route  : ");
    for (i = 0; i < n; i++) printf("%d ", best_path[i] + 1);
    printf("\nMinimum energy : %d kWh\n", best_cost);
    printf("Tinit          : %.6f s\n", Tinit);
    printf("Tcomp          : %.6f s\n", Tcomp);
    printf("Ttotal         : %.6f s\n", Tinit + Tcomp);

    return 0;
}
