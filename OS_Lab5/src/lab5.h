#ifndef LAB5_H
#define LAB5_H

#ifdef LAB5_BUILD_DLL
#define LAB5_API __declspec(dllexport)
#else
#define LAB5_API __declspec(dllimport)
#endif

LAB5_API int GCF(int A, int B);


LAB5_API int *Sort(int *array);

typedef int (*Lab5GcfFn)(int, int);
typedef int *(*Lab5SortFn)(int *);

#endif
