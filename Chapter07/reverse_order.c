/* Acquisition in reverse order: A takes r1 then r2, B takes r2 then r1 -> deadlock.
 * down() uses sem_timedwait with a 1 s timeout; this way the deadlocked threads
 * give up and exit and the program terminates. In the second round B starts after
 * A has finished: the same code does not deadlock this time. */
#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define STATE_LEN 96

typedef sem_t semaphore;

/* On the slide: semaphore r1 = 1, r2 = 1;  (sem_t cannot be assigned = 1; sem_init in main) */
static semaphore r1, r2;

static int window;                        /* 1: sleep 100 ms after down */
static char state_A[STATE_LEN], state_B[STATE_LEN];
static _Thread_local char *my_state;      /* this thread's status text */
static _Thread_local int held;            /* bit 1: r1, bit 2: r2 */

static const char *name(semaphore *s) { return s == &r1 ? "r1" : "r2"; }
static int bit(semaphore *s) { return s == &r1 ? 1 : 2; }

static void down(semaphore *s)
{
    struct timespec deadline;

    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += 1;
    while (sem_timedwait(s, &deadline) != 0) {
        if (errno == ETIMEDOUT) {
            snprintf(my_state, STATE_LEN, "blocked (waiting for %s, holding %s)",
                     name(s), held == 1 ? "r1" : "r2");
            pthread_exit(NULL);             /* could not get it within 1 s: give up */
        }
    }
    held |= bit(s);
    snprintf(my_state, STATE_LEN, "holding %s", name(s));
    if (window)
        usleep(100000);                     /* widen the window */
}

static void up(semaphore *s)
{
    held &= ~bit(s);
    sem_post(s);
}

static void use_both(void)
{
    snprintf(my_state, STATE_LEN, "ran with both resources");
}

static void A(void)
{
    down(&r1);
    down(&r2);
    use_both();
    up(&r2); up(&r1);
}

static void B(void)
{
    down(&r2);
    down(&r1);
    use_both();
    up(&r1); up(&r2);
}

static void *run_A(void *arg)
{
    (void)arg;
    my_state = state_A;
    A();
    snprintf(my_state, STATE_LEN, "done");
    return NULL;
}

static void *run_B(void *arg)
{
    (void)arg;
    my_state = state_B;
    B();
    snprintf(my_state, STATE_LEN, "done");
    return NULL;
}

static void round_(int concurrent)
{
    pthread_t a, b;

    sem_init(&r1, 0, 1);
    sem_init(&r2, 0, 1);
    snprintf(state_A, STATE_LEN, "ready");
    snprintf(state_B, STATE_LEN, "ready");

    pthread_create(&a, NULL, run_A, NULL);
    if (!concurrent)
        pthread_join(a, NULL);              /* A finishes both downs before B starts */
    pthread_create(&b, NULL, run_B, NULL);
    if (concurrent)
        pthread_join(a, NULL);
    pthread_join(b, NULL);

    printf("A: %s\n", state_A);
    printf("B: %s\n", state_B);

    sem_destroy(&r1);
    sem_destroy(&r2);
}

int main(void)
{
    printf("1) A and B at the same time, 100 ms between the downs:\n");
    window = 1;
    round_(1);
    printf("   -> cycle: A -> r2 -> B -> r1 -> A: deadlock\n");

    printf("2) B starts when A is done:\n");
    window = 0;
    round_(0);
    printf("   -> no deadlock\n");
    return 0;
}

/* Expected output (takes about 1.2 s):
 * 1) A and B at the same time, 100 ms between the downs:
 * A: blocked (waiting for r2, holding r1)
 * B: blocked (waiting for r1, holding r2)
 *    -> cycle: A -> r2 -> B -> r1 -> A: deadlock
 * 2) B starts when A is done:
 * A: done
 * B: done
 *    -> no deadlock
 * The deadlock does not occur on every run; the 100 ms window in round 1 makes it
 * almost certain.
 */
