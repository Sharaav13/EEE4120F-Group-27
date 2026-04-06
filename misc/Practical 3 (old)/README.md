# Practical 3: Minimum Energy Consumption Freight Route Optimization

## Overview

Practical 3 focuses on optimizing freight routes to minimize energy consumption using parallel programming paradigms. The assignment is divided into two main parts:

1. **Part 1: OpenMP Implementation** - Shared-memory parallelization
2. **Part 2: MPI Implementation** - Distributed-memory parallelization

This practical demonstrates how to leverage parallel computing to solve combinatorial optimization problems efficiently.

---

## Problem Description

### Wariara Freights Route Optimization

The problem involves finding the optimal route for freight transportation between locations that minimizes total energy consumption. The solution considers:

- **Energy Matrix**: Pre-computed energy costs between location pairs.
- **Optimization Goal**: Find the route that minimizes total energy consumption.

#### Input Format

The input consists of an energy cost file (`energy<N>`):

**Energy File** (`energy<N>`): Contains the number of nodes followed by the energy costs.
```
n
e01
e02 e12
e03 e13 e23
...
```

Example (`energy4`):
```
4
 54 
 76  30 
 24  51  64 
```

#### Output Format

Results are written to an output file containing the optimal route information and total energy cost.

---

## Folder Structure

```
Practical-3/
├── OpenMP/
│   ├── Makefile                    # Build configuration for OpenMP version
│   ├── wariara_freights_route.c   # OpenMP implementation
│   ├── input/                      # Input datasets (energy4, energy5, ...)
│   └── output/                     # Output directory for results (energy4.txt, ...)
│
└── MPI/
    ├── Makefile                    # Build configuration for MPI version
    ├── wariara_freights_route.c   # MPI implementation
    ├── input/                      # Input datasets (energy4, energy5, ...)
    └── output/                     # Output directory for results (energy4.txt, ...)
```

---

## Part 1: OpenMP Implementation

### Overview

The OpenMP version uses **shared-memory parallelization**. Multiple threads work on the same memory space.

### Compilation

#### Using Makefile
```bash
cd OpenMP
make                # Compiles both optimized and debug versions
```

#### Manual Compilation (Without Makefile)
```bash
cd OpenMP
gcc -O3 -fopenmp wariara_freights_route.c -o wariara_freights_route
```

### Execution

#### Using Makefile
To pass arguments through `make`, use the `ARGS` variable:
```bash
make run ARGS="-p 4 -i input/energy4 -o output/energy4.txt"
```

#### Direct Execution (Without Makefile)
```bash
./wariara_freights_route -p 4 -i input/energy4 -o output/energy4.txt
```

**Arguments:**
- `-p <num>`: Number of threads to use
- `-i <file>`: Input energy file path
- `-o <file>`: Output file path
- `-h`: Display help message

---

## Part 2: MPI Implementation

### Overview

The MPI version uses **distributed-memory parallelization**. Each process has its own memory space and communicates via message passing.

### Compilation

#### Using Makefile
```bash
cd MPI
make                # Compiles both optimized and debug versions
```

#### Manual Compilation (Without Makefile)
```bash
cd MPI
mpicc -O3 wariara_freights_route.c -o wariara_freights_route
```

### Execution

#### Using Makefile
To pass arguments through `make`, use the `ARGS` variable. You can also specify the number of processes with `NP`:
```bash
make run NP=4 ARGS="-i input/energy4 -o output/energy4.txt"
```

#### Direct Execution (Without Makefile)
```bash
mpirun -np 4 ./wariara_freights_route -i input/energy4 -o output/energy4.txt
```

**Note:** Use `--use-hwthread-cpus` if you want to oversubscribe or leverage all hardware threads:
```bash
mpirun -np 12 --use-hwthread-cpus ./wariara_freights_route -i input/energy4 -o output/energy4.txt
```

---

## Input Datasets

| Dataset | Nodes | Files |
|---------|-------|-------|
| Test 1  | 4     | `energy4` |
| Test 2  | 5     | `energy5` |
| Test 3  | 6     | `energy6` |
| Test 4  | 7     | `energy7` |
| Test 5  | 8     | `energy8` |
| Test 6  | 9     | `energy9` |
| Test 7  | 10    | `energy10` |

---

## Performance Metrics

- **Speedup:** $S = T_{serial} / T_{parallel}$
- **Efficiency:** $E = S / P$ (where P = thread/process count)
- **Scalability:** Performance scaling with increased cores.

---

