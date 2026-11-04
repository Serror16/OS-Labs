#ifndef KMEANS_H
#define KMEANS_H

#include "points.h"

typedef struct {
    size_t iterations;
    double elapsed_seconds;
} KMeansStats;

int kmeans_run(const Point *points, size_t point_count, size_t clusters,
               size_t threads, Point *centers, size_t *labels,
               size_t *counts, KMeansStats *stats);

#endif
