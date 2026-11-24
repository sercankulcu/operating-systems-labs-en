/* Scalable (approximate) counter: each CPU has its own local counter and lock; when the local
 * value reaches the threshold S it is transferred to the global counter. get() is fast but
 * approximate: it lags by at most NCPU * (S - 1). First the table from the slide
 * (NCPU = 4, S = 4) is played, then 4 threads really count. */
#include <stdio.h>
#include <pthread.h>

#define NCPU 4

typedef struct {
    int global;      pthread_mutex_t glock;
    int local[NCPU]; pthread_mutex_t llock[NCPU];
    int S;                     /* transfer threshold */
} acount_t;

static void acount_init(acount_t *c, int S)
{
    c->global = 0;
    pthread_mutex_init(&c->glock, NULL);
    for (int i = 0; i < NCPU; i++) {
        c->local[i] = 0;
        pthread_mutex_init(&c->llock[i], NULL);
    }
    c->S = S;
}

static void acount_destroy(acount_t *c)
{
    pthread_mutex_destroy(&c->glock);
    for (int i = 0; i < NCPU; i++)
        pthread_mutex_destroy(&c->llock[i]);
}

void add(acount_t *c, int cpu)
{
    pthread_mutex_lock(&c->llock[cpu]);
    c->local[cpu]++;
    if (c->local[cpu] >= c->S) {        /* transfer */
        pthread_mutex_lock(&c->glock);
        c->global += c->local[cpu];
        pthread_mutex_unlock(&c->glock);
        c->local[cpu] = 0;
    }
    pthread_mutex_unlock(&c->llock[cpu]);
}

int get(acount_t *c)                    /* approximate */
{
    pthread_mutex_lock(&c->glock);
    int v = c->global;
    pthread_mutex_unlock(&c->glock);
    return v;
}

static int actual;

static void print_row(acount_t *c, int t)
{
    printf("%d   %2d %2d %2d %2d   %2d   %2d\n", t, c->local[0], c->local[1],
           c->local[2], c->local[3], c->global, actual);
}

/* in one time step the given CPUs call add() (1-based, 0 = end) */
static void step(acount_t *c, int t, const int *cpus)
{
    for (const int *p = cpus; *p != 0; p++) {
        add(c, *p - 1);
        actual++;
    }
    print_row(c, t);
}

static acount_t shared;
#define ROUNDS 100000

static void *worker(void *arg)
{
    int cpu = *(int *)arg;
    for (int k = 0; k < ROUNDS; k++)
        add(&shared, cpu);
    return NULL;
}

int main(void)
{
    acount_t c;
    acount_init(&c, 4);
    printf("t   L1 L2 L3 L4    G  actual\n");
    print_row(&c, 0);
    static const int t1[] = { 1, 3, 0 };       /* CPUs 1 and 3 */
    static const int t2[] = { 1, 2, 3, 0 };
    static const int t3[] = { 1, 3, 4, 0 };
    static const int t4[] = { 1, 2, 0 };       /* L1 reaches the threshold: G = 4 */
    static const int t5[] = { 3, 4, 0 };       /* L3 reaches the threshold: G = 8 */
    step(&c, 1, t1);
    step(&c, 2, t2);
    step(&c, 3, t3);
    step(&c, 4, t4);
    step(&c, 5, t5);
    printf("get() = %d, actual = %d, worst-case gap NCPU*(S-1) = %d\n",
           get(&c), actual, NCPU * (c.S - 1));
    acount_destroy(&c);

    /* 4 threads, S = 1024 */
    pthread_t t[NCPU];
    int id[NCPU];
    acount_init(&shared, 1024);
    for (int i = 0; i < NCPU; i++) {
        id[i] = i;
        pthread_create(&t[i], NULL, worker, &id[i]);
    }
    for (int i = 0; i < NCPU; i++)
        pthread_join(t[i], NULL);
    int remaining = 0;
    for (int i = 0; i < NCPU; i++)
        remaining += shared.local[i];
    printf("S = 1024: get() = %d, left in the locals = %d, total = %d\n",
           get(&shared), remaining, get(&shared) + remaining);
    acount_destroy(&shared);
    return 0;
}

/* Expected output (same on every run):
 * t   L1 L2 L3 L4    G  actual
 * 0    0  0  0  0    0    0
 * 1    1  0  1  0    0    2
 * 2    2  1  2  0    0    5
 * 3    3  1  3  1    0    8
 * 4    0  2  3  1    4   10
 * 5    0  2  0  2    8   12
 * get() = 8, actual = 12, worst-case gap NCPU*(S-1) = 12
 * S = 1024: get() = 397312, left in the locals = 2688, total = 400000
 * (each CPU transfers 1024 at a time, 97 times: 99328; 672 stay local)
 */
