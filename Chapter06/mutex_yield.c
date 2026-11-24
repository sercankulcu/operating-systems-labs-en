/* Mutex implementation (Tanenbaum Figure 2-29): try the lock with TSL, and if it is taken
 * give up the CPU with thread_yield instead of spinning. Four threads increment a counter
 * under the same mutex; the number of thread_yield calls is also counted. */
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sched.h>

#define NUM_THREADS 4
#define ROUNDS 50000

static atomic_int MUTEX = 0;
static atomic_long yield_count = 0;

static void thread_yield(void)
{
    atomic_fetch_add_explicit(&yield_count, 1, memory_order_relaxed);
    sched_yield();
}

void mutex_lock(void)
{
    for (;;) {
        int REGISTER = atomic_exchange(&MUTEX, 1);  /* TSL REGISTER,MUTEX */
        if (REGISTER == 0)                          /* CMP REGISTER,#0 / JZE ok */
            return;                                 /* ok: RET */
        thread_yield();                             /* CALL thread_yield */
    }                                               /* JMP mutex_lock */
}

void mutex_unlock(void)
{
    atomic_store(&MUTEX, 0);                        /* MOVE MUTEX,#0 */
}

static long counter = 0;

static void *worker(void *arg)
{
    (void)arg;
    for (int k = 0; k < ROUNDS; k++) {
        mutex_lock();
        counter++;
        mutex_unlock();
    }
    return NULL;
}

int main(void)
{
    pthread_t t[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_create(&t[i], NULL, worker, NULL);
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(t[i], NULL);
    printf("counter = %ld\n", counter);
    printf("thread_yield calls = %ld (varies)\n", atomic_load(&yield_count));
    return 0;
}

/* Expected output (the second number varies with timing):
 * counter = 200000
 * thread_yield calls = N (varies)
 */
