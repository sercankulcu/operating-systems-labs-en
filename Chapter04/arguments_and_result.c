/* Passing arguments and getting a result: several values are put in a struct
 * and its address is passed; the result is allocated on the heap and handed
 * to main through pthread_join. */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct { int lo, hi; } range_t;
typedef struct { long total; int count; } result_t;

void *add_range(void *arg)
{
    range_t *a = arg;           /* void* -> type */
    result_t *s = malloc(sizeof *s); /* on heap */
    if (s == NULL)
        return NULL;
    s->total = 0; s->count = 0;
    for (int i = a->lo; i <= a->hi; i++) {
        s->total += i; s->count++;
    }
    return s;         /* = pthread_exit(s) */
}

int main(void)
{
    range_t a = { 1, 100 };
    pthread_t t;  void *ret;
    pthread_create(&t, NULL, add_range, &a);
    pthread_join(t, &ret);          /* ret = s */
    result_t *s = ret;
    if (s == NULL)
        return 1;
    printf("%d numbers, total %ld\n", s->count, s->total);
    free(s);
    return 0;
}

/* Expected output:
 * 100 numbers, total 5050
 */
