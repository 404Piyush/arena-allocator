/* bench/bench_arena.c
 *
 * Microbenchmark: compare arena_alloc vs system malloc for many
 * small allocations + a reset cycle.
 *
 * Usage: ./build/bench_arena [N]
 */
#include "arena.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(int argc, char **argv) {
    int n = (argc > 1) ? atoi(argv[1]) : 1000000;

    /* ---- Phase 1: 1 million small mallocs vs arena allocs ---- */
    arena *a = arena_create((size_t)n * 32 + 4096);

    double t0 = now_sec();
    for (int i = 0; i < n; i++) {
        volatile void *p = arena_alloc(a, 24);
        (void)p;
    }
    double t1 = now_sec();
    printf("arena  %d : %7.3f s  (%9.0f ops/s)  high_wat=%zu\n",
           n, t1 - t0, n / (t1 - t0), arena_high_wat(a));

    /* Reset is O(1). */
    double t2 = now_sec();
    arena_reset(a);
    double t3 = now_sec();
    printf("reset       : %7.3f s\n", t3 - t2);

    /* System malloc for comparison. */
    void **ptrs = malloc((size_t)n * sizeof(void *));
    t0 = now_sec();
    for (int i = 0; i < n; i++) {
        ptrs[i] = malloc(24);
    }
    t1 = now_sec();
    printf("malloc %d : %7.3f s  (%9.0f ops/s)\n", n, t1 - t0, n / (t1 - t0));
    for (int i = 0; i < n; i++) free(ptrs[i]);
    free(ptrs);

    /* ---- Phase 2: reset cycle (1000 phases of N/1000 allocs) ---- */
    int phase = n / 1000;
    if (phase < 1) phase = 1;
    t0 = now_sec();
    for (int p = 0; p < 1000; p++) {
        for (int i = 0; i < phase; i++) arena_alloc(a, 24);
        arena_reset(a);
    }
    t1 = now_sec();
    printf("arena  cycle (%d x %d) : %7.3f s\n", 1000, phase, t1 - t0);

    arena_destroy(a);
    return 0;
}
