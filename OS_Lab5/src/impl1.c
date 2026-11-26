#include "lab5.h"

int GCF(int A, int B)
{
    if (A <= 0 || B <= 0) {
        return 0;
    }

    while (B != 0) {
        int remainder = A % B;
        A = B;
        B = remainder;
    }
    return A;
}

int *Sort(int *array)
{
    if (array == 0 || array[0] < 0) {
        return 0;
    }

    for (int end = array[0]; end > 1; --end) {
        for (int i = 1; i < end; ++i) {
            if (array[i] > array[i + 1]) {
                int temporary = array[i];
                array[i] = array[i + 1];
                array[i + 1] = temporary;
            }
        }
    }
    return array;
}
