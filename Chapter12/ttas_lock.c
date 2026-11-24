/* TTAS (test-and-test-and-set) lock: a waiter first only reads the lock (spinning on
 * its own cache copy) and tries TSL when it sees the lock free. tsl() is implemented
 * with C11 atomic_exchange. Three threads increment a shared counter. */
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>

#define NTHR 3
#define ROUNDS 100000

static int tsl(atomic_int *k)
{
    return atomic_exchange(k, 1);     /* return the old value, write 1 */
}

void lock(atomic_int *k)
{
    for (;;) {
        while (*k == 1)   /* spin locally */
            ;
        if (tsl(k) == 0)  /* looked free */
            return;       /* acquired */
    }
}

void unlock(atomic_int *k)
{
    *k = 0;
}

static atomic_int spin = 0;
static long counter = 0;

static void *worker(void *arg)
{
    (void)arg;
    for (int i = 0; i < ROUNDS; i++) {
        lock(&spin);
        counter++;
        unlock(&spin);
    }
    return NULL;
}

int main(void)
{
    pthread_t t[NTHR];
    for (int i = 0; i < NTHR; i++)
        pthread_create(&t[i], NULL, worker, NULL);
    for (int i = 0; i < NTHR; i++)
        pthread_join(t[i], NULL);
    printf("counter = %ld (expected %d)\n", counter, NTHR * ROUNDS);
    return 0;
}

/* Expected output:
 * counter = 300000 (expected 300000)
 */
