/* Why a lock variable is not enough: "test" and "set" are separate steps, so two threads
 * can both see lock == 0 and enter the critical section together.
 * The critical section problem skeleton: entry section, critical section, exit, remainder section. */
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>

#define ROUNDS 200000

/* Shared lock. Reads and writes are each atomic (for defined behaviour),
 * but "read, then write 1" together is NOT atomic: the same bug as on the slide. */
atomic_int lock = 0;            /* shared */

static atomic_int inside = 0;      /* number of threads in the critical section */
static atomic_int violations = 0;  /* was anyone seen inside together with another */
static long counter = 0;           /* the data that must be protected */

void enter(void)
{
    while (atomic_load_explicit(&lock, memory_order_relaxed) != 0)
        ;                       /* busy wait */
    atomic_store_explicit(&lock, 1, memory_order_relaxed);
}

void leave(void)
{
    atomic_store_explicit(&lock, 0, memory_order_relaxed);
}

static void critical_section(void)
{
    if (atomic_fetch_add(&inside, 1) != 0)
        atomic_fetch_add(&violations, 1);
    counter++;                  /* deliberately unprotected: the race shows up here */
    atomic_fetch_sub(&inside, 1);
}

static void *process(void *arg)
{
    (void)arg;
    for (int k = 0; k < ROUNDS; k++) {   /* bounded rounds instead of while (true) */
        enter();                /* entry section */
        critical_section();     /* critical section */
        leave();                /* exit section */
        /* remainder section */
    }
    return NULL;
}

int main(void)
{
    pthread_t t[2];
    for (int i = 0; i < 2; i++)
        pthread_create(&t[i], NULL, process, NULL);
    for (int i = 0; i < 2; i++)
        pthread_join(t[i], NULL);

    printf("expected counter = %d\n", 2 * ROUNDS);
    printf("found counter    = %ld\n", counter);
    printf("mutual exclusion violations: %d times\n", atomic_load(&violations));
    return 0;
}

/* Expected output (race condition: the numbers vary on every run; on a multi-core
 * machine usually violations > 0 and counter < 400000, though by luck it may be 0):
 * expected counter = 400000
 * found counter    = 39xxxx
 * mutual exclusion violations: N times
 */
