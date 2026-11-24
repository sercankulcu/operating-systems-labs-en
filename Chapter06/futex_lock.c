/* A mutex built on the Linux futex (Drepper, "Futexes Are Tricky"): the lock word is
 * 0 = free, 1 = locked, 2 = locked and there are waiters. Without contention one CAS is
 * enough and the kernel is not entered; if locked, the thread sleeps with futex_wait and
 * the releaser calls futex_wake. */
#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>

#define NUM_THREADS 4
#define ROUNDS 100000

static atomic_int lock_word = 0;
static atomic_long wait_count = 0, wake_count = 0;

static long futex(atomic_int *addr, int op, int val)
{
    return syscall(SYS_futex, (int *)addr, op, val, NULL, NULL, 0);
}

static void futex_lock(atomic_int *w)
{
    int c = 0;
    if (atomic_compare_exchange_strong(w, &c, 1))
        return;                              /* CAS 0 -> 1: no system call */
    if (c != 2)
        c = atomic_exchange(w, 2);           /* mark "there are waiters" */
    while (c != 0) {
        atomic_fetch_add_explicit(&wait_count, 1, memory_order_relaxed);
        futex(w, FUTEX_WAIT_PRIVATE, 2);     /* sleep if the value is still 2 */
        c = atomic_exchange(w, 2);           /* woke up: if the old value was 0 we hold the lock */
    }
}

static void futex_unlock(atomic_int *w)
{
    if (atomic_fetch_sub(w, 1) != 1) {       /* old value 2: there are waiters */
        atomic_store(w, 0);
        atomic_fetch_add_explicit(&wake_count, 1, memory_order_relaxed);
        futex(w, FUTEX_WAKE_PRIVATE, 1);     /* wake one */
    }
}

static long counter = 0;

static void *worker(void *arg)
{
    (void)arg;
    for (int k = 0; k < ROUNDS; k++) {
        futex_lock(&lock_word);
        counter++;
        futex_unlock(&lock_word);
    }
    return NULL;
}

int main(void)
{
    /* no contention: a single thread, no futex calls at all */
    futex_lock(&lock_word);
    futex_unlock(&lock_word);
    printf("uncontended: futex_wait = %ld, futex_wake = %ld\n",
           atomic_load(&wait_count), atomic_load(&wake_count));

    pthread_t t[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_create(&t[i], NULL, worker, NULL);
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(t[i], NULL);
    printf("counter = %ld\n", counter);
    printf("contended: futex_wait = %ld, futex_wake = %ld (varies)\n",
           atomic_load(&wait_count), atomic_load(&wake_count));
    return 0;
}

/* Expected output (the numbers on the last line depend on timing):
 * uncontended: futex_wait = 0, futex_wake = 0
 * counter = 400000
 * contended: futex_wait = N, futex_wake = M (varies)
 */
