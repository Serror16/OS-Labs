#ifndef LAB5_COMMANDS_H
#define LAB5_COMMANDS_H

#include "lab5.h"

typedef struct LibraryFunctions {
    Lab5GcfFn gcf;
    Lab5SortFn sort;
} LibraryFunctions;

typedef int (*Lab5SwitchFn)(LibraryFunctions *functions, void *context);

int run_commands(LibraryFunctions *functions, Lab5SwitchFn switch_function, void *context);

#endif
