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
#include <string.h>
#include <omp.h>
#include <limits.h>

#define MAX_N 10

// ============================================================================
// Global variables
// ============================================================================

int procs = 1;

int n;
int adj[MAX_N][MAX_N];

// Shared best solution — protected by a lock
int global_best_cost;
int global_best_path[MAX_N];
omp_lock_t best_lock;

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
// Branch-and-Bound recursive solver (called per-thread)
//
//  path[]        : current partial path  (1-indexed city numbers)
//  path_len      : number of cities visited so far
//  visited[]     : boolean array of visited cities (0-indexed)
//  current_cost  : accumulated energy cost so far
//  local_best_*  : thread-local best (tighter pruning within one thread)
// ============================================================================

void branch_and_bound(int *path, int path_len, int *visited,
                      int current_cost,
                      int *local_best_cost, int *local_best_path)
{
    // Base case: all cities visited — complete route found
    if (path_len == n) {
        if (current_cost < *local_best_cost) {
            *local_best_cost = current_cost;
            memcpy(local_best_path, path, n * sizeof(int));

            // Update the global best under the lock
            omp_set_lock(&best_lock);
            if (current_cost < global_best_cost) {
                global_best_cost = current_cost;
                memcpy(global_best_path, path, n * sizeof(int));
            }
            omp_unset_lock(&best_lock);
        }
        return;
    }

    int last_city = path[path_len - 1] - 1;  // convert to 0-indexed

    for (int next = 0; next < n; next++) {
        if (visited[next]) continue;

        int new_cost = current_cost + adj[last_city][next];

        // Read the global best without a lock for speed.
        // A slightly stale value causes at most some extra work — never a wrong answer.
        int cur_global = global_best_cost;

        if (new_cost >= cur_global)      continue;  // prune against global bound
        if (new_cost >= *local_best_cost) continue;  // prune against local  bound

        visited[next] = 1;
        path[path_len] = next + 1;  // store 1-indexed

        branch_and_bound(path, path_len + 1, visited, new_cost,
                         local_best_cost, local_best_path);

        visited[next] = 0;
    }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv)
{
    double t_init_start = gettime();

    int opt;
    int i, j;
    char *input_file  = NULL;
    char *output_file = NULL;
    FILE *infile  = NULL;
    FILE *outfile = NULL;
    int success_flag = 1;

    while ((opt = getopt(argc, argv, "p:i:o:h")) != -1)
    {
        switch (opt)
        {
            case 'p':
                procs = atoi(optarg);
                break;
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

    if (!success_flag) return 1;

    double t_init_end = gettime();
    double t_init = t_init_end - t_init_start;

    printf("Running with %d processes/threads on a graph with %d nodes\n", procs, n);

    // -------------------------------------------------------------------------
    // Set up OpenMP
    // -------------------------------------------------------------------------
    omp_set_num_threads(procs);
    omp_init_lock(&best_lock);

    global_best_cost = INT_MAX;
    memset(global_best_path, 0, sizeof(global_best_path));

    // -------------------------------------------------------------------------
    // Parallelisation strategy
    // -----------------------------------------------------------------
    // The route always starts at City 1 (index 0).
    // We expand the first level of the B&B tree — the (n-1) choices for
    // the second city — and distribute those sub-trees across threads
    // with a DYNAMIC schedule so that idle threads pull extra work as
    // soon as they finish their current sub-tree.
    //
    // Each thread owns private copies of path[], visited[], and a
    // thread-local best.  The shared global_best_cost is read (without
    // holding the lock) inside the recursion for fast pruning; it is
    // written (under the lock) only when a new global optimum is found.
    // -------------------------------------------------------------------------

    double t_comp_start = gettime();

    // First-level children: cities 1 .. n-1  (0-indexed)
    int num_children = n - 1;

    #pragma omp parallel for schedule(dynamic, 1) \
            shared(global_best_cost, global_best_path, adj, n)
    for (i = 0; i < num_children; i++)
    {
        // ---- Thread-private state ----
        int path[MAX_N];
        int visited[MAX_N];
        int local_best_cost = INT_MAX;
        int local_best_path[MAX_N];

        memset(visited,         0, sizeof(visited));
        memset(local_best_path, 0, sizeof(local_best_path));

        // Fix: City 1 is the start (0-indexed city 0)
        visited[0] = 1;
        path[0]    = 1;

        // First move: city 0 → children[i]  (children[i] == i+1 in 0-indexed)
        int next      = i + 1;
        int step_cost = adj[0][next];

        // Prune immediately if the very first edge already beats nothing
        if (step_cost >= global_best_cost) continue;

        visited[next] = 1;
        path[1]       = next + 1;  // 1-indexed for output

        branch_and_bound(path, 2, visited, step_cost,
                         &local_best_cost, local_best_path);
    }

    double t_comp_end = gettime();
    double t_comp  = t_comp_end  - t_comp_start;
    double t_total = t_init + t_comp;

    // -------------------------------------------------------------------------
    // Write output file
    // -------------------------------------------------------------------------
    fprintf(outfile, "Optimal route (minimum energy = %d kWh):\n", global_best_cost);
    for (i = 0; i < n; i++) {
        fprintf(outfile, "%d", global_best_path[i]);
        if (i < n - 1) fprintf(outfile, " -> ");
    }
    fprintf(outfile, "\n");
    fclose(outfile);

    // -------------------------------------------------------------------------
    // Console summary
    // -------------------------------------------------------------------------
    printf("\n=== Results ===\n");
    printf("Optimal route:");
    for (i = 0; i < n; i++) {
        printf(" %d", global_best_path[i]);
        if (i < n - 1) printf(" ->");
    }
    printf("\nMinimum energy consumption: %d kWh\n", global_best_cost);
    printf("\n=== Timing ===\n");
    printf("Initialisation time (Tinit) : %.6f s\n", t_init);
    printf("Computation time   (Tcomp) : %.6f s\n", t_comp);
    printf("Total time         (Ttotal): %.6f s\n", t_total);

    omp_destroy_lock(&best_lock);

    return 0;
}
