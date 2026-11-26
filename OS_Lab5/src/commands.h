#ifndef LAB5_COMMANDS_H
#define LAB5_COMMANDS_H

#include "lab5.h"

typedef struct Lab5Functions {
    Lab5GcfFn gcf;
    Lab5SortFn sort;
} Lab5Functions;

typedef int (*Lab5SwitchFn)(Lab5Functions *functions, void *context);

int lab5_run(Lab5Functions *functions, Lab5SwitchFn switch_function, void *context);

#endif
