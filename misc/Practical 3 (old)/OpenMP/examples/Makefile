CC = gcc
CFLAGS = -fopenmp

all: omp_test

omp_test: omp_test.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f omp_test