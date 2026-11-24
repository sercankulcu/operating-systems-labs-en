/* Memory models and reordering: the "store buffer" litmus test.
 * x = y = 0;  T1: x = 1; r1 = y;   T2: y = 1; r2 = x;
 * Under sequential consistency r1 == 0 && r2 == 0 is impossible; even x86 can produce it
 * with relaxed accesses. With seq_cst (a memory barrier) it is never seen. */
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sched.h>

#define TRIALS 50000

static atomic_int x, y;
static int r1, r2;                 /* read by the main thread at the end of each round */
static bool with_barrier;

static atomic_int round_no = -1;   /* the main thread starts the round */
static atomic_int finished = 0;    /* number of threads that finished that round */

static void wait_round(int k)
{
    unsigned spins = 0;
    while (atomic_load_explicit(&round_no, memory_order_acquire) != k)
        if (++spins % 1024 == 0)
            sched_yield();
}

static void *T1(void *arg)
{
    (void)arg;
    for (int k = 0; k < TRIALS; k++) {
        wait_round(k);
        if (with_barrier) {
            atomic_store_explicit(&x, 1, memory_order_seq_cst);
            r1 = atomic_load_explicit(&y, memory_order_seq_cst);
        } else {
            atomic_store_explicit(&x, 1, memory_order_relaxed);
            r1 = atomic_load_explicit(&y, memory_order_relaxed);
        }
        atomic_fetch_add_explicit(&finished, 1, memory_order_release);
    }
    return NULL;
}

static void *T2(void *arg)
{
    (void)arg;
    for (int k = 0; k < TRIALS; k++) {
        wait_round(k);
        if (with_barrier) {
            atomic_store_explicit(&y, 1, memory_order_seq_cst);
            r2 = atomic_load_explicit(&x, memory_order_seq_cst);
        } else {
            atomic_store_explicit(&y, 1, memory_order_relaxed);
            r2 = atomic_load_explicit(&x, memory_order_relaxed);
        }
        atomic_fetch_add_explicit(&finished, 1, memory_order_release);
    }
    return NULL;
}

static int run(bool b)
{
    pthread_t a, c;
    int both_zero = 0;
    with_barrier = b;
    atomic_store(&round_no, -1);
    atomic_store(&finished, 0);
    pthread_create(&a, NULL, T1, NULL);
    pthread_create(&c, NULL, T2, NULL);
    for (int k = 0; k < TRIALS; k++) {
        atomic_store(&x, 0);                 /* x = y = 0 */
        atomic_store(&y, 0);
        atomic_store_explicit(&round_no, k, memory_order_release);
        unsigned spins = 0;
        while (atomic_load_explicit(&finished, memory_order_acquire) != 2 * (k + 1))
            if (++spins % 1024 == 0)
                sched_yield();
        if (r1 == 0 && r2 == 0)              /* r1 == 0 && r2 == 0 ? */
            both_zero++;
    }
    pthread_join(a, NULL);
    pthread_join(c, NULL);
    return both_zero;
}

int main(void)
{
    printf("r1 == 0 && r2 == 0 in %d trials:\n", TRIALS);
    printf("relaxed (no barrier)  : %d times\n", run(false));
    printf("seq_cst (with barrier): %d times\n", run(true));
    return 0;
}

/* Expected output (x86-64; the first number varies on every run and may be 0;
 * the second number is always 0):
 * r1 == 0 && r2 == 0 in 50000 trials:
 * relaxed (no barrier)  : N times
 * seq_cst (with barrier): 0 times
 */
