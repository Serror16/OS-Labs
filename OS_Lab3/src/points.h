#ifndef POINTS_H
#define POINTS_H

#include <stddef.h>

typedef struct {
    double x;
    double y;
} Point;

int read_points(const char *path, Point **points, size_t *count);

#endif
