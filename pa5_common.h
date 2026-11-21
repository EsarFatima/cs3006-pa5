/*
 * pa5_common.h -- CS3006 Fall 2026, Programming Assignment 5
 *
 * Given code. Do not modify this file. You may read it, include it, and
 * call the functions in it. It contains nothing you are graded on: input
 * generation, block distribution, and an order-independent fingerprint
 * for checking that sorting neither lost nor invented keys.
 */
#ifndef PA5_COMMON_H
#define PA5_COMMON_H

#include <stdint.h>
#include <stddef.h>

/* ---- Input generation ------------------------------------------------
 * pa5_key(seed, i) is the i-th key of the global input array, for
 * i = 0 .. n-1. It depends only on (seed, i): any process can generate
 * any slice of the global array without communicating, and the global
 * array is identical whatever the number of processes.               */
static inline uint64_t pa5_mix64(uint64_t z)
{
    z += 0x9E3779B97F4A7C15ULL;                 /* splitmix64 finaliser */
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static inline uint32_t pa5_key(uint64_t seed, uint64_t i)
{
    return (uint32_t)(pa5_mix64(seed * 0x2545F4914F6CDD1DULL + i) >> 32);
}

/* ---- Block distribution ----------------------------------------------
 * Global indices are split into p contiguous blocks. The first (n mod p)
 * ranks hold one extra element. Rank r holds indices
 * [pa5_block_lo(n,p,r), pa5_block_lo(n,p,r) + pa5_block_count(n,p,r)). */
static inline uint64_t pa5_block_count(uint64_t n, int p, int r)
{
    return n / (uint64_t)p + ((uint64_t)r < n % (uint64_t)p ? 1 : 0);
}

static inline uint64_t pa5_block_lo(uint64_t n, int p, int r)
{
    uint64_t q = n / (uint64_t)p, rem = n % (uint64_t)p;
    return (uint64_t)r * q + ((uint64_t)r < rem ? (uint64_t)r : rem);
}

/* ---- Fingerprint -----------------------------------------------------
 * An order-independent fingerprint of a multiset of keys. Sorting must
 * leave it unchanged. Combine per-rank fingerprints with MPI_Allreduce:
 * .sum with MPI_SUM and .x with MPI_BXOR, both on MPI_UINT64_T.        */
typedef struct { uint64_t sum; uint64_t x; } pa5_fp_t;

static inline pa5_fp_t pa5_fingerprint(const uint32_t *a, uint64_t m)
{
    pa5_fp_t f = {0, 0};
    for (uint64_t i = 0; i < m; i++) {
        uint64_t h = pa5_mix64(a[i]);
        f.sum += h;
        f.x ^= h;
    }
    return f;
}

#endif /* PA5_COMMON_H */
