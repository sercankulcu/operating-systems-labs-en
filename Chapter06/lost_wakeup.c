/* A fatal race: the lost wakeup. A producer-consumer written with sleep/wakeup: the
 * consumer sees count == 0 but is interrupted before it sleeps; the producer sends
 * wakeup(consumer), and since nobody is asleep the signal is lost. In the end both
 * sleep. A delay forces the moment of interruption; instead of sleeping forever,
 * sleep() returns after a 1-second timeout and the program reports the deadlock and exits. */
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <errno.h>

#define sleep proc_sleep           /* do not clash with <unistd.h> sleep() */

#define N 100
#define TRUE 1
#define PRODUCE_MAX 1000           /* at most this many items instead of while (TRUE) */

atomic_int count = 0;
static atomic_int buffer[N];
static int insert_i = 0, remove_i = 0;   /* used only by the producer / only by the consumer */

/* sleep record of each process */
struct sleeper {
    const char *name;
    pthread_mutex_t m;
    pthread_cond_t c;
    int sleeping, woken, deadlocked;
};
static struct sleeper producer_rec = { "producer", PTHREAD_MUTEX_INITIALIZER,
                                       PTHREAD_COND_INITIALIZER, 0, 0, 0 };
static struct sleeper consumer_rec = { "consumer", PTHREAD_MUTEX_INITIALIZER,
                                       PTHREAD_COND_INITIALIZER, 0, 0, 0 };
static _Thread_local struct sleeper *self;
static atomic_int lost = 0;
static atomic_int consumer_read = 0;

void *producer(void *arg);
void *consumer(void *arg);

/* sleep(): sleep until woken; if nobody wakes us within 1 s, count it as deadlock */
static int sleep(void)
{
    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += 1;
    pthread_mutex_lock(&self->m);
    self->sleeping = 1;
    while (!self->woken) {
        if (pthread_cond_timedwait(&self->c, &self->m, &deadline) == ETIMEDOUT) {
            self->deadlocked = 1;
            break;
        }
    }
    self->sleeping = 0;
    self->woken = 0;
    int d = self->deadlocked;
    pthread_mutex_unlock(&self->m);
    return d;
}

/* wakeup(who): if who is not asleep the wakeup goes nowhere */
static void wakeup(void *(*who)(void *))
{
    struct sleeper *u = (who == producer) ? &producer_rec : &consumer_rec;
    pthread_mutex_lock(&u->m);
    if (u->sleeping) {
        u->woken = 1;
        pthread_cond_signal(&u->c);
    } else {
        atomic_fetch_add(&lost, 1);        /* lost wakeup */
    }
    pthread_mutex_unlock(&u->m);
}

static int next_item = 0;
static int produce_item(void) { return ++next_item; }
static void insert_item(int item) { atomic_store(&buffer[insert_i], item); insert_i = (insert_i + 1) % N; }
static int remove_item(void) { int x = atomic_load(&buffer[remove_i]); remove_i = (remove_i + 1) % N; return x; }
static long consumed_total = 0;
static void consume_item(int item) { consumed_total += item; }

/* the consumer is interrupted right after reading count (the scheduler takes the CPU) */
static void interruption(void)
{
    atomic_store(&consumer_read, 1);
    struct timespec ts = { 0, 100000000 };   /* 100 ms */
    nanosleep(&ts, NULL);
}

void *producer(void *arg)
{
    int item;
    (void)arg;
    self = &producer_rec;
    for (int k = 0; k < PRODUCE_MAX; k++) {  /* while (TRUE) */
        item = produce_item();
        if (atomic_load(&count) == N)
            if (sleep()) return NULL;        /* deadlocked */
        insert_item(item);
        atomic_store(&count, atomic_load(&count) + 1);   /* count = count + 1 */
        if (atomic_load(&count) == 1) wakeup(consumer);
    }
    return NULL;
}

void *consumer(void *arg)
{
    int item;
    (void)arg;
    self = &consumer_rec;
    for (int k = 0; k < PRODUCE_MAX; k++) {  /* while (TRUE) */
        if (atomic_load(&count) == 0) {
            interruption();                  /* <- interrupted here */
            if (sleep()) return NULL;        /* deadlocked */
        }
        item = remove_item();
        atomic_store(&count, atomic_load(&count) - 1);   /* count = count - 1 */
        if (atomic_load(&count) == N - 1) wakeup(producer);
        consume_item(item);
    }
    return NULL;
}

int main(void)
{
    pthread_t p, c;
    pthread_create(&c, NULL, consumer, NULL);
    while (!atomic_load(&consumer_read)) {   /* the consumer saw count == 0 */
        struct timespec ts = { 0, 1000000 };
        nanosleep(&ts, NULL);
    }
    pthread_create(&p, NULL, producer, NULL);
    pthread_join(p, NULL);
    pthread_join(c, NULL);

    printf("count = %d\n", atomic_load(&count));
    printf("lost wakeups = %d\n", atomic_load(&lost));
    printf("producer: %s\n", producer_rec.deadlocked ? "asleep (nobody woke it)" : "finished");
    printf("consumer: %s\n", consumer_rec.deadlocked ? "asleep (nobody woke it)" : "finished");
    if (producer_rec.deadlocked && consumer_rec.deadlocked)
        printf("both would sleep forever: deadlock\n");
    return 0;
}

/* Expected output (timing dependent, but because of the 100 ms window it is in
 * practice the same on every run):
 * count = 100
 * lost wakeups = 1
 * producer: asleep (nobody woke it)
 * consumer: asleep (nobody woke it)
 * both would sleep forever: deadlock
 */
