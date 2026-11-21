
#include <stdint.h>
#include <stddef.h>
#include <omp.h>
#include "../pa5_common.h"

void quicksort_seq(uint32_t *a, int lo, int hi);

static void swap(uint32_t *x, uint32_t *y)
{
    uint32_t temp = *x;
    *x = *y;
    *y = temp;
}

static uint32_t median_of_three(uint32_t x, uint32_t y, uint32_t z)
{
    if (x < y)
    {
        if (y < z)
            return y;
        else if (x < z)
            return z;
        else
            return x;
    }
    else
    {
        if (x < z)
            return x;
        else if (y < z)
            return z;
        else
            return y;
    }
}

static void partition(
    uint32_t *a,
    int lo,
    int hi,
    int *lt_out,
    int *gt_out)
{
    int mid = lo + (hi - lo) / 2;

    uint32_t pivot = median_of_three(a[lo], a[mid], a[hi]);

    int lt = lo;
    int i = lo;
    int gt = hi;

    while (i <= gt)
    {
        if (a[i] < pivot)
        {
            swap(&a[lt], &a[i]);
            lt++;
            i++;
        }
        else if (a[i] > pivot)
        {
            swap(&a[i], &a[gt]);
            gt--;
        }
        else
        {
            i++;
        }
    }

    *lt_out = lt;
    *gt_out = gt;
}

static void quicksort_task(uint32_t *a, int lo, int hi, size_t cutoff)
{
    if (lo >= hi)
        return;

    size_t length = (size_t)(hi - lo + 1);

    if (length <= cutoff)
    {
        quicksort_seq(a, lo, hi);
        return;
    }

    int lt, gt;

    partition(a, lo, hi, &lt, &gt);

    int left_lo = lo;
    int left_hi = lt - 1;

    int right_lo = gt + 1;
    int right_hi = hi;

    /* Sort the left partition */
    size_t left_size = (left_hi >= left_lo)
                           ? (size_t)(left_hi - left_lo + 1)
                           : 0;

    if (left_size > 0 && left_size <= cutoff)
    {
        quicksort_seq(a, left_lo, left_hi);
    }
    else if (left_size > cutoff)
    {
        #pragma omp task shared(a) firstprivate(left_lo, left_hi, cutoff)
        {
            quicksort_task(a, left_lo, left_hi, cutoff);
        }
    }

    /* Sort the right partition */
    size_t right_size = (right_hi >= right_lo)
                            ? (size_t)(right_hi - right_lo + 1)
                            : 0;

    if (right_size > 0 && right_size <= cutoff)
    {
        quicksort_seq(a, right_lo, right_hi);
    }
    else if (right_size > cutoff)
    {
        #pragma omp task shared(a) firstprivate(right_lo, right_hi, cutoff)
        {
            quicksort_task(a, right_lo, right_hi, cutoff);
        }
    }

    #pragma omp taskwait
}

void quicksort_omp(uint32_t *a, size_t n, size_t cutoff)
{
    if (n <= 1)
        return;

    if (cutoff == 0)
        cutoff = 1;

    #pragma omp parallel
    {
        #pragma omp single
        {
            quicksort_task(a, 0, (int)n - 1, cutoff);
        }
    }
}
