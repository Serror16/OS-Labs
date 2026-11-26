#ifndef LAB5_H
#define LAB5_H

#if defined(_WIN32)
#  if defined(LAB5_BUILD_DLL)
#    define LAB5_API __declspec(dllexport)
#  else
#    define LAB5_API __declspec(dllimport)
#  endif
#else
#  define LAB5_API
#endif

/* Both arguments must be positive natural numbers. Returns 0 for invalid input. */
LAB5_API int GCF(int A, int B);

/*
 * array[0] is the number of elements; values are array[1] through array[array[0]].
 * The function sorts values in place, leaves array[0] unchanged, and returns array.
 * Returns NULL if array is NULL or its length is negative.
 */
LAB5_API int *Sort(int *array);

typedef int (*Lab5GcfFn)(int, int);
typedef int *(*Lab5SortFn)(int *);

#endif
