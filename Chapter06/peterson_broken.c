/* Why Peterson breaks on a modern CPU: each core's write buffer (store buffer) delays the
 * "flag[i] = true" write, and the following read of "flag[j]" happens first. Both processes
 * see flag[j] == false and enter the critical section together. The same code runs first
 * with relaxed (unordered) and then with seq_cst (memory barrier) accesses. */
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sched.h>

#define ROUNDS 200000

static atomic_bool flag[2];
static atomic_int  turn;
static bool with_barrier;                  /* false: relaxed, true: seq_cst */

static atomic_int inside;
static atomic_int violations;

/* The memory order must be a compile-time constant (if it is a variable GCC treats it as seq_cst);
 * so the two versions are written separately. */
static void enter_relaxed(int i)
{
    int j = 1 - i;
    unsigned spins = 0;
    atomic_store_explicit(&flag[i], true, memory_order_relaxed);
    atomic_store_explicit(&turn, j, memory_order_relaxed);
    while (atomic_load_explicit(&flag[j], memory_order_relaxed)
           && atomic_load_explicit(&turn, memory_order_relaxed) == j) {
        if (++spins % 1024 == 0)
            sched_yield();
    }
}

static void enter_seq_cst(int i)
{
    int j = 1 - i;
    unsigned spins = 0;
    atomic_store_explicit(&flag[i], true, memory_order_seq_cst);
    atomic_store_explicit(&turn, j, memory_order_seq_cst);
    while (atomic_load_explicit(&flag[j], memory_order_seq_cst)
           && atomic_load_explicit(&turn, memory_order_seq_cst) == j) {
        if (++spins % 1024 == 0)
            sched_yield();
    }
}

static void enter(int i)
{
    if (with_barrier)
        enter_seq_cst(i);
    else
        enter_relaxed(i);
}

static void leave(int i)
{
    if (with_barrier)
        atomic_store_explicit(&flag[i], false, memory_order_seq_cst);
    else
        atomic_store_explicit(&flag[i], false, memory_order_relaxed);
}

static void *P(void *arg)
{
    int i = *(int *)arg;
    for (int k = 0; k < ROUNDS; k++) {
        enter(i);
        /* critical section: is anyone else inside? */
        if (atomic_fetch_add_explicit(&inside, 1, memory_order_relaxed) != 0)
            atomic_fetch_add_explicit(&violations, 1, memory_order_relaxed);
        atomic_fetch_sub_explicit(&inside, 1, memory_order_relaxed);
        leave(i);
    }
    return NULL;
}

static int run(bool b)
{
    pthread_t t[2];
    int id[2] = { 0, 1 };
    with_barrier = b;
    atomic_store(&flag[0], false);
    atomic_store(&flag[1], false);
    atomic_store(&turn, 0);
    atomic_store(&inside, 0);
    atomic_store(&violations, 0);
    for (int i = 0; i < 2; i++)
        pthread_create(&t[i], NULL, P, &id[i]);
    for (int i = 0; i < 2; i++)
        pthread_join(t[i], NULL);
    return atomic_load(&violations);
}

int main(void)
{
    int r = run(false);
    printf("relaxed (no barrier)  : %d violations\n", r);
    int s = run(true);
    printf("seq_cst (with barrier): %d violations\n", s);
    return 0;
}

/* Expected output (x86-64, multi-core; the first number varies on every run and
 * may be 0 by luck, the second line is always 0):
 * relaxed (no barrier)  : N violations
 * seq_cst (with barrier): 0 violations
 */
