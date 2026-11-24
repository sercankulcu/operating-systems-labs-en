/* Atomic increment with CAS: read, then "write if unchanged"; if it changed, retry.
 * First the trace from the slide (v = 5, T1 and T2 both read 5, T2 wins, T1 retries,
 * v = 7) is played step by step; then two threads call increment(). */
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>

#define ROUNDS 100000

/* The body on the slide (the three lines together are ONE atomic instruction):
 *     int old = *v;  if (old == exp) *v = nv;  return old;
 * GCC/Clang equivalent: __atomic_compare_exchange_n (x86: lock cmpxchg).
 * On failure the current value is written to old; either way it returns the old value. */
int compare_and_swap(int *v, int exp, int nv)
{
    int old = exp;
    (void)__atomic_compare_exchange_n(v, &old, nv, 0,
                                      __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return old;
}

void increment(int *v)
{
    int temp;
    do {
        temp = *v;                          /* read              */
    } while (temp != compare_and_swap(v, temp, temp + 1));
}                                           /* write if unchanged */

static int counter = 0;
static atomic_int counter2 = 0;

static void *worker(void *arg)
{
    (void)arg;
    for (int k = 0; k < ROUNDS; k++) {
        increment(&counter);
        atomic_fetch_add(&counter2, 1);     /* C11 one-liner */
    }
    return NULL;
}

int main(void)
{
    /* The trace from the slide, interleaved by hand */
    int v = 5;
    int t1_temp = v;                        /* T1 reads: 5 */
    int t2_temp = v;                        /* T2 reads: 5 */
    printf("v = %d, T1.temp = %d, T2.temp = %d\n", v, t1_temp, t2_temp);
    int r = compare_and_swap(&v, t2_temp, t2_temp + 1);
    printf("T2: CAS(v, %d, %d) old = %d -> success, v = %d\n",
           t2_temp, t2_temp + 1, r, v);
    r = compare_and_swap(&v, t1_temp, t1_temp + 1);
    printf("T1: CAS(v, %d, %d) old = %d -> failure, retry\n",
           t1_temp, t1_temp + 1, r);
    t1_temp = v;                            /* T1 reads again: 6 */
    r = compare_and_swap(&v, t1_temp, t1_temp + 1);
    printf("T1: CAS(v, %d, %d) old = %d -> success, v = %d\n",
           t1_temp, t1_temp + 1, r, v);

    pthread_t t[2];
    for (int i = 0; i < 2; i++)
        pthread_create(&t[i], NULL, worker, NULL);
    for (int i = 0; i < 2; i++)
        pthread_join(t[i], NULL);
    printf("increment()      : %d\n", counter);
    printf("atomic_fetch_add : %d\n", atomic_load(&counter2));
    return 0;
}

/* Expected output (same on every run):
 * v = 5, T1.temp = 5, T2.temp = 5
 * T2: CAS(v, 5, 6) old = 5 -> success, v = 6
 * T1: CAS(v, 5, 6) old = 6 -> failure, retry
 * T1: CAS(v, 6, 7) old = 6 -> success, v = 7
 * increment()      : 200000
 * atomic_fetch_add : 200000
 */
