#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

void quicksort_omp(uint32_t *a, size_t n, size_t cutoff);

static int is_sorted(uint32_t *a, size_t n)
{
    for (size_t i = 1; i < n; i++)
    {
        if (a[i - 1] > a[i])
            return 0;
    }

    return 1;
}

static void run_test(uint32_t *a, size_t n, size_t cutoff)
{
    quicksort_omp(a, n, cutoff);

    if (is_sorted(a, n))
        printf("PASS\n");
    else
        printf("FAIL\n");
}

int main(void)
{
    uint32_t a1[] = {9, 3, 7, 3, 1, 8, 5, 3, 12, 2};
    uint32_t a2[] = {1, 2, 3, 4, 5};
    uint32_t a3[] = {9, 7, 5, 3, 1};
    uint32_t a4[] = {5, 5, 5, 5, 5};
    uint32_t a5[] = {42};

    printf("Random/duplicate values: ");
    run_test(a1, 10, 2);

    printf("Already sorted: ");
    run_test(a2, 5, 2);

    printf("Reverse sorted: ");
    run_test(a3, 5, 2);

    printf("All duplicates: ");
    run_test(a4, 5, 2);

    printf("Single element: ");
    run_test(a5, 1, 2);

    printf("Empty array: ");
    run_test(NULL, 0, 2);

    return 0;
}