% =========================================================================
% Practical 3 – Benchmark Results Plotter
% Wariara Freights Route Optimizer (OpenMP & MPI)
%
% HOW TO USE:
%   1. Run your OpenMP and MPI programs for p = 1, 2, 4, 8 (and any other
%      thread/process counts your machine supports).
%   2. Fill in the timing values you measured into the DATA INPUT section
%      below.  Every variable that starts with "omp_" is for the OpenMP
%      version; every variable that starts with "mpi_" is for MPI.
%   3. Run this script in MATLAB (R2019b or later recommended).
%      All figures are saved as high-resolution PNGs in the working
%      directory so you can drop them straight into your report.
%
% PLOTS PRODUCED:
%   Figure 1  – Execution time breakdown (Tinit / Tcomp) – OpenMP
%   Figure 2  – Execution time breakdown (Tinit / Tcomp) – MPI
%   Figure 3  – Total speedup:  OpenMP vs MPI vs ideal
%   Figure 4  – Computation speedup: OpenMP vs MPI vs ideal
%   Figure 5  – Efficiency (%):  OpenMP vs MPI
%   Figure 6  – OpenMP vs MPI total time (side-by-side bar)
% =========================================================================

clear; clc; close all;

% =========================================================================
%  DATA INPUT  ← fill these in with your measured values
% =========================================================================

% Number of threads / processes tested (must be the same list for both)
p = [1, 2, 4, 8];   % e.g. [1 2 4 8]  — edit as needed

% ----- OpenMP timing (seconds) -------------------------------------------
%   Replace the placeholder values with your actual measurements.
%   omp_tinit(k)  = initialisation time   with p(k) threads
%   omp_tcomp(k)  = computation time      with p(k) threads
omp_tinit  = [0.000120, 0.000135, 0.000148, 0.000162];
omp_tcomp  = [1.245000, 0.641000, 0.334000, 0.198000];

% ----- MPI timing (seconds) ----------------------------------------------
%   mpi_tinit(k)  = initialisation time   with p(k) processes
%   mpi_tcomp(k)  = computation time      with p(k) processes
mpi_tinit  = [0.002100, 0.002350, 0.002600, 0.002900];
mpi_tcomp  = [1.245000, 0.660000, 0.355000, 0.220000];

% =========================================================================
%  DERIVED QUANTITIES  (no editing needed below this line)
% =========================================================================

omp_ttotal = omp_tinit + omp_tcomp;
mpi_ttotal = mpi_tinit + mpi_tcomp;

% Speedup  S(p) = T(1) / T(p)
omp_speedup_total = omp_ttotal(1) ./ omp_ttotal;
mpi_speedup_total = mpi_ttotal(1) ./ mpi_ttotal;

omp_speedup_comp  = omp_tcomp(1)  ./ omp_tcomp;
mpi_speedup_comp  = mpi_tcomp(1)  ./ mpi_tcomp;

% Efficiency  E(p) = S(p) / p  × 100 %
omp_efficiency = (omp_speedup_total ./ p) * 100;
mpi_efficiency = (mpi_speedup_total ./ p) * 100;

ideal_speedup = p;   % perfect linear speedup reference

% =========================================================================
%  COLOUR PALETTE  (UCT-inspired; easy to distinguish in greyscale too)
% =========================================================================
c_omp   = [0.00, 0.45, 0.70];   % blue
c_mpi   = [0.84, 0.37, 0.00];   % orange
c_ideal = [0.47, 0.67, 0.19];   % green (dashed)
c_init  = [0.60, 0.60, 0.60];   % grey  (Tinit portion)

font_title  = 13;
font_axis   = 11;
font_legend = 10;
fig_size    = [0, 0, 720, 440];   % [x y width height] in pixels

% =========================================================================
%  FIGURE 1 – OpenMP Execution Time Breakdown (stacked bar)
% =========================================================================
fig1 = figure('Name','OpenMP Time Breakdown','Position',fig_size);

bar_data = [omp_tinit', omp_tcomp'];
b = bar(p, bar_data, 'stacked');
b(1).FaceColor = c_init;
b(2).FaceColor = c_omp;

xlabel('Number of Threads (p)', 'FontSize', font_axis);
ylabel('Time (s)',               'FontSize', font_axis);
title('OpenMP – Execution Time Breakdown (T_{init} + T_{comp})', ...
      'FontSize', font_title);
legend({'T_{init}','T_{comp}'}, 'Location','northeast', ...
       'FontSize', font_legend);
set(gca, 'XTick', p, 'FontSize', font_axis);
grid on;
saveas(fig1, 'fig1_openmp_time_breakdown.png');

% =========================================================================
%  FIGURE 2 – MPI Execution Time Breakdown (stacked bar)
% =========================================================================
fig2 = figure('Name','MPI Time Breakdown','Position',fig_size);

bar_data2 = [mpi_tinit', mpi_tcomp'];
b2 = bar(p, bar_data2, 'stacked');
b2(1).FaceColor = c_init;
b2(2).FaceColor = c_mpi;

xlabel('Number of Processes (p)', 'FontSize', font_axis);
ylabel('Time (s)',                 'FontSize', font_axis);
title('MPI – Execution Time Breakdown (T_{init} + T_{comp})', ...
      'FontSize', font_title);
legend({'T_{init}','T_{comp}'}, 'Location','northeast', ...
       'FontSize', font_legend);
set(gca, 'XTick', p, 'FontSize', font_axis);
grid on;
saveas(fig2, 'fig2_mpi_time_breakdown.png');

% =========================================================================
%  FIGURE 3 – Total Speedup: OpenMP vs MPI vs Ideal
% =========================================================================
fig3 = figure('Name','Total Speedup','Position',fig_size);

plot(p, ideal_speedup,        '--', 'Color', c_ideal, ...
     'LineWidth', 1.8, 'DisplayName','Ideal (linear)');
hold on;
plot(p, omp_speedup_total, '-o', 'Color', c_omp, ...
     'LineWidth', 2.0, 'MarkerSize', 7, 'MarkerFaceColor', c_omp, ...
     'DisplayName','OpenMP');
plot(p, mpi_speedup_total, '-s', 'Color', c_mpi, ...
     'LineWidth', 2.0, 'MarkerSize', 7, 'MarkerFaceColor', c_mpi, ...
     'DisplayName','MPI');

xlabel('Number of Threads / Processes (p)', 'FontSize', font_axis);
ylabel('Speedup  S_p = T_1 / T_p',          'FontSize', font_axis);
title('Total Speedup – OpenMP vs MPI',       'FontSize', font_title);
legend('Location','northwest', 'FontSize', font_legend);
set(gca, 'XTick', p, 'FontSize', font_axis);
xlim([min(p)-0.2, max(p)+0.2]);
ylim([0, max(p) * 1.15]);
grid on;
saveas(fig3, 'fig3_total_speedup.png');

% =========================================================================
%  FIGURE 4 – Computation Speedup: OpenMP vs MPI vs Ideal
% =========================================================================
fig4 = figure('Name','Computation Speedup','Position',fig_size);

plot(p, ideal_speedup,       '--', 'Color', c_ideal, ...
     'LineWidth', 1.8, 'DisplayName','Ideal (linear)');
hold on;
plot(p, omp_speedup_comp, '-o', 'Color', c_omp, ...
     'LineWidth', 2.0, 'MarkerSize', 7, 'MarkerFaceColor', c_omp, ...
     'DisplayName','OpenMP');
plot(p, mpi_speedup_comp, '-s', 'Color', c_mpi, ...
     'LineWidth', 2.0, 'MarkerSize', 7, 'MarkerFaceColor', c_mpi, ...
     'DisplayName','MPI');

xlabel('Number of Threads / Processes (p)', 'FontSize', font_axis);
ylabel('Computation Speedup  S_{comp}',      'FontSize', font_axis);
title('Computation Speedup – OpenMP vs MPI', 'FontSize', font_title);
legend('Location','northwest', 'FontSize', font_legend);
set(gca, 'XTick', p, 'FontSize', font_axis);
xlim([min(p)-0.2, max(p)+0.2]);
ylim([0, max(p) * 1.15]);
grid on;
saveas(fig4, 'fig4_computation_speedup.png');

% =========================================================================
%  FIGURE 5 – Parallel Efficiency: OpenMP vs MPI
% =========================================================================
fig5 = figure('Name','Parallel Efficiency','Position',fig_size);

yline(100, '--', 'Color', c_ideal, 'LineWidth', 1.8, ...
      'DisplayName','Ideal (100%)');
hold on;
plot(p, omp_efficiency, '-o', 'Color', c_omp, ...
     'LineWidth', 2.0, 'MarkerSize', 7, 'MarkerFaceColor', c_omp, ...
     'DisplayName','OpenMP');
plot(p, mpi_efficiency, '-s', 'Color', c_mpi, ...
     'LineWidth', 2.0, 'MarkerSize', 7, 'MarkerFaceColor', c_mpi, ...
     'DisplayName','MPI');

xlabel('Number of Threads / Processes (p)', 'FontSize', font_axis);
ylabel('Efficiency  E_p = S_p / p  (%)',    'FontSize', font_axis);
title('Parallel Efficiency – OpenMP vs MPI','FontSize', font_title);
legend('Location','northeast', 'FontSize', font_legend);
set(gca, 'XTick', p, 'FontSize', font_axis);
xlim([min(p)-0.2, max(p)+0.2]);
ylim([0, 120]);
grid on;
saveas(fig5, 'fig5_efficiency.png');

% =========================================================================
%  FIGURE 6 – OpenMP vs MPI Total Time (grouped bar)
% =========================================================================
fig6 = figure('Name','OpenMP vs MPI Total Time','Position',fig_size);

bar_data6 = [omp_ttotal', mpi_ttotal'];
b6 = bar(p, bar_data6, 'grouped');
b6(1).FaceColor = c_omp;
b6(2).FaceColor = c_mpi;

xlabel('Number of Threads / Processes (p)', 'FontSize', font_axis);
ylabel('Total Time  T_{total}  (s)',         'FontSize', font_axis);
title('Total Execution Time – OpenMP vs MPI','FontSize', font_title);
legend({'OpenMP','MPI'}, 'Location','northeast', 'FontSize', font_legend);
set(gca, 'XTick', p, 'FontSize', font_axis);
grid on;
saveas(fig6, 'fig6_openmp_vs_mpi_total_time.png');

% =========================================================================
%  CONSOLE SUMMARY TABLE
% =========================================================================
fprintf('\n========================================================\n');
fprintf('  Benchmark Summary\n');
fprintf('========================================================\n');
fprintf('%-6s | %-12s %-12s | %-12s %-12s\n', ...
        'p', 'OMP Ttotal','OMP Spdp','MPI Ttotal','MPI Spdp');
fprintf('-------+-------------------------+------------------------\n');
for k = 1:numel(p)
    fprintf('%-6d | %-12.6f %-12.4f | %-12.6f %-12.4f\n', ...
        p(k), omp_ttotal(k), omp_speedup_total(k), ...
              mpi_ttotal(k), mpi_speedup_total(k));
end
fprintf('========================================================\n\n');

fprintf('Computation speedup:\n');
fprintf('%-6s | %-14s | %-14s\n','p','OMP Scomp','MPI Scomp');
fprintf('-------+----------------+----------------\n');
for k = 1:numel(p)
    fprintf('%-6d | %-14.4f | %-14.4f\n', ...
        p(k), omp_speedup_comp(k), mpi_speedup_comp(k));
end
fprintf('\n');

fprintf('Efficiency:\n');
fprintf('%-6s | %-14s | %-14s\n','p','OMP Eff (%)','MPI Eff (%)');
fprintf('-------+----------------+----------------\n');
for k = 1:numel(p)
    fprintf('%-6d | %-14.2f | %-14.2f\n', ...
        p(k), omp_efficiency(k), mpi_efficiency(k));
end
fprintf('\n');

fprintf('All figures saved as PNG in the current working directory.\n');
