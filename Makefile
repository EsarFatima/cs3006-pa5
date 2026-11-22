
CC = gcc
CFLAGS = -Wall -Wextra -O2
OMPFLAGS = -fopenmp

all: localsort

localsort: src/localsort.c src/quicksort_seq.c src/quicksort_omp.c pa5_common.h
	$(CC) $(CFLAGS) $(OMPFLAGS) src/localsort.c src/quicksort_seq.c src/quicksort_omp.c -o localsort

clean:
	rm -f localsort *.o
