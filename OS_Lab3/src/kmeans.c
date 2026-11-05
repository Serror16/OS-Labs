#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "kmeans.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ITERATIONS 100


typedef struct {
    double sum_x;
    double sum_y;
    size_t count;
} Partial; //частичный результат одного потока по одному кластеру

typedef struct {
    const Point *points;
    const Point *centers;
    size_t *labels; //labels[i] — номер кластера точки i
    size_t begin;
    size_t end;
    size_t clusters;
    size_t changed; //количество точек этого потока, у которых на текущей итерации изменился номер кластера.
    Partial *partial; //адрес массива из К частичных результатов именно этого потока
    HANDLE handle; //дескриптор созданного потока Windows
} Job;

static size_t min_size(size_t a, size_t b)
{
    if (a < b) return a;
    return b;
}


static void process_points(Job *job) {
    size_t i, cluster;
    job->changed = 0;
    memset(job->partial, 0, job->clusters * sizeof(*job->partial));

    for (i = job->begin; i < job->end; i++) {
        Point point = job->points[i];
        size_t best = 0;
        double dx = point.x - job->centers[0].x;
        double dy = point.y - job->centers[0].y;
        double best_distance = dx * dx + dy * dy;
        for (cluster = 1; cluster < job->clusters; cluster++) {
            double distance;
            dx = point.x - job->centers[cluster].x;
            dy = point.y - job->centers[cluster].y;
            distance = dx * dx + dy * dy;
            if (distance < best_distance) {
                best_distance = distance;
                best = cluster;
            }
        }

        if (job->labels[i] != best) {
            job->labels[i] = best;
            ++job->changed;
        }

        job->partial[best].sum_x += point.x;
        job->partial[best].sum_y += point.y;
        ++job->partial[best].count;
    }
}

static DWORD WINAPI worker_entry(LPVOID argument) {
    process_points((Job *)argument);
    return 0;
}

int kmeans_run(const Point *points, size_t point_count, size_t clusters,
               size_t threads, Point *centers, size_t *labels,
               size_t *counts, KMeansStats *stats) {
    Job *jobs;
    Partial *partials, *totals;
    LARGE_INTEGER frequency, start, finish;
    size_t i, cluster, iteration, created, changed;
    size_t thread_count = min_size(threads, point_count);
    size_t points_per_thread = point_count / thread_count;
    size_t extra_points = point_count % thread_count;

    if (thread_count > SIZE_MAX / sizeof(*jobs) ||
        clusters > SIZE_MAX / thread_count ||
        thread_count * clusters > SIZE_MAX / sizeof(*partials) ||
        clusters > SIZE_MAX / sizeof(*totals))
        return 0;

    jobs = malloc(thread_count * sizeof(*jobs));
    partials = malloc(thread_count * clusters * sizeof(*partials));
    totals = malloc(clusters * sizeof(*totals));

    if (!jobs || !partials || !totals) {
        free(jobs);
        free(partials);
        free(totals);
        return 0;
    }

    for (i = 0; i < point_count; ++i)
        labels[i] = SIZE_MAX;

    for (cluster = 0; cluster < clusters; cluster++)
        centers[cluster] = points[cluster];

    for (i = 0; i < thread_count; i++) {
        jobs[i].points = points;
        jobs[i].centers = centers;
        jobs[i].labels = labels;
        jobs[i].begin = points_per_thread * i + min_size(i, extra_points);
        jobs[i].end = points_per_thread * (i + 1) + min_size(i + 1, extra_points);
        jobs[i].clusters = clusters;
        jobs[i].partial = partials + i * clusters;
    }

    printf("PID=%lu points=%zu clusters=%zu compute_threads=%zu\n",
           (unsigned long)GetCurrentProcessId(), point_count, clusters, thread_count);
    fflush(stdout);
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);

    //запуск расчета
    for (iteration = 1; iteration <= MAX_ITERATIONS; iteration++) {
        memset(totals, 0, clusters * sizeof(*totals));
        created = 0;
        for (i = 1; i < thread_count; i++) {
            jobs[i].handle = CreateThread(NULL, 0, worker_entry, &jobs[i], 0, NULL);
            if (!jobs[i].handle) {
                for (i = 1; i <= created; i++) {
                    WaitForSingleObject(jobs[i].handle, INFINITE);
                    CloseHandle(jobs[i].handle);
                }
                free(jobs);
                free(partials);
                free(totals);
                return 0;
            }
            created++;
        }

        process_points(&jobs[0]);
        for (i = 1; i < thread_count; i++) {
            WaitForSingleObject(jobs[i].handle, INFINITE);
            CloseHandle(jobs[i].handle);
        }



        changed = 0;
        for (i = 0; i < thread_count; i++) {
            changed += jobs[i].changed;
            for (cluster = 0; cluster < clusters; cluster++) {
                totals[cluster].sum_x += jobs[i].partial[cluster].sum_x;
                totals[cluster].sum_y += jobs[i].partial[cluster].sum_y;
                totals[cluster].count += jobs[i].partial[cluster].count;
            }
        }
        for (cluster = 0; cluster < clusters; cluster++) { //проходит по всем кластерам для пересчёта центров
            counts[cluster] = totals[cluster].count;
            if (counts[cluster]) {
                centers[cluster].x = totals[cluster].sum_x / (double)counts[cluster];
                centers[cluster].y = totals[cluster].sum_y / (double)counts[cluster];
            }
        }
        if (changed == 0) break;
    }

    QueryPerformanceCounter(&finish);
    stats->iterations = min_size(iteration, MAX_ITERATIONS);
    stats->elapsed_seconds = (double)(finish.QuadPart - start.QuadPart) / (double)frequency.QuadPart;  //делит число отсчётов на частоту счётчика и получает время в секундах. В него входят создание потоков, расчёт, ожидание, объединение сумм и пересчёт центров на всех итерациях
    free(jobs);
    free(partials);
    free(totals);
    return 1;
}
