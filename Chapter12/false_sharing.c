/* False sharing: two threads increment two independent counters. If the counters are
 * in the same 64-byte cache line the line bounces between the cores; with
 * _Alignas(64) they go to separate lines and the program speeds up. */
#include <stdio.h>
#include <stddef.h>
#include <pthread.h>
#include <time.h>

#define ROUNDS 20000000L

struct counters {      /* bad */
    long a;   /* offset 0 */
    long b;   /* offset 8 */
};

struct padded {        /* good */
    _Alignas(64) long a;
    _Alignas(64) long b;
};

static volatile struct counters s;     /* volatile: keep the compiler from removing */
static volatile struct padded d;       /* the increments inside the loop */

static void *s_a(void *x) { (void)x; for (long i = 0; i < ROUNDS; i++) s.a++; return NULL; }
static void *s_b(void *x) { (void)x; for (long i = 0; i < ROUNDS; i++) s.b++; return NULL; }
static void *d_a(void *x) { (void)x; for (long i = 0; i < ROUNDS; i++) d.a++; return NULL; }
static void *d_b(void *x) { (void)x; for (long i = 0; i < ROUNDS; i++) d.b++; return NULL; }

static double run(void *(*f0)(void *), void *(*f1)(void *))
{
    struct timespec t0, t1;
    pthread_t p0, p1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    pthread_create(&p0, NULL, f0, NULL);   /* thread 0: for(..) s.a++; */
    pthread_create(&p1, NULL, f1, NULL);   /* thread 1: for(..) s.b++; */
    pthread_join(p0, NULL);
    pthread_join(p1, NULL);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    return (double)(t1.tv_sec - t0.tv_sec) * 1e3 + (double)(t1.tv_nsec - t0.tv_nsec) / 1e6;
}

int main(void)
{
    printf("counters: offset of b %zu, size %zu\n",
           offsetof(struct counters, b), sizeof(struct counters));
    printf("padded:   offset of b %zu, size %zu\n",
           offsetof(struct padded, b), sizeof(struct padded));

    double bad = run(s_a, s_b);
    double good = run(d_a, d_b);
    printf("same line:      %.1f ms (a = %ld, b = %ld)\n", bad, s.a, s.b);
    printf("separate lines: %.1f ms (a = %ld, b = %ld)\n", good, d.a, d.b);
    return 0;
}

/* Expected output (times depend on the machine; on a multicore machine
 * the same line is usually several times slower):
 * counters: offset of b 8, size 16
 * padded:   offset of b 64, size 128
 * same line:      180.0 ms (a = 20000000, b = 20000000)
 * separate lines: 45.0 ms (a = 20000000, b = 20000000)
 */
