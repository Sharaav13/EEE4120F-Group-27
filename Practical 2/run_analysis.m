% =========================================================================
% Practical 2: Mandelbrot-Set Serial vs Parallel Analysis
% =========================================================================
%
% GROUP NUMBER: 27
%
% MEMBERS:
%   - Member 1 Max Mendelow, MNDMAX003
%   - Member 2 Sharaav Dhebideen, DHBSHA001

%% ========================================================================
%  PART 1: Mandelbrot Set Image Plotting and Saving
%  ========================================================================
%
% TODO: Implement Mandelbrot set plotting and saving function
function mandelbrot_plot(iter_matrix, max_iter, res_name, compute_type) %Add necessary input arguments
      
    % Show the image
    figure;
    imagesc(iter_matrix, [0 max_iter]);
    colormap(parula);
    colorbar;
    axis image;
    axis off;
    figname = res_name + " Mandelbrot Image (" + compute_type + ")";
    title(figname,'FontSize',14)
    
    % Save the image
    save_name = "mandelbrot_" + res_name + "_" + compute_type + ".png";
    saveas(gcf, save_name); 
end

%% ========================================================================
%  PART 2: Serial Mandelbrot Set Computation
%  ========================================================================`
%
%TODO: Implement serial Mandelbrot set computation function
function iter_matrix = mandelbrot_serial(max_iter, img_size) %Add necessary input arguments
    
    % Extract the image dimensions
    n_x = img_size(1);
    n_y = img_size(2);
    empty_matrix = zeros(n_x,n_y);
   
    % Using the defined standard region of the Mandelbrot set
    x_vals = linspace(-2,0.5,n_x);
    y_vals = linspace(-1.2,1.2,n_y);

    % Perform Mandelbrot Algorithm
    for x = 1:n_x
        %x_0 = 2.5*((x-1)/(n_x-1)) - 2;
        x_0 = x_vals(x);
        for y = 1:n_y
            %y_0 = 2.4*((y-1)/(n_y-1)) - 1.2;
            y_0 = y_vals(y);
            
            % Initialise variables for performing iterations
            x_i = 0; 
            y_i = 0;
            iter = 0;

            % Evaluation of Mandelbrot condition
            while ((iter < max_iter) && ((x_i^2 + y_i^2) <= 4))
                temp = x_i^2 - y_i^2;
                y_i = 2*x_i*y_i + y_0;
                x_i = temp + x_0;
                iter = iter + 1;
            end
          
            empty_matrix (x,y) = iter;  
        end
    end

    iter_matrix = empty_matrix;
end

%% ========================================================================
%  PART 3: Parallel Mandelbrot Set Computation
%  ========================================================================
%
%TODO: Implement parallel Mandelbrot set computation function
function iter_matrix = mandelbrot_parallel(max_iter, img_size, workers) %Add necessary input arguments 
    
    % Extract the image dimensions
    n_x = img_size(1);
    n_y = img_size(2);
    empty_matrix = zeros(n_x,n_y);

    % Using the defined standard region of the Mandelbrot set
    x_vals = linspace(-2,0.5,n_x);
    y_vals = linspace(-1.2,1.2,n_y);

    % Configure parallel computing 
    p = gcp('nocreate');    % Check if their is an existing pool
    if (isempty(p)) || (p.NumWorkers ~= workers)
        if ~isempty(p); delete(p); end
        p = parpool(workers);
    end

    % Perform Mandelbrot Algorithm
    parfor x = 1:n_x % Parallelized outer loop
        %x_0 = 2.5*((x-1)/(n_x-1)) - 2;
        x_0 = x_vals(x);
        for y = 1:n_y
            %y_0 = 2.4*((y-1)/(n_y-1)) - 1.2;
            y_0 = y_vals(y);

            % Initialise variables for performing iterations
            x_i = 0; 
            y_i = 0;
            iter = 0;

            % Evaluation of Mandelbrot condition
            while ((iter < max_iter) && ((x_i^2 + y_i^2) <= 4))
                temp = x_i^2 - y_i^2;
                y_i = 2*x_i*y_i + y_0;
                x_i = temp + x_0;
                iter = iter + 1;
            end
          
            empty_matrix (x,y) = iter;  
        end
    end

    iter_matrix = empty_matrix;
end

%% ========================================================================
%  PART 4: Testing and Analysis
%  ========================================================================
% Compare the performance of serial Mandelbrot set computation
% with parallel Mandelbrot set computation.

function run_analysis()
    %Array conatining all the image sizes to be tested
    image_sizes = [
        [800,600],   %SVGA
        [1280,720],  %HD
        [1920,1080], %Full HD
        [2048,1080], %2K Cinema
        [2560,1440], %2K QHD
        [3840,2160], %4K UHD
        [5120,2880], %5K
        [7680,4320]  %8K UHD
    ]
    
    max_iterations = 1000; 
    
    %TODO: For each image size, perform the following:
    %   a. Measure execution time of mandelbrot_serial
    %   b. Measure execution time of mandelbrot_parallel
    %   c. Store results (image size, time_serial, time_parallel, speedup) 
    %   d. Plot and save the Mandelbrot set images generated by both methods
    
    image_names = {"SVGA","HD","Full HD","2K Cinema","2K QHD","4K UHD","5K","8K UHD"};
    
    repetitions = 4; % Number of times to repeat a specific benchmark test
    workers = 2; % Number of workers for parallel computing
    
    % Arrays for storing results
    time_serial = zeros(8);
    time_parallel = zeros(8);
    speedup = time_zeros(8);
    efficiency = zeros(8);
    
    n = feature('numcores');

    sum_time_serial = 0;
    sum_time_parallel = 0;

    for i = 1:8 % Test for all 8 image sizes
        
        for r = 1:repetitions
            % Determining Serial Mandelbrot function's execution time 
            start_time_serial = tic; % start timer
            iteration_matrix_serial = mandelbrot_serial(max_iterations, image_size(i));
            sum_time_serial = sum_time_serial + toc(start_time_serial); % end timer

            % Determining Parallel Mandelbrot function's execution time 
            start_time_parallel = tic; % start timer
            iteration_matrix_parallel = mandelbrot_parallel(max_iterations, image_size(i), workers);
            sum_time_parallel = sum_time_parallel + toc(start_time_parallel); % end timer
            
        end
        
        % Plot and save Mandelbrot images
        mandelbrot_plot(iteration_matrix_serial, max_iterations, image_names{i}, "serial");
        mandelbrot_plot(iteration_matrix_parallel, max_iterations, image_names{i}, "parallel");
        
        time_serial(i) = sum_time_serial/repetitions;
        time_parallel(i) = sum_time_parallel/repetitions;
        speedup(i) = time_serial(i)/time_parallel(i);
        efficiency(i) = (speedup(i)*100)/n;
    end

    % Display results
    disp("time_serial")
    disp(time_serial)
    disp("time_parallel")
    disp(time_parallel)
    disp("speedup")
    disp(speedup)
    disp("efficiency")
    disp(efficiency)
       
    % Plot double bar graph for execution times
    Y1 = [time_serial; time_parallel];
    figure
    b1 = bar(Y1);               
    b1(1).FaceColor = [0.2 0.6 0.8];
    b1(2).FaceColor = [0.9 0.4 0.3];
    legend({'Serial','Parallel'}, 'Location','best');
    xticks(1:size(Y1,1));
    xticklabels(image_names);
    xlabel('Image Resolutions');
    ylabel('Execution Time (s)');
    title('Double Bar Graph showing the Execution Times of the Serial and Parallel implementation of the Mandelbrot function for each Image Resolution');
    grid on

    % Plot bar graph for speedup
    figure
    bar(speedup);               
    xticks(1:8);
    xticklabels(image_names);
    xlabel('Image Resolutions');
    ylabel('Speed Up/Slow down');
    title('Bar Graph showing the Speed Up/Slow Down of Parallel-computed Mandelbrot function for each Image Resolution');
    grid on

    % Plot bar graph for efficiency
    figure
    bar(efficiency);               
    xticks(1:8);
    xticklabels(image_names);
    xlabel('Image Resolutions');
    ylabel('Efficiency (%)');
    title('Bar Graph showing the Efficiency of core utilisation by the Parallel Mandelbrot function for each Image Resolution');
    grid on

end