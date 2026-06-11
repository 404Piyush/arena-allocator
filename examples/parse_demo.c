/* examples/parse_demo.c
 *
 * A tiny "parse a line into tokens" demo. Tokens are allocated
 * from an arena.  At the end of the parse, arena_reset() throws
 * them all away in O(1) — no per-token free.
 */
#include "arena.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Return an array of token pointers, terminated by NULL. */
static char **tokenize(arena *a, const char *line) {
    size_t cap = 8, n = 0;
    char **toks = (char **)arena_alloc(a, cap * sizeof(char *));
    const char *p = line;
    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;
        const char *start = p;
        while (*p && !isspace((unsigned char)*p)) p++;
        size_t len = (size_t)(p - start);
        char *tok = (char *)arena_alloc(a, len + 1);
        memcpy(tok, start, len);
        tok[len] = '\0';
        if (n == cap) {
            /* Grow the token array in the same arena. */
            size_t newcap = cap * 2;
            char **grown = (char **)arena_alloc(a, newcap * sizeof(char *));
            memcpy(grown, toks, n * sizeof(char *));
            toks = grown;
            cap = newcap;
        }
        toks[n++] = tok;
    }
    if (n == cap) {
        char **grown = (char **)arena_alloc(a, (cap + 1) * sizeof(char *));
        memcpy(grown, toks, n * sizeof(char *));
        toks = grown;
        cap = cap + 1;
    }
    toks[n] = NULL;
    return toks;
}

int main(int argc, char **argv) {
    const char *line = (argc > 1) ? argv[1]
                                  : "the quick brown  fox jumps over the lazy dog";
    arena *a = arena_create(4096);

    char **toks = tokenize(a, line);
    printf("parsed %zu tokens:\n", 0);  /* placeholder, real count below */
    size_t n = 0;
    for (char **t = toks; *t; t++) {
        printf("  [%zu] %s\n", n++, *t);
    }
    printf("total tokens: %zu (arena in-use = %zu bytes)\n", n, arena_in_use(a));

    /* Throw away all tokens at once. */
    arena_reset(a);
    printf("after reset: arena in-use = %zu bytes\n", arena_in_use(a));

    arena_destroy(a);
    return 0;
}
