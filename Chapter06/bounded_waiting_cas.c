/* Bounded waiting with CAS: instead of releasing the lock, the exiting process hands the turn to
 * the first process waiting in the ring (i+1, i+2, ...); so no process waits more than n-1
 * turns. First the trace from the slide (n = 4; P2 inside, P0 and P3 waiting) is played with
 * real threads, the main thread controlling the order; then a free run. */
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sched.h>

#define n 4
#define ROUNDS 10000

atomic_bool waiting[n];
atomic_int  lock = 0;

int compare_and_swap(atomic_int *v, int exp, int nv)
{
    int old = exp;
    (void)atomic_compare_exchange_strong(v, &old, nv);
    return old;
}

/* for the trace: the main thread gives permission to leave the critical section */
static bool stepped = false;
static atomic_bool go_on[n];
static atomic_int  cs_owner = -1;
static char cs_order[16];
static int  cs_n = 0;

static long counter = 0;
static atomic_int inside = 0, violations = 0;

static void spin_pause(unsigned *spins)
{
    if (++*spins % 1024 == 0)
        sched_yield();
}

static void critical_section(int i)
{
    if (atomic_fetch_add(&inside, 1) != 0)
        atomic_fetch_add(&violations, 1);
    counter++;
    if (stepped) {
        cs_order[cs_n++] = (char)('0' + i);
        atomic_store(&cs_owner, i);
        unsigned d = 0;
        while (!atomic_load(&go_on[i]))
            spin_pause(&d);
    }
    atomic_fetch_sub(&inside, 1);
}

static void process(int i, int rounds)
{
    int key, j;
    for (int k = 0; k < rounds; k++) {       /* bounded rounds instead of while (true) */
        unsigned d = 0;
        atomic_store(&waiting[i], true);
        key = 1;
        while (atomic_load(&waiting[i]) && key == 1) {
            key = compare_and_swap(&lock, 0, 1);
            if (key == 1)
                spin_pause(&d);
        }
        atomic_store(&waiting[i], false);
        critical_section(i);                 /* critical section */
        j = (i + 1) % n;
        while ((j != i) && !atomic_load(&waiting[j]))
            j = (j + 1) % n;
        if (j == i)
            atomic_store(&lock, 0);          /* no one waiting */
        else
            atomic_store(&waiting[j], false); /* hand over turn */
        /* remainder section */
    }
}

static int rounds_count;

static void *P(void *arg)
{
    process(*(int *)arg, rounds_count);
    return NULL;
}

static void wait_cs(int i)
{
    unsigned d = 0;
    while (atomic_load(&cs_owner) != i)
        spin_pause(&d);
}

static void wait_waiting(int i)
{
    unsigned d = 0;
    while (!atomic_load(&waiting[i]))
        spin_pause(&d);
}

int main(void)
{
    pthread_t t[n];
    int id[n] = { 0, 1, 2, 3 };

    /* 1) The trace from the slide: P2 enters; P0 and P3 wait; P2 -> P3 -> P0; lock = 0 */
    stepped = true;
    rounds_count = 1;
    pthread_create(&t[2], NULL, P, &id[2]);
    wait_cs(2);                            /* P2 is in the critical section */
    pthread_create(&t[0], NULL, P, &id[0]);
    wait_waiting(0);                       /* waiting = [T F F F] */
    pthread_create(&t[3], NULL, P, &id[3]);
    wait_waiting(3);                       /* waiting = [T F F T] */
    atomic_store(&go_on[2], true);         /* P2 leaves: j = 3 -> waiting[3] = false */
    wait_cs(3);
    atomic_store(&go_on[3], true);         /* P3 leaves: j = 0 -> waiting[0] = false */
    wait_cs(0);
    atomic_store(&go_on[0], true);         /* P0 leaves: no one waiting -> lock = 0 */
    pthread_join(t[2], NULL);
    pthread_join(t[3], NULL);
    pthread_join(t[0], NULL);
    cs_order[cs_n] = '\0';
    printf("critical section order:");
    for (int k = 0; k < cs_n; k++)
        printf(" P%c", cs_order[k]);
    printf("\nlock = %d\n", atomic_load(&lock));

    /* 2) Free run: 4 processes x ROUNDS */
    stepped = false;
    rounds_count = ROUNDS;
    counter = 0;
    for (int i = 0; i < n; i++)
        pthread_create(&t[i], NULL, P, &id[i]);
    for (int i = 0; i < n; i++)
        pthread_join(t[i], NULL);
    printf("4 processes x %d: counter = %ld, violations = %d, lock = %d\n",
           ROUNDS, counter, atomic_load(&violations), atomic_load(&lock));
    return 0;
}

/* Expected output (same on every run):
 * critical section order: P2 P3 P0
 * lock = 0
 * 4 processes x 10000: counter = 40000, violations = 0, lock = 0
 */
