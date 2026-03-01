# Practical 2: Mandelbrot Set Serial vs Parallel Analysis

## Overview

Practical 2 focuses on understanding parallel computing by implementing the Mandelbrot set computation using both serial (single-threaded) and parallel (multi-threaded) approaches in MATLAB. You will learn how to leverage multiple CPU cores to accelerate computationally intensive tasks and analyze performance improvements through speedup measurements.

---
## Problem Description

### The Mandelbrot Set

The Mandelbrot set is a famous fractal defined in the complex plane. For a complex number $c = a + bi$, we iterate the recurrence relation:

$$z_{n+1} = z_n^2 + c$$

starting with $z_0 = 0$. A complex number $c$ is in the Mandelbrot set if the sequence remains bounded (doesn't diverge to infinity).



---

## Folder Structure

```
Practical-2/
├── README.md                    # This file
├── run_analysis.m              # Main MATLAB script with TODO sections
└── (output files generated during execution)
```
---
## Running the Practical

### Prerequisites

- MATLAB R2020a or later
- **Parallel Computing Toolbox**
- Modern multi-core CPU (2+ cores for parallel speedup)

### Checking Parallel Computing Toolbox

```matlab
% Check if Parallel Computing Toolbox is available
if license('test', 'Parallel_Computing_Toolbox')
    disp('✓ Parallel Computing Toolbox is available');
else
    disp('✗ Parallel Computing Toolbox not available - parfor will run serially');
end

% Get number of workers available
pool = gcp('nocreate');
if isempty(pool)
    disp('No parallel pool active');
else
    disp(['Parallel pool has ', num2str(pool.NumWorkers), ' workers']);
end
```

### Execution Steps

1. **Open MATLAB:**
   ```bash
   cd Practical-2
   matlab
   ```

2. **Implement functions:**
   - Complete `mandelbrot_serial()` function
   - Complete `mandelbrot_parallel()` function
   - Complete `mandelbrot_plot()` function
   - Complete `run_analysis()` function

3. **Run the analysis:**
   ```matlab
   run_analysis()
   ```

4. **View results:**
   - Console output with timing comparisons
   - Generated PNG images of Mandelbrot sets
   - Performance analysis plots

### MATLAB Script Structure

```matlab
%% PART 1: Plotting
function mandelbrot_plot(varargin)
    % YOUR IMPLEMENTATION HERE
end

%% PART 2: Serial Computation
function iterations = mandelbrot_serial(varargin)
    % YOUR IMPLEMENTATION HERE
end

%% PART 3: Parallel Computation
function iterations = mandelbrot_parallel(varargin)
    % YOUR IMPLEMENTATION HERE
end

%% PART 4: Analysis
function run_analysis()
    % YOUR IMPLEMENTATION HERE
end
```





---
## Implementation Requirements

### Part 1: Mandelbrot Set Image Plotting and Saving

**Requirements:**
- ✓ Convert iteration counts to colormap
- ✓ Use appropriate color scheme (parula, hot, jet, etc.)
- ✓ Save high-quality image
- ✓ Include title and color bar

### Part 2: Serial Mandelbrot Set Computation


**Requirements:**
- ✓ Use nested loops to iterate through pixel grid
- ✓ Map pixel coordinates to complex plane
- ✓ Implement Mandelbrot iteration logic
- ✓ Return iteration count matrix
- ✓ No parallelization for this version

### Part 3: Parallel Mandelbrot Set Computation


**Requirements:**
- ✓ Parallelise the serial implementation
- ✓ Return iteration count matrix
- ✓ Requires Parallel Computing Toolbox

### Part 4: Testing and Analysis

- For each image:
  - ✓ Run both serial and parallel implementations multiple times for accuracy
  - ✓ Measure execution time using `tic` and `toc`
  - ✓ Compute speedup: 
  - ✓ Verify correctness by comparing outputs
  - ✓ Store results (image size, times, speedup,image visualisations)

## Performance Metrics

### Speedup

Speedup measures how much faster the parallel version runs:

$$\text{Speedup} = \frac{T_{\text{serial}}}{T_{\text{parallel}}}$$

**Interpretation:**
- Speedup = 1: No improvement
- Speedup = 2: Twice as fast (on 2-core system)
- Speedup = N: Near-ideal scaling (on N-core system)

### Efficiency

Efficiency measures how well cores are utilized:

$$\text{Efficiency} = \frac{\text{Speedup}}{N_{\text{cores}}} \times 100\%$$

**Interpretation:**
- Efficiency = 100%: Perfect scaling
- Efficiency = 50%: Each core utilized 50% of potential
- Typical realistic efficiency: 70-90% (overhead from parallelization)

### Amdahl's Law

Theoretical maximum speedup with P processors:

$$\text{Speedup} = \frac{1}{(1-f) + f/P}$$

Where $f$ is the fraction of parallelizable code.


## References

### MATLAB Functions Reference

- `parfor` - Parallel for loop
- `gcp()` - Get current parallel pool
- `parpool()` - Create parallel pool
- `tic/toc` - Timing measurement
- `timeit()` - Precise function timing
- `imagesc()` - Display image scaled
- `colormap()` - Set color palette
- `saveas()` - Save figure to file

### Mathematical Resources

- [Mandelbrot Set Wikipedia](https://en.wikipedia.org/wiki/Mandelbrot_set)
- [Fractal Geometry](https://en.wikipedia.org/wiki/Fractal_geometry)
- [Complex Numbers](https://en.wikipedia.org/wiki/Complex_number)

### Parallel Computing Resources

- [MATLAB Parallel Computing Toolbox Documentation](https://www.mathworks.com/help/parallel/)
- [MATLAB Parallel Programming Guide](https://www.mathworks.com/help/parallel/parallel-computing.html)
- [Amdahl's Law](https://en.wikipedia.org/wiki/Amdahl%27s_law)


## Version History

- **Initial Release**: Practical 2 2026
- **Last Updated**: February 15th 2026

---

## Contact & Support

For questions about this practical, refer to:
- Course syllabus and lecture notes
- TA/Tutors during lab sessions
- Practical assignment specification document
