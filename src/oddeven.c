#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <mpi.h>
#include <omp.h>
#include "../pa5_common.h"

void quicksort_omp(uint32_t *a, size_t n, size_t cutoff);

int main(int argc, char *argv[])
{
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);

    int rank, p;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    if (provided < MPI_THREAD_FUNNELED) {
        if (rank == 0)
            fprintf(stderr, "MPI does not provide FUNNELED support\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (argc != 4) {
        if (rank == 0)
            fprintf(stderr, "Usage: mpirun -np <p> ./oddeven <n> <seed> <cutoff>\n");
        MPI_Finalize();
        return 1;
    }

    uint64_t n = strtoull(argv[1], NULL, 10);
    uint64_t seed = strtoull(argv[2], NULL, 10);
    size_t cutoff = (size_t)strtoull(argv[3], NULL, 10);

    if (n == 0 || n < (uint64_t)p || n > INT_MAX || cutoff == 0) {
        if (rank == 0)
            fprintf(stderr, "Invalid arguments: require n >= p and cutoff > 0\n");
        MPI_Finalize();
        return 1;
    }

    uint64_t local_n = pa5_block_count(n, p, rank);
    uint64_t local_lo = pa5_block_lo(n, p, rank);
    uint64_t max_n = pa5_block_count(n, p, 0);

    /* Allocate three buffers sized to max_n to safely handle unequal block sizes */
    uint32_t *local = malloc((size_t)max_n * sizeof(uint32_t));
    uint32_t *received = malloc((size_t)max_n * sizeof(uint32_t));
    uint32_t *merged = malloc((size_t)max_n * sizeof(uint32_t));

    if (!local || !received || !merged) {
        fprintf(stderr, "Rank %d: memory allocation failed\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* Generate local block (outside timed region) */
    for (uint64_t i = 0; i < local_n; i++) {
        local[i] = pa5_key(seed, local_lo + i);
    }

    /* Input global fingerprint (outside timed region) */
    pa5_fp_t local_fp_in = pa5_fingerprint(local, local_n);
    pa5_fp_t global_fp_in = {0, 0};
    MPI_Allreduce(&local_fp_in.sum, &global_fp_in.sum, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(&local_fp_in.x, &global_fp_in.x, 1, MPI_UINT64_T, MPI_BXOR, MPI_COMM_WORLD);

    /* ---- Timed Region (B2) ---- */
    MPI_Barrier(MPI_COMM_WORLD);
    double t_total_start = MPI_Wtime();

    /* 1. Local OpenMP sort */
    double t0 = MPI_Wtime();
    quicksort_omp(local, (size_t)local_n, cutoff);
    double t_local = MPI_Wtime() - t0;

    double t_comm = 0.0;
    double t_merge = 0.0;

    /* 2. Exactly p phases of Odd-Even Transposition */
    for (int phase = 0; phase < p; phase++) {
        int partner;
        if (phase % 2 == 0) {
            partner = (rank % 2 == 0) ? (rank + 1) : (rank - 1);
        } else {
            partner = (rank % 2 == 1) ? (rank + 1) : (rank - 1);
        }

        if (partner >= 0 && partner < p) {
            uint64_t partner_n = pa5_block_count(n, p, partner);

            double tc0 = MPI_Wtime();
            MPI_Sendrecv(local, (int)local_n, MPI_UINT32_T, partner, 0,
                         received, (int)partner_n, MPI_UINT32_T, partner, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            t_comm += MPI_Wtime() - tc0;

            double tm0 = MPI_Wtime();
            if (rank < partner) {
                /* Keep smallest local_n keys: merge from front */
                uint64_t i = 0, j = 0, k = 0;
                while (k < local_n) {
                    if (j >= partner_n || (i < local_n && local[i] <= received[j])) {
                        merged[k++] = local[i++];
                    } else {
                        merged[k++] = received[j++];
                    }
                }
            } else {
                /* Keep largest local_n keys: merge from back */
                int64_t i = (int64_t)local_n - 1;
                int64_t j = (int64_t)partner_n - 1;
                int64_t k = (int64_t)local_n - 1;
                while (k >= 0) {
                    if (j < 0 || (i >= 0 && local[i] >= received[j])) {
                        merged[k--] = local[i--];
                    } else {
                        merged[k--] = received[j--];
                    }
                }
            }
            t_merge += MPI_Wtime() - tm0;

            /* Swap pointers instead of copying */
            uint32_t *tmp = local;
            local = merged;
            merged = tmp;
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double t_total = MPI_Wtime() - t_total_start;

    /* ---- Verification (B3) ---- */
    /* 1. Local ascending order */
    int ok_local = 1;
    for (uint64_t i = 1; i < local_n; i++) {
        if (local[i - 1] > local[i]) {
            ok_local = 0;
            break;
        }
    }

    /* 2. Boundary check: smallest key >= left neighbour's largest key */
    int ok_boundary = 1;
    uint32_t my_first = local[0];
    uint32_t my_last = local[local_n - 1];
    uint32_t left_last = 0;
    int left_neighbor = (rank > 0) ? (rank - 1) : MPI_PROC_NULL;
    int right_neighbor = (rank < p - 1) ? (rank + 1) : MPI_PROC_NULL;

    MPI_Sendrecv(&my_last, 1, MPI_UINT32_T, right_neighbor, 1,
                 &left_last, 1, MPI_UINT32_T, left_neighbor, 1,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    if (rank > 0 && my_first < left_last) {
        ok_boundary = 0;
    }

    /* 3. Global fingerprint match */
    pa5_fp_t local_fp_out = pa5_fingerprint(local, local_n);
    pa5_fp_t global_fp_out = {0, 0};
    MPI_Allreduce(&local_fp_out.sum, &global_fp_out.sum, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(&local_fp_out.x, &global_fp_out.x, 1, MPI_UINT64_T, MPI_BXOR, MPI_COMM_WORLD);

    int ok_fp = (global_fp_in.sum == global_fp_out.sum) && (global_fp_in.x == global_fp_out.x);

    int local_all_ok = ok_local && ok_boundary && ok_fp;
    int global_ok = 0;
    MPI_Allreduce(&local_all_ok, &global_ok, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);

    /* ---- Timing Reductions & Output Format (B2 & B4) ---- */
    double t_local_max = 0.0, t_comm_max = 0.0, t_merge_max = 0.0;
    MPI_Reduce(&t_local, &t_local_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&t_comm, &t_comm_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&t_merge, &t_merge_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    char hostname[MPI_MAX_PROCESSOR_NAME];
    int name_len = 0;
    MPI_Get_processor_name(hostname, &name_len);

    int num_threads = omp_get_max_threads();
    char line_buf[512];
    snprintf(line_buf, sizeof(line_buf),
             "rank %d on %s: threads=%d keys=%llu local=%.6f comm=%.6f merge=%.6f",
             rank, hostname, num_threads, (unsigned long long)local_n,
             t_local, t_comm, t_merge);

    char *all_lines = NULL;
    if (rank == 0) {
        all_lines = malloc((size_t)p * 512);
    }

    MPI_Gather(line_buf, 512, MPI_CHAR, all_lines, 512, MPI_CHAR, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        for (int r = 0; r < p; r++) {
            printf("%s\n", all_lines + (size_t)r * 512);
        }
        printf("RESULT,%d,%llu,%.6f,%.6f,%.6f,%.6f,%s\n",
               p, (unsigned long long)n, t_total, t_local_max, t_comm_max, t_merge_max,
               global_ok ? "OK" : "FAIL");
        free(all_lines);
    }

    free(local);
    free(received);
    free(merged);

    MPI_Finalize();
    return global_ok ? 0 : 1;
}