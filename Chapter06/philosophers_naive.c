/* Dining philosophers, naive solution: each philosopher takes the left fork first, then the right.
 * If all five take their left fork at the same time, all wait for the right one: deadlock.
 * A delay forces this bad order (after taking the left fork everyone thinks for 100 ms);
 * if the right fork cannot be taken within 1 s the philosopher puts the left fork down and gives up. */
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <time.h>
#include <errno.h>

#define wait   semaphore_wait      /* do not clash with <sys/wait.h> */
#define signal semaphore_signal    /* do not clash with <signal.h> */

#define ROUNDS 3

typedef sem_t semaphore;

static int wait(semaphore *s)    /* 0 if not acquired within 1 s */
{
    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += 1;
    while (sem_timedwait(s, &deadline) != 0)
        if (errno == ETIMEDOUT)
            return 0;
    return 1;
}
static void signal(semaphore *s) { sem_post(s); }

semaphore fork_sem[5];     /* all 1 */

static atomic_int meals = 0;
static int stuck[5];       /* did it wait for the right fork and give up */
static atomic_int stuck_count = 0;

static void think(void) { }
static void eat(void) { atomic_fetch_add(&meals, 1); }

static void bad_timing(void)   /* preemption after the left fork is taken */
{
    struct timespec ts = { 0, 100000000 };   /* 100 ms */
    nanosleep(&ts, NULL);
}

void philosopher(int i)
{
    for (int k = 0; k < ROUNDS; k++) {       /* ROUNDS rounds instead of while (true) */
        think();
        wait(&fork_sem[i]);                  /* left */
        bad_timing();
        if (!wait(&fork_sem[(i + 1) % 5])) { /* right */
            stuck[i] = 1;
            atomic_fetch_add(&stuck_count, 1);
            /* in reality it waits forever. If we put the left fork down at once, a
             * neighbour that has not timed out yet could take it and eat; so everyone
             * keeps the fork until all are stuck (at most 2 s). */
            for (int d = 0; d < 2000 && atomic_load(&stuck_count) < 5; d++) {
                struct timespec ts = { 0, 1000000 };
                nanosleep(&ts, NULL);
            }
            signal(&fork_sem[i]);
            return;
        }
        eat();
        signal(&fork_sem[(i + 1) % 5]);
        signal(&fork_sem[i]);
    }
}

static void *philosopher_thread(void *arg)
{
    philosopher(*(int *)arg);
    return NULL;
}

int main(void)
{
    pthread_t t[5];
    int id[5];
    for (int i = 0; i < 5; i++)
        sem_init(&fork_sem[i], 0, 1);
    for (int i = 0; i < 5; i++) {
        id[i] = i;
        pthread_create(&t[i], NULL, philosopher_thread, &id[i]);
    }
    for (int i = 0; i < 5; i++)
        pthread_join(t[i], NULL);

    int n = 0;
    for (int i = 0; i < 5; i++)
        n += stuck[i];
    printf("meals eaten = %d\n", atomic_load(&meals));
    printf("philosophers holding the left fork and waiting for the right = %d\n", n);
    if (n == 5)
        printf("deadlock: everyone holds the fork on their left, nobody can eat\n");
    for (int i = 0; i < 5; i++)
        sem_destroy(&fork_sem[i]);
    return 0;
}

/* Expected output (the delay forces the bad order, so in practice it is the same
 * on every run; takes about 1.1 s):
 * meals eaten = 0
 * philosophers holding the left fork and waiting for the right = 5
 * deadlock: everyone holds the fork on their left, nobody can eat
 */
