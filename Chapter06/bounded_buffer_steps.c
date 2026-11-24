/* The bounded buffer step by step: a two-slot buffer (empty = 2), semaphores with the names
 * wait/signal. First the producer runs alone: it puts two items and blocks on empty at the
 * third (mutex = 1, empty = 0, full = 2). Then the consumer starts and all 5 items
 * pass through in order. */
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

/* wait and signal are renamed after the headers so they do not clash with
 * the <sys/wait.h> / <signal.h> names. */
#define wait   semaphore_wait
#define signal semaphore_signal

#define COUNT 5

typedef sem_t semaphore;
static void wait(semaphore *s)   { sem_wait(s); }
static void signal(semaphore *s) { sem_post(s); }

semaphore mutex, empty, full;    /* mutex = 1, empty = 2, full = 0 */

static int buffer[2];
static int in = 0, out = 0, next_item = 0;
static int consumed[COUNT], cn = 0;

static int produce_item(void) { return ++next_item; }
static void insert_item(int item) { buffer[in] = item; in = (in + 1) % 2; }
static int remove_item(void) { int x = buffer[out]; out = (out + 1) % 2; return x; }
static void consume_item(int item) { consumed[cn++] = item; }

void *producer(void *arg)
{
    int item;
    (void)arg;
    for (int k = 0; k < COUNT; k++) {    /* COUNT rounds instead of while (true) */
        item = produce_item();
        wait(&empty);
        wait(&mutex);
        insert_item(item);
        signal(&mutex);
        signal(&full);
    }
    return NULL;
}

void *consumer(void *arg)
{
    int item;
    (void)arg;
    for (int k = 0; k < COUNT; k++) {
        wait(&full);
        wait(&mutex);
        item = remove_item();
        signal(&mutex);
        signal(&empty);
        consume_item(item);
    }
    return NULL;
}

static void print_state(const char *title)
{
    int m, e, f;
    sem_getvalue(&mutex, &m);
    sem_getvalue(&empty, &e);
    sem_getvalue(&full, &f);
    printf("%-26s mutex = %d, empty = %d, full = %d\n", title, m, e, f);
}

int main(void)
{
    pthread_t p, c;
    sem_init(&mutex, 0, 1);
    sem_init(&empty, 0, 2);
    sem_init(&full, 0, 0);
    print_state("initial:");

    pthread_create(&p, NULL, producer, NULL);
    for (;;) {                           /* until the producer has put two items and blocks */
        int f;
        sem_getvalue(&full, &f);
        if (f == 2)
            break;
        struct timespec ts = { 0, 1000000 };
        nanosleep(&ts, NULL);
    }
    struct timespec ts = { 0, 20000000 }; /* 20 ms: the producer is waiting on empty */
    nanosleep(&ts, NULL);
    print_state("producer blocked on empty:");

    pthread_create(&c, NULL, consumer, NULL);
    pthread_join(p, NULL);
    pthread_join(c, NULL);
    print_state("final:");
    printf("consumed:");
    for (int i = 0; i < cn; i++)
        printf(" %d", consumed[i]);
    printf("\n");
    sem_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);
    return 0;
}

/* Expected output (same on every run):
 * initial:                   mutex = 1, empty = 2, full = 0
 * producer blocked on empty: mutex = 1, empty = 0, full = 2
 * final:                     mutex = 1, empty = 2, full = 0
 * consumed: 1 2 3 4 5
 */
