/* tests/test_arena.c
 *
 * Test suite for the bump arena. Custom test framework, zero
 * external deps.
 */
#include "arena.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_pass = 0;
static int g_fail = 0;

#define ASSERT_TRUE(cond)                                                  \
    do {                                                                    \
        if (cond) { ++g_pass; }                                            \
        else { ++g_fail;                                                   \
            fprintf(stderr, "FAIL: %s:%d  %s\n", __FILE__, __LINE__, #cond); \
        }                                                                   \
    } while (0)

#define ASSERT_EQ_SIZE(a, b)                                              \
    do {                                                                    \
        size_t _a = (size_t)(a), _b = (size_t)(b);                         \
        if (_a == _b) { ++g_pass; }                                        \
        else { ++g_fail;                                                   \
            fprintf(stderr, "FAIL: %s:%d  %s == %s  (got %zu vs %zu)\n",     \
                    __FILE__, __LINE__, #a, #b, _a, _b);                    \
        }                                                                   \
    } while (0)

#define ASSERT_NOT_NULL(p)                                                 \
    do {                                                                    \
        if ((p) != NULL) { ++g_pass; }                                     \
        else { ++g_fail;                                                   \
            fprintf(stderr, "FAIL: %s:%d  %s != NULL\n", __FILE__, __LINE__, #p); \
        }                                                                   \
    } while (0)

/* --- tests ---------------------------------------------------------- */

static void test_create_destroy(void) {
    arena *a = arena_create(4096);
    ASSERT_NOT_NULL(a);
    ASSERT_EQ_SIZE(arena_size(a), 4096);
    ASSERT_EQ_SIZE(arena_in_use(a), 0);
    arena_destroy(a);
}

static void test_null_safety(void) {
    arena_destroy(NULL);
    ASSERT_EQ_SIZE(arena_size(NULL), 0);
    ASSERT_EQ_SIZE(arena_in_use(NULL), 0);
    ASSERT_TRUE(arena_alloc(NULL, 16) == NULL);
    arena_free(NULL, NULL);
    arena_reset(NULL);
}

static void test_alloc(void) {
    arena *a = arena_create(4096);
    void *p1 = arena_alloc(a, 16);
    void *p2 = arena_alloc(a, 32);
    void *p3 = arena_alloc(a, 64);
    ASSERT_NOT_NULL(p1);
    ASSERT_NOT_NULL(p2);
    ASSERT_NOT_NULL(p3);
    /* Each allocation must be 16-byte aligned. */
    ASSERT_EQ_SIZE(((uintptr_t)p1) & 15, 0);
    ASSERT_EQ_SIZE(((uintptr_t)p2) & 15, 0);
    ASSERT_EQ_SIZE(((uintptr_t)p3) & 15, 0);
    /* In-use should be the sum of (rounded-up) sizes. */
    ASSERT_EQ_SIZE(arena_in_use(a), 16 + 32 + 64);
    arena_destroy(a);
}

static void test_alloc_beyond_capacity(void) {
    arena *a = arena_create(4096);
    ASSERT_NOT_NULL(arena_alloc(a, 2048));
    ASSERT_NOT_NULL(arena_alloc(a, 2048));
    /* Next one doesn't fit. */
    ASSERT_TRUE(arena_alloc(a, 1) == NULL);
    /* But the original allocations are still valid. */
    arena_reset(a);
    ASSERT_NOT_NULL(arena_alloc(a, 4096));
    arena_destroy(a);
}

static void test_free_is_noop(void) {
    arena *a = arena_create(64);
    void *p = arena_alloc(a, 32);
    ASSERT_NOT_NULL(p);
    size_t before = arena_in_use(a);
    arena_free(a, p);
    /* Free is a no-op: in_use unchanged. */
    ASSERT_EQ_SIZE(arena_in_use(a), before);
    arena_destroy(a);
}

static void test_reset(void) {
    arena *a = arena_create(4096);
    for (int i = 0; i < 10; i++) {
        void *p = arena_alloc(a, 16);
        ASSERT_NOT_NULL(p);
        memset(p, i & 0xff, 16);
    }
    ASSERT_EQ_SIZE(arena_in_use(a), 160);
    arena_reset(a);
    ASSERT_EQ_SIZE(arena_in_use(a), 0);
    /* After reset we can allocate again. */
    void *p = arena_alloc(a, 4096);
    ASSERT_NOT_NULL(p);
    arena_destroy(a);
}

static void test_high_watermark(void) {
    arena *a = arena_create(4096);
    /* 100 aligns up to 112, 200 aligns up to 208. Total = 320. */
    arena_alloc(a, 100);
    arena_alloc(a, 200);
    ASSERT_EQ_SIZE(arena_high_wat(a), 320);
    arena_reset(a);
    ASSERT_EQ_SIZE(arena_in_use(a), 0);
    /* High-watermark persists across reset. */
    ASSERT_EQ_SIZE(arena_high_wat(a), 320);
    arena_destroy(a);
}

static void test_remaining(void) {
    arena *a = arena_create(4096);
    ASSERT_EQ_SIZE(arena_remaining(a), 4096);
    arena_alloc(a, 256);
    ASSERT_EQ_SIZE(arena_remaining(a), 4096 - 256);
    arena_alloc(a, 3840);
    ASSERT_EQ_SIZE(arena_remaining(a), 0);
    arena_destroy(a);
}

static void test_repeated_phases(void) {
    arena *a = arena_create(4096);

    for (int phase = 0; phase < 5; phase++) {
        int n = 10 + phase * 5;
        void *ptrs[64];
        for (int i = 0; i < n; i++) {
            ptrs[i] = arena_alloc(a, 32);
            ASSERT_NOT_NULL(ptrs[i]);
        }
        for (int i = 0; i < n; i++) {
            memset(ptrs[i], phase, 32);
        }
        arena_reset(a);
        ASSERT_EQ_SIZE(arena_in_use(a), 0);
    }
    arena_destroy(a);
}

/* --- main ---------------------------------------------------------- */

int main(void) {
    fprintf(stderr, "[test_arena] starting\n");
    test_create_destroy();
    fprintf(stderr, "[test_arena] test_create_destroy done\n");
    test_null_safety();
    fprintf(stderr, "[test_arena] test_null_safety done\n");
    test_alloc();
    test_alloc_beyond_capacity();
    test_free_is_noop();
    test_reset();
    test_high_watermark();
    test_remaining();
    test_repeated_phases();

    printf("\n%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
