/* Strict alternation: the turn variable lets P0 and P1 into the critical section in
 * turn. Mutual exclusion holds but progress does not: if one is slow outside its critical
 * section, the other is forced to wait too. */
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

#define ROUNDS 4

atomic_int turn = 0;

static char log_buf[64];         /* order of entry into the critical section */
static int log_n = 0;

static void critical_section(int i)
{
    log_buf[log_n++] = (char)('0' + i);   /* only one writes at a time thanks to turn */
}

static void noncritical(int i)
{
    /* P1 is slow outside the critical section: P0 cannot enter a second time meanwhile */
    if (i == 1) {
        struct timespec ts = { 0, 2000000 };  /* 2 ms */
        nanosleep(&ts, NULL);
    }
}

static void *P0(void *arg)
{
    (void)arg;
    for (int k = 0; k < ROUNDS; k++) {       /* ROUNDS rounds instead of while (true) */
        while (atomic_load(&turn) != 0)
            ;                                 /* wait */
        critical_section(0);
        atomic_store(&turn, 1);
        noncritical(0);
    }
    return NULL;
}

static void *P1(void *arg)
{
    (void)arg;
    for (int k = 0; k < ROUNDS; k++) {
        while (atomic_load(&turn) != 1)
            ;                                 /* wait */
        critical_section(1);
        atomic_store(&turn, 0);
        noncritical(1);
    }
    return NULL;
}

int main(void)
{
    pthread_t t0, t1;
    pthread_create(&t0, NULL, P0, NULL);
    pthread_create(&t1, NULL, P1, NULL);
    pthread_join(t0, NULL);
    pthread_join(t1, NULL);

    log_buf[log_n] = '\0';
    printf("critical section order: %s\n", log_buf);
    printf("turn = %d\n", atomic_load(&turn));
    return 0;
}

/* Expected output (same on every run: the order is forced by turn):
 * critical section order: 01010101
 * turn = 0
 */
