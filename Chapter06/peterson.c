/* Peterson's algorithm: mutual exclusion for two processes with plain reads/writes only.
 * flag and turn are defined as C11 atomics (default seq_cst); this prevents reordering on a
 * modern CPU and the algorithm works correctly. */
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sched.h>

#define ROUNDS 100000

/* shared */
atomic_bool flag[2] = { false, false };
atomic_int  turn;

static long counter = 0;              /* data protected by the critical section */

void enter(int i)
{
    int j = 1 - i;
    atomic_store(&flag[i], true);     /* I want in */
    atomic_store(&turn, j);           /* after you */
    unsigned spins = 0;
    while (atomic_load(&flag[j]) && atomic_load(&turn) == j) {
        if (++spins % 1024 == 0)      /* busy wait; give up the CPU on few cores */
            sched_yield();
    }
}

void leave(int i)
{
    atomic_store(&flag[i], false);
}

static void *P(void *arg)
{
    int i = *(int *)arg;              /* P_i  (j = 1 - i) */
    for (int k = 0; k < ROUNDS; k++) {   /* ROUNDS rounds instead of while (true) */
        enter(i);
        counter++;                    /* critical section */
        leave(i);
        /* remainder section */
    }
    return NULL;
}

int main(void)
{
    pthread_t t[2];
    int id[2] = { 0, 1 };
    atomic_init(&turn, 0);
    for (int i = 0; i < 2; i++)
        pthread_create(&t[i], NULL, P, &id[i]);
    for (int i = 0; i < 2; i++)
        pthread_join(t[i], NULL);
    printf("counter = %ld\n", counter);
    return 0;
}

/* Expected output (same on every run):
 * counter = 200000
 */
