
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <omp.h>
#include <limits.h>
#include "../pa5_common.h"

void quicksort_seq(uint32_t *a, int lo, int hi);
void quicksort_omp(uint32_t *a, size_t n, size_t cutoff);

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <n> <seed> <cutoff>\n", argv[0]);
        return 1;
    }

    uint64_t n = strtoull(argv[1], NULL, 10);
    uint64_t seed = strtoull(argv[2], NULL, 10);
    size_t cutoff = (size_t)strtoull(argv[3], NULL, 10);

    if (n == 0 || n > INT_MAX || cutoff == 0)
    {
        fprintf(stderr, "Invalid arguments\n");
        return 1;
    }

    if (n > SIZE_MAX / sizeof(uint32_t))
    {
        fprintf(stderr, "Array size is too large\n");
        return 1;
    }

    /* Allocate three arrays */
    uint32_t *original = malloc((size_t)n * sizeof(uint32_t));
    uint32_t *seq = malloc((size_t)n * sizeof(uint32_t));
    uint32_t *omp_array = malloc((size_t)n * sizeof(uint32_t));

    if (original == NULL || seq == NULL || omp_array == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        free(original);
        free(seq);
        free(omp_array);
        return 1;
    }

    /* Generate original input */
    for (uint64_t i = 0; i < n; i++)
    {
        original[i] = pa5_key(seed, i);
    }

    /* Copy input for both sorting algorithms */
    for (uint64_t i = 0; i < n; i++)
    {
        seq[i] = original[i];
        omp_array[i] = original[i];
    }

    /* Fingerprint original input */
    pa5_fp_t original_fp = pa5_fingerprint(original, n);

    /* Sequential sorting and timing */
    double start_seq = omp_get_wtime();

    quicksort_seq(seq, 0, (int)n - 1);

    double end_seq = omp_get_wtime();
    double t_seq = end_seq - start_seq;

    /* OpenMP sorting and timing */
    double start_omp = omp_get_wtime();

    quicksort_omp(omp_array, (size_t)n, cutoff);

    double end_omp = omp_get_wtime();
    double t_omp = end_omp - start_omp;

    /* Verify sorted order */
    int seq_sorted = 1;
    int omp_sorted = 1;

    for (uint64_t i = 1; i < n; i++)
    {
        if (seq[i - 1] > seq[i])
            seq_sorted = 0;

        if (omp_array[i - 1] > omp_array[i])
            omp_sorted = 0;
    }

    /* Verify fingerprints */
    pa5_fp_t seq_fp = pa5_fingerprint(seq, n);
    pa5_fp_t omp_fp = pa5_fingerprint(omp_array, n);

    int seq_correct =
        seq_sorted &&
        seq_fp.sum == original_fp.sum &&
        seq_fp.x == original_fp.x;

    int omp_correct =
        omp_sorted &&
        omp_fp.sum == original_fp.sum &&
        omp_fp.x == original_fp.x;

    int threads = omp_get_max_threads();

    /* Print results */
    printf("%llu,%d,%zu,%.6f,%.6f,%s\n",
           (unsigned long long)n,
           threads,
           cutoff,
           t_seq,
           t_omp,
           (seq_correct && omp_correct) ? "OK" : "FAIL");

    /* Release memory */
    free(original);
    free(seq);
    free(omp_array);

    return (seq_correct && omp_correct) ? 0 : 1;
}
