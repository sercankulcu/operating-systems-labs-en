/* One condition variable is not enough: a one-slot buffer, two consumers (T1, T2), one
 * producer (U). With a single condition c a consumer's signal may by mistake wake the other
 * consumer; in the end all three threads sleep on c. With two conditions (full, empty)
 * each signal wakes the right group. To make the result deterministic the condition
 * variable is written by hand with a FIFO queue (pthread_cond does not guarantee which
 * waiter it wakes). If a wait is not woken within 1 s it counts as a deadlock. */
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

/* do not clash with <sys/wait.h> and <signal.h> */
#define wait   cond_wait
#define signal cond_signal

struct waiter {
    sem_t s;
    struct waiter *next;
};
typedef struct {
    struct waiter *head;         /* FIFO wait queue */
} condition_t;

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static int deadlocked = 0;

/* called with m held: join the queue, release m, sleep, reacquire m */
static void wait(condition_t *c, pthread_mutex_t *mx)
{
    struct waiter b;
    struct waiter **p = &c->head;
    sem_init(&b.s, 0, 0);
    b.next = NULL;
    while (*p != NULL)
        p = &(*p)->next;
    *p = &b;
    pthread_mutex_unlock(mx);

    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += 1;
    int was_woken = 1;
    while (sem_timedwait(&b.s, &deadline) != 0) {
        if (errno == ETIMEDOUT) {
            was_woken = 0;
            break;
        }
    }
    if (was_woken) {
        /* Mesa: the woken thread only joins the ready queue; until it runs, the
         * signaller continues. To make this order the same on every run
         * the woken thread is 10 ms late. */
        struct timespec delay = { 0, 10000000 };
        nanosleep(&delay, NULL);
    }
    pthread_mutex_lock(mx);
    if (!was_woken) {
        int queued = 0;
        for (struct waiter **q = &c->head; *q != NULL; q = &(*q)->next) {
            if (*q == &b) {
                *q = b.next;
                queued = 1;
                break;
            }
        }
        if (queued)
            deadlocked = 1;          /* nobody woke it */
        else
            sem_wait(&b.s);          /* a signal arrived just at the timeout */
    }
    sem_destroy(&b.s);
}

/* called with m held: wake the first waiter in the queue */
static void signal(condition_t *c)
{
    struct waiter *b = c->head;
    if (b != NULL) {
        c->head = b->next;
        sem_post(&b->s);
    }
}

static int queue_length(condition_t *c)
{
    int n = 0;
    for (struct waiter *b = c->head; b != NULL; b = b->next)
        n++;
    return n;
}

/* ---- one-slot buffer ---- */
static condition_t c, full, empty;
static int two_conditions;
static int count = 0, slot = 0, next_item = 0, consumed = 0;

static void put(int x) { slot = x; count = 1; }
static int get(void) { count = 0; return slot; }

static void *consumer(void *arg)
{
    (void)arg;
    for (int k = 0; k < 2; k++) {
        int x;
        pthread_mutex_lock(&m);
        if (!two_conditions) {
            /* WRONG: one condition c */
            while (count == 0 && !deadlocked) wait(&c, &m);
            if (deadlocked) { pthread_mutex_unlock(&m); return NULL; }
            x = get();  signal(&c);
        } else {
            /* RIGHT: two conditions */
            while (count == 0 && !deadlocked) wait(&full, &m);
            if (deadlocked) { pthread_mutex_unlock(&m); return NULL; }
            x = get();  signal(&empty);
        }
        (void)x;
        consumed++;
        pthread_mutex_unlock(&m);
    }
    return NULL;
}

static void *producer(void *arg)
{
    (void)arg;
    for (int k = 0; k < 4; k++) {
        int x = ++next_item;
        pthread_mutex_lock(&m);
        if (!two_conditions) {
            while (count == 1 && !deadlocked) wait(&c, &m);
            if (deadlocked) { pthread_mutex_unlock(&m); return NULL; }
            put(x);    signal(&c);
        } else {
            while (count == 1 && !deadlocked) wait(&empty, &m);
            if (deadlocked) { pthread_mutex_unlock(&m); return NULL; }
            put(x);    signal(&full);
        }
        pthread_mutex_unlock(&m);
    }
    return NULL;
}

static void wait_for_queue(condition_t *k, int n)
{
    for (;;) {
        pthread_mutex_lock(&m);
        int len = queue_length(k);
        pthread_mutex_unlock(&m);
        if (len == n)
            return;
        struct timespec ts = { 0, 1000000 };
        nanosleep(&ts, NULL);
    }
}

static void scenario(int two)
{
    pthread_t t1, t2, u;
    condition_t *tk = two ? &full : &c;   /* the condition the consumers wait on */
    two_conditions = two;
    deadlocked = 0;
    count = 0; slot = 0; next_item = 0; consumed = 0;
    pthread_create(&t1, NULL, consumer, NULL);
    wait_for_queue(tk, 1);                /* T1 is waiting */
    pthread_create(&t2, NULL, consumer, NULL);
    wait_for_queue(tk, 2);                /* T1, T2 are waiting */
    pthread_create(&u, NULL, producer, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(u, NULL);
    printf("%s: produced = %d, consumed = %d -> %s\n",
           two ? "two conditions (full, empty)" : "one condition c             ",
           next_item, consumed,
           deadlocked ? "DEADLOCK: all asleep on c" : "ok");
}

int main(void)
{
    scenario(0);
    scenario(1);
    return 0;
}

/* Expected output (same on every run; the first scenario takes ~1 s because of the timeout):
 * one condition c             : produced = 2, consumed = 1 -> DEADLOCK: all asleep on c
 * two conditions (full, empty): produced = 4, consumed = 4 -> ok
 *
 * The order with one condition: c = [T1 T2]; U puts, signal -> T1. U comes again, the slot
 * is full: c = [T2 U]. T1 takes, signal -> T2 (wrong group!). T2 sees empty and waits;
 * T1 waits too: c = [U T1 T2] or [U T2 T1]. Now nobody can signal.
 */
