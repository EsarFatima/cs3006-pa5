#include <stdio.h>
#include <stdint.h>

void quicksort_seq(uint32_t *a, int lo, int hi);

int main(void)
{
    uint32_t a[] = {42};

    int n = sizeof(a) / sizeof(a[0]);

    quicksort_seq(a, 0, n - 1);

    printf("Sorted array: ");

    for (int i = 0; i < n; i++)
    {
        printf("%u ", a[i]);
    }

    printf("\n");

    return 0;
}