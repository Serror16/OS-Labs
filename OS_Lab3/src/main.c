#include "kmeans.h"
#include "options.h"
#include "points.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    Options options;
    KMeansStats stats;
    Point *points = NULL, *centers = NULL;
    size_t *labels = NULL, *counts = NULL;
    size_t point_count = 0, cluster;
    int result = EXIT_FAILURE;

    if (!parse_options(argc, argv, &options)) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    if (!read_points(options.input_path, &points, &point_count)) goto cleanup;
    if (options.clusters > point_count) {
        fprintf(stderr, "K must be no greater than the point count.\n");
        goto cleanup;
    }
    if (options.clusters > SIZE_MAX / sizeof(*centers) ||
        point_count > SIZE_MAX / sizeof(*labels) ||
        options.clusters > SIZE_MAX / sizeof(*counts)) goto run_error;
    centers = malloc(options.clusters * sizeof(*centers));
    labels = malloc(point_count * sizeof(*labels));
    counts = malloc(options.clusters * sizeof(*counts));
    if (!centers || !labels || !counts) goto run_error;
    if (!kmeans_run(points, point_count, options.clusters, options.threads,
                    centers, labels, counts, &stats)) goto run_error;

    printf("iterations=%zu elapsed_seconds=%.9f\n",
           stats.iterations, stats.elapsed_seconds);
    for (cluster = 0; cluster < options.clusters; ++cluster)
        printf("cluster=%zu center=(%.9f, %.9f) points=%zu\n", cluster + 1,
               centers[cluster].x, centers[cluster].y, counts[cluster]);
    result = EXIT_SUCCESS;
    goto cleanup;

run_error:
    fprintf(stderr, "K-means failed.\n");
cleanup:
    free(counts);
    free(labels);
    free(centers);
    free(points);
    return result;
}
