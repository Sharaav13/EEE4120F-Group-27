CC = mpicc
CFLAGS = 
NP = 4

all: mpi_test

mpi_test: mpi_test.c
	$(CC) $(CFLAGS) $< -o $@

run:
	mpirun --use-hwthread-cpus -np $(NP) ./mpi_test

clean:
	rm -f mpi_test