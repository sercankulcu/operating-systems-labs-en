/* A lock built on compare_and_swap: "if old == exp write nv, return old" is one atomic
 * instruction. First the trace from the slide (T0 acquires, T1 spins, T0 releases, T1
 * acquires) is played with a single thread, then two threads increment a counter under the lock. */
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sched.h>

#define ROUNDS 100000

/* The body on the slide (the three lines together are ONE atomic instruction):
 *     int old = *v;  if (old == exp) *v = nv;  return old;
 * C11 equivalent: atomic_compare_exchange_strong (x86: lock cmpxchg).
 * On failure the current value is written to exp; so it always returns the old value. */
int compare_and_swap(atomic_int *v, int exp, int nv)
{
    int old = exp;
    (void)atomic_compare_exchange_strong(v, &old, nv);
    return old;
}

atomic_int lock = 0;

void enter(void)
{
    unsigned spins = 0;
    while (compare_and_swap(&lock, 0, 1) != 0)
        if (++spins % 1024 == 0)      /* busy wait */
            sched_yield();
}

void leave(void)
{
    atomic_store(&lock, 0);
}

static long counter = 0;

static void *worker(void *arg)
{
    (void)arg;
    for (int k = 0; k < ROUNDS; k++) {
        enter();
        counter++;                    /* critical section */
        leave();
    }
    return NULL;
}

int main(void)
{
    /* The trace from the slide */
    int old;
    printf("lock = %d\n", atomic_load(&lock));
    old = compare_and_swap(&lock, 0, 1);
    printf("T0: old = %d -> entered the critical section, lock = %d\n", old, atomic_load(&lock));
    old = compare_and_swap(&lock, 0, 1);
    printf("T1: old = %d -> spins (busy wait), lock = %d\n", old, atomic_load(&lock));
    leave();
    printf("T0: leave(), lock = %d\n", atomic_load(&lock));
    old = compare_and_swap(&lock, 0, 1);
    printf("T1: old = %d -> entered the critical section, lock = %d\n", old, atomic_load(&lock));
    leave();

    /* With real threads */
    pthread_t t[2];
    for (int i = 0; i < 2; i++)
        pthread_create(&t[i], NULL, worker, NULL);
    for (int i = 0; i < 2; i++)
        pthread_join(t[i], NULL);
    printf("two threads x %d: counter = %ld\n", ROUNDS, counter);
    return 0;
}

/* Expected output (same on every run):
 * lock = 0
 * T0: old = 0 -> entered the critical section, lock = 1
 * T1: old = 1 -> spins (busy wait), lock = 1
 * T0: leave(), lock = 0
 * T1: old = 0 -> entered the critical section, lock = 1
 * two threads x 100000: counter = 200000
 */
