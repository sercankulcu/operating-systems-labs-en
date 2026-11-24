/* A common bug: a thread must not keep its result on its own stack and
 * return the address; the frame disappears when the function returns. The
 * correct version allocates the result on the heap. The buggy version is not
 * compiled: gcc -Wreturn-local-addr warns about it. */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct { long total; int count; } result_t;

#if 0   /* BUGGY VERSION: undefined behavior if compiled */
void *add_range(void *arg) {
  result_t s = { 0, 0 };  /* on T's stack */
  /* ... fill in s ... */
  return &s;            /* BUG */
}
#endif

/* Correct version: adds 1..upper. */
void *add_range(void *arg)
{
    int upper = *(int *)arg;
    result_t *s = malloc(sizeof *s);
    if (s == NULL)
        return NULL;
    /* ... fill in s ... */
    s->total = 0; s->count = 0;
    for (int i = 1; i <= upper; i++) {
        s->total += i; s->count++;
    }
    return s;    /* on heap: lives on */
}

int main(void)
{
    int upper = 10;
    pthread_t t;
    void *ret;
    pthread_create(&t, NULL, add_range, &upper);
    pthread_join(t, &ret);
    result_t *s = ret;
    if (s == NULL)
        return 1;
    printf("%d numbers, total %ld\n", s->count, s->total);
    free(s);
    return 0;
}

/* Expected output:
 * 10 numbers, total 55
 */
