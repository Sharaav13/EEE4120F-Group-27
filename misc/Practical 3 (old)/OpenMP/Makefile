
CC=gcc
CFLAGS=-O3 -fopenmp
CFLAGS_DEBUG=-g -fopenmp
ARGS=

all: wariara_freights_route wariara_freights_route_debug

.PHONY: clean

clean: 
	rm -rf wariara_freights_route wariara_freights_route_debug *~ *.o

wariara_freights_route: wariara_freights_route.c
	$(CC) $(CFLAGS) $< -o wariara_freights_route

wariara_freights_route_debug: wariara_freights_route.c
	$(CC) $(CFLAGS_DEBUG) $< -o wariara_freights_route_debug

run: wariara_freights_route
	./wariara_freights_route $(ARGS)

run_debug: wariara_freights_route_debug
	./wariara_freights_route_debug $(ARGS)

