/* Why while and not if? Under Mesa semantics signal is only a "hint": until the woken thread
 * reacquires the lock, someone else (B) may grab the item. Consumer A waits on full; the
 * main thread calls put(), signals, and right after that, acting as B, calls take().
 * With the if version A reads from an empty buffer; with the while version it waits again. */
#include <stdio.h>
#include <pthread.h>
#include <time.h>

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t full = PTHREAD_COND_INITIALIZER;
static int count = 0;            /* items in the buffer */
static int next_item = 0;
static int waiting = 0, wakeups = 0;

static void lock(pthread_mutex_t *x)   { pthread_mutex_lock(x); }
static void unlock(pthread_mutex_t *x) { pthread_mutex_unlock(x); }
static void cond_wait(pthread_cond_t *c, pthread_mutex_t *x)
{
    waiting++;
    pthread_cond_wait(c, x);
    waiting--;
    wakeups++;
}

static void put(void) { count++; next_item++; }
static int take(void)                   /* taking from an empty buffer is a BUG */
{
    if (count == 0)
        return -1;
    count--;
    return next_item;
}

static int A_result;

/* WRONG (under Mesa) */
static void *consumer_if(void *arg)
{
    int x;
    (void)arg;
    lock(&m);
    if (count == 0)
        cond_wait(&full, &m);
    x = take();      /* may be empty */
    unlock(&m);
    A_result = x;
    return NULL;
}

/* RIGHT */
static void *consumer_while(void *arg)
{
    int x;
    (void)arg;
    lock(&m);
    while (count == 0)
        cond_wait(&full, &m);
    x = take();      /* count > 0 for sure */
    unlock(&m);
    A_result = x;
    return NULL;
}

/* wait until A is waiting on full with wakeup count u */
static void wait_for_A(int u)
{
    for (;;) {
        lock(&m);
        int ready = (waiting == 1 && wakeups == u);
        unlock(&m);
        if (ready)
            return;
        struct timespec ts = { 0, 1000000 };
        nanosleep(&ts, NULL);
    }
}

static void scenario(const char *name, void *(*consumer)(void *), int second_put)
{
    pthread_t A;
    count = 0; next_item = 0; waiting = 0; wakeups = 0;
    pthread_create(&A, NULL, consumer, NULL);
    wait_for_A(0);                    /* A is asleep on full */

    lock(&m);
    put();                            /* producer: item 1 */
    pthread_cond_signal(&full);       /* A is woken but cannot take the lock yet */
    int B = take();                   /* B slips in and grabs the item */
    unlock(&m);
    printf("%s: B take() = %d\n", name, B);

    if (second_put) {                 /* while: A is waiting again */
        wait_for_A(1);
        printf("%s: A woke up, saw count = 0, waits again\n", name);
        lock(&m);
        put();                        /* item 2 */
        pthread_cond_signal(&full);
        unlock(&m);
    }
    pthread_join(A, NULL);
    if (A_result < 0)
        printf("%s: A take() = %d -> read from an empty buffer (BUG)\n", name, A_result);
    else
        printf("%s: A take() = %d -> correct\n", name, A_result);
}

int main(void)
{
    scenario("if   ", consumer_if, 0);
    scenario("while", consumer_while, 1);
    return 0;
}

/* Expected output (same on every run; a very rare spurious wakeup could wake
 * A early in the "if" version):
 * if   : B take() = 1
 * if   : A take() = -1 -> read from an empty buffer (BUG)
 * while: B take() = 1
 * while: A woke up, saw count = 0, waits again
 * while: A take() = 2 -> correct
 */
