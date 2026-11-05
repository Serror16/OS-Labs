#include "points.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int read_points(const char *path, Point **result, size_t *result_count) {
    FILE *file = fopen(path, "r");
    Point *points = NULL;
    size_t count, i;
    if (!file) {
        perror(path);
        return 0;
    }
    if (fscanf(file, "%zu", &count) != 1 || count == 0 || count > SIZE_MAX / sizeof(*points)) {
        fprintf(stderr, "Invalid point count in %s\n", path);
        goto fail;
    }
    points = malloc(count * sizeof(*points));
    if (!points) goto fail;
    for (i = 0; i < count; i++) {
        if (fscanf(file, "%lf %lf", &points[i].x, &points[i].y) != 2) {
            fprintf(stderr, "Invalid point %zu in %s\n", i + 1, path);
            goto fail;
        }
    }
    fclose(file);
    *result = points;
    *result_count = count;
    return 1;
fail:
    fclose(file);
    free(points);
    return 0;
}
