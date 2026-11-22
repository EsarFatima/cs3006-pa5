CC = gcc
MPICC = mpicc
CFLAGS = -Wall -Wextra -O2
OMPFLAGS = -fopenmp

all: localsort oddeven

localsort: src/localsort.c src/quicksort_seq.c src/quicksort_omp.c pa5_common.h
	$(CC) $(CFLAGS) $(OMPFLAGS) src/localsort.c src/quicksort_seq.c src/quicksort_omp.c -o localsort

oddeven: src/oddeven.c src/quicksort_seq.c src/quicksort_omp.c pa5_common.h
	$(MPICC) $(CFLAGS) $(OMPFLAGS) src/oddeven.c src/quicksort_seq.c src/quicksort_omp.c -o oddeven

clean:
	rm -f localsort oddeven test_quicksort test_quicksort_omp *.o
