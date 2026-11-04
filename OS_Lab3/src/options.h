#ifndef OPTIONS_H
#define OPTIONS_H

#include <stddef.h>

typedef struct {
    const char *input_path;
    size_t clusters;
    size_t threads;
} Options;

int parse_options(int argc, char **argv, Options *options);
void print_usage(const char *program);

#endif
