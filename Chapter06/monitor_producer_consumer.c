/* Producer-consumer with a monitor (N = 2). C has no monitors: the monitor's implicit lock is
 * a mutex, and the full and empty conditions are pthread condition variables. Each
 * procedure takes the lock on entry and releases it on exit. Under Hoare semantics "if"
 * is enough; pthreads has Mesa semantics, so "while" is used here.
 * The consumer arrives first (waits on empty), the producer inserts a, b, c, d. */
#include <stdio.h>
#include <pthread.h>
#include <time.h>

/* do not clash with <stdio.h> remove() */
#define remove pc_remove

#define N 2

/* monitor ProducerConsumer */
static pthread_mutex_t monitor_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t full = PTHREAD_COND_INITIALIZER;
static pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
static int count = 0;
static int buffer[N], head = 0, tail = 0;
static int max_count = 0;         /* largest count seen */
static int empty_waiting = 0;

static void insert_item(int item) { buffer[tail] = item; tail = (tail + 1) % N; }
static int remove_item(void) { int x = buffer[head]; head = (head + 1) % N; return x; }

void insert(int item)
{
    pthread_mutex_lock(&monitor_lock);
    while (count == N) pthread_cond_wait(&full, &monitor_lock);   /* if count = N then wait(full) */
    insert_item(item);
    count = count + 1;
    if (count > max_count) max_count = count;
    if (count == 1) pthread_cond_signal(&empty);
    pthread_mutex_unlock(&monitor_lock);
}

int remove(void)
{
    pthread_mutex_lock(&monitor_lock);
    while (count == 0) {                                          /* if count = 0 then wait(empty) */
        empty_waiting++;
        pthread_cond_wait(&empty, &monitor_lock);
        empty_waiting--;
    }
    int r = remove_item();
    count = count - 1;
    if (count == N - 1) pthread_cond_signal(&full);
    pthread_mutex_unlock(&monitor_lock);
    return r;
}
/* end monitor */

#define ITEMS 4
static int received[ITEMS];

static void *consumer(void *arg)
{
    (void)arg;
    for (int k = 0; k < ITEMS; k++)
        received[k] = remove();
    return NULL;
}

static void *producer(void *arg)
{
    (void)arg;
    for (int k = 0; k < ITEMS; k++)
        insert('a' + k);
    return NULL;
}

int main(void)
{
    pthread_t t, p;
    pthread_create(&t, NULL, consumer, NULL);
    for (;;) {                               /* until T waits on empty */
        pthread_mutex_lock(&monitor_lock);
        int w = empty_waiting;
        pthread_mutex_unlock(&monitor_lock);
        if (w == 1)
            break;
        struct timespec ts = { 0, 1000000 };
        nanosleep(&ts, NULL);
    }
    printf("T remove(): buffer empty, waiting on empty\n");
    pthread_create(&p, NULL, producer, NULL);
    pthread_join(p, NULL);
    pthread_join(t, NULL);

    printf("T received:");
    for (int k = 0; k < ITEMS; k++)
        printf(" %c", received[k]);
    printf("\ncount = %d, max count = %d (N = %d is never exceeded)\n",
           count, max_count <= N ? max_count : -1, N);
    return 0;
}

/* Expected output (the first two lines are the same on every run; "max count"
 * may be 1 or 2 depending on timing):
 * T remove(): buffer empty, waiting on empty
 * T received: a b c d
 * count = 0, max count = 2 (N = 2 is never exceeded)
 */
