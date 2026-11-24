/* What if the order of down changes: the producer takes mutex first, then empty. When the
 * buffer is full the producer blocks on empty while holding mutex; the consumer gets full
 * but blocks on mutex: deadlock. Instead of waiting forever, down() returns after a
 * 1-second timeout and the program reports the state and exits. */
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <time.h>
#include <errno.h>

#define N 2
#define COUNT 5

typedef sem_t semaphore;

static atomic_int deadlocked = 0;

/* down: returns 0 if it cannot be acquired within 1 s (a sign of deadlock) */
static int down(semaphore *s)
{
    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += 1;
    while (sem_timedwait(s, &deadline) != 0) {
        if (errno == ETIMEDOUT) {
            atomic_fetch_add(&deadlocked, 1);
            return 0;
        }
    }
    return 1;
}
static void up(semaphore *s) { sem_post(s); }

semaphore mutex, empty, full;     /* 1, N, 0 */

static int buffer[N], in = 0, out = 0, next_item = 0;
static const char *producer_where = "finished", *consumer_where = "finished";
static atomic_int consumer_start = 0;

static int produce_item(void) { return ++next_item; }
static void insert_item(int item) { buffer[in] = item; in = (in + 1) % N; }
static int remove_item(void) { int x = buffer[out]; out = (out + 1) % N; return x; }
static void consume_item(int item) { (void)item; }

void *producer(void *arg)
{
    int item;
    (void)arg;
    for (int k = 0; k < COUNT; k++) {
        item = produce_item();
        if (!down(&mutex)) { producer_where = "mutex"; return NULL; }  /* <- first */
        if (k == N)                    /* buffer full: let the consumer start now */
            atomic_store(&consumer_start, 1);
        if (!down(&empty)) {                                            /* <- then */
            producer_where = "empty (holding mutex)";
            return NULL;    /* in reality it waits forever; mutex is never released */
        }
        insert_item(item);
        up(&mutex);
        up(&full);
    }
    return NULL;
}

void *consumer(void *arg)
{
    int item;
    (void)arg;
    while (!atomic_load(&consumer_start)) {   /* delay until the buffer is full */
        struct timespec ts = { 0, 1000000 };
        nanosleep(&ts, NULL);
    }
    for (int k = 0; k < COUNT; k++) {
        if (!down(&full))  { consumer_where = "full";  return NULL; }
        if (!down(&mutex)) { consumer_where = "mutex (full acquired)"; return NULL; }
        item = remove_item();
        up(&mutex);
        up(&empty);
        consume_item(item);
    }
    return NULL;
}

int main(void)
{
    pthread_t p, c;
    sem_init(&mutex, 0, 1);
    sem_init(&empty, 0, N);
    sem_init(&full, 0, 0);
    pthread_create(&p, NULL, producer, NULL);
    pthread_create(&c, NULL, consumer, NULL);
    pthread_join(p, NULL);
    pthread_join(c, NULL);

    printf("producer  waited at: %s\n", producer_where);
    printf("consumer  waited at: %s\n", consumer_where);
    printf("down calls that timed out: %d -> %s\n", atomic_load(&deadlocked),
           atomic_load(&deadlocked) == 2 ? "deadlock" : "no deadlock");
    sem_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);
    return 0;
}

/* Expected output (same on every run; takes about 1 second):
 * producer  waited at: empty (holding mutex)
 * consumer  waited at: mutex (full acquired)
 * down calls that timed out: 2 -> deadlock
 */
