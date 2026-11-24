/* Whom to wake? signal versus broadcast. mem_alloc(n) waits if there are not enough free
 * bytes. With avail = 0, T1 asks for 100 and T2 for 10 bytes and both wait. If mem_free(50)
 * woke only T1 with signal, T1 would wait again and T2 (satisfiable) would keep sleeping.
 * broadcast wakes both, each re-checks its own condition: T2 proceeds, T1 waits. */
#include <stdio.h>
#include <pthread.h>
#include <time.h>

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t c = PTHREAD_COND_INITIALIZER;

static void lock(pthread_mutex_t *x)   { pthread_mutex_lock(x); }
static void unlock(pthread_mutex_t *x) { pthread_mutex_unlock(x); }
static void broadcast(pthread_cond_t *k) { pthread_cond_broadcast(k); }

static int waiting = 0;
static void wait_c(pthread_cond_t *k, pthread_mutex_t *x)
{
    waiting++;
    pthread_cond_wait(k, x);
    waiting--;
}
#define wait wait_c                /* do not clash with <sys/wait.h> wait() */

int avail = 0;            /* free bytes */

void mem_alloc(int n)
{
    lock(&m);
    while (avail < n)
        wait(&c, &m);
    avail -= n;  /* ... allocate */
    unlock(&m);
}

void mem_free(int n)
{
    lock(&m);
    avail += n;
    broadcast(&c); /* signal won't do */
    unlock(&m);
}

static void *T(void *arg)
{
    int n = *(int *)arg;
    mem_alloc(n);
    return NULL;
}

static void wait_for_waiting(int k)
{
    for (;;) {
        lock(&m);
        int w = waiting;
        unlock(&m);
        if (w == k)
            return;
        struct timespec ts = { 0, 1000000 };
        nanosleep(&ts, NULL);
    }
}

static int read_avail(void)
{
    lock(&m);
    int a = avail;
    unlock(&m);
    return a;
}

int main(void)
{
    pthread_t t1, t2;
    int n1 = 100, n2 = 10;
    pthread_create(&t1, NULL, T, &n1);
    wait_for_waiting(1);
    pthread_create(&t2, NULL, T, &n2);
    wait_for_waiting(2);
    printf("avail = %d: T1 (100) and T2 (10) are waiting\n", read_avail());

    mem_free(50);
    pthread_join(t2, NULL);             /* thanks to broadcast, T2 proceeds */
    wait_for_waiting(1);                /* T1 went back to sleep */
    printf("mem_free(50) -> T2 got 10 bytes, T1 is still waiting, avail = %d\n", read_avail());

    mem_free(60);
    pthread_join(t1, NULL);
    printf("mem_free(60) -> T1 got 100 bytes, avail = %d\n", read_avail());
    return 0;
}

/* Expected output (same on every run):
 * avail = 0: T1 (100) and T2 (10) are waiting
 * mem_free(50) -> T2 got 10 bytes, T1 is still waiting, avail = 40
 * mem_free(60) -> T1 got 100 bytes, avail = 0
 */
