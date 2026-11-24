/* Atomic hardware instructions: a simple spinlock with test_and_set, and the C11 equivalent of
 * enter_region / leave_region written with the TSL / XCHG instructions.
 * Two threads each increment a counter 100000 times in the critical section. */
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sched.h>

#define ROUNDS 100000

/* The definition on the slide (all of it is ONE atomic instruction):
 *     bool old = *target;  *target = true;  return old;
 * In C11 this single instruction is atomic_exchange (x86: xchg, ARM: ldaxr/stlxr or swp). */
bool test_and_set(atomic_bool *target)
{
    return atomic_exchange(target, true);
}

/* usage: a simple spinlock */
atomic_bool lock = false;

static long counter1 = 0;

static void *spin_worker(void *arg)
{
    (void)arg;
    for (int k = 0; k < ROUNDS; k++) {
        unsigned spins = 0;
        while (test_and_set(&lock))
            if (++spins % 1024 == 0)     /* busy wait */
                sched_yield();
        counter1++;                      /* critical section */
        atomic_store(&lock, false);      /* lock = false; */
    }
    return NULL;
}

/* TSL REGISTER,LOCK or MOVE REGISTER,#1 + XCHG REGISTER,LOCK:
 * register = LOCK; LOCK = 1 in one instruction. CMP REGISTER,#0; JNE enter_region. */
static atomic_int LOCK = 0;
static long counter2 = 0;

void enter_region(void)
{
    int REGISTER;
    unsigned spins = 0;
    do {
        REGISTER = atomic_exchange(&LOCK, 1);   /* XCHG REGISTER,LOCK */
        if (REGISTER != 0 && ++spins % 1024 == 0)
            sched_yield();
    } while (REGISTER != 0);                    /* was LOCK 0? if not, try again */
}                                               /* entered the critical section */

void leave_region(void)
{
    atomic_store(&LOCK, 0);                     /* MOVE LOCK,#0 */
}

static void *tsl_worker(void *arg)
{
    (void)arg;
    for (int k = 0; k < ROUNDS; k++) {
        enter_region();
        counter2++;
        leave_region();
    }
    return NULL;
}

static void two_workers(void *(*f)(void *))
{
    pthread_t t[2];
    for (int i = 0; i < 2; i++)
        pthread_create(&t[i], NULL, f, NULL);
    for (int i = 0; i < 2; i++)
        pthread_join(t[i], NULL);
}

int main(void)
{
    two_workers(spin_worker);
    printf("test_and_set spinlock: counter = %ld\n", counter1);
    two_workers(tsl_worker);
    printf("enter_region (XCHG)  : counter = %ld\n", counter2);
    return 0;
}

/* Expected output (same on every run):
 * test_and_set spinlock: counter = 200000
 * enter_region (XCHG)  : counter = 200000
 */
