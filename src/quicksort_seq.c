#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include "../pa5_common.h"

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
void quicksort_seq(uint32_t *a, int lo, int hi)
{
    while (lo < hi)
    {
        int mid = lo + (hi - lo) / 2;

        uint32_t pivot = median_of_three(a[lo], a[mid], a[hi]);

        int lt = lo;
        int i = lo;
        int gt = hi;

        // ADD THE PARTITION LOOP HERE
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

        // Recursion code will go here next
        if (lt - lo < hi - gt)
        {
              quicksort_seq(a, lo, lt - 1);
              lo = gt + 1;
        }
        else
       {
              quicksort_seq(a, gt + 1, hi);
              hi = lt - 1;
       }
    }
}