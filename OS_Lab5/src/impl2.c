#include "lab5.h"

int GCF(int A, int B)
{
    if (A <= 0 || B <= 0) {
        return 0;
    }

    int divisor = A < B ? A : B;
    for (; divisor >= 1; --divisor) {
        if (A % divisor == 0 && B % divisor == 0) {
            return divisor;
        }
    }
    return 0;
}

static void quick_sort(int *array, long long left, long long right)
{
    while (left < right) {
        long long i = left;
        long long j = right;
        int pivot = array[left + (right - left) / 2];

        while (i <= j) {
            while (array[i] < pivot) {
                ++i;
            }
            while (array[j] > pivot) {
                --j;
            }
            if (i <= j) {
                int temporary = array[i];
                array[i] = array[j];
                array[j] = temporary;
                ++i;
                --j;
            }
        }

        /* Recurse into the smaller partition to keep stack usage bounded. */
        if (j - left < right - i) {
            if (left < j) {
                quick_sort(array, left, j);
            }
            left = i;
        } else {
            if (i < right) {
                quick_sort(array, i, right);
            }
            right = j;
        }
    }
}

int *Sort(int *array)
{
    if (array == 0 || array[0] < 0) {
        return 0;
    }
    if (array[0] > 1) {
        quick_sort(array, 1, array[0]);
    }
    return array;
}
