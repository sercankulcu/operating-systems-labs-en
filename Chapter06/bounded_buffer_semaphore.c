/* The bounded buffer (producer-consumer) with three semaphores: mutex guards access to the
 * buffer, empty counts the empty slots, full counts the full slots. The producer produces
 * 1..1000, the consumer consumes all of them in order. */
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#define N 100
#define COUNT 1000                /* COUNT items instead of while (true) */

typedef sem_t semaphore;
static void down(semaphore *s) { sem_wait(s); }
static void up(semaphore *s)   { sem_post(s); }

semaphore mutex;   /* access to the buffer = 1 */
semaphore empty;   /* number of empty slots = N */
semaphore full;    /* number of full slots = 0 */

static int buffer[N];
static int in = 0, out = 0;
static int next_item = 0;
static long total = 0;
static int order_broken = 0, expected = 1;

static int produce_item(void) { return ++next_item; }
static void insert_item(int item) { buffer[in] = item; in = (in + 1) % N; }
static int remove_item(void) { int x = buffer[out]; out = (out + 1) % N; return x; }
static void consume_item(int item)
{
    if (item != expected++)
        order_broken = 1;
    total += item;
}

void *producer(void *arg)
{
    int item;
    (void)arg;
    for (int k = 0; k < COUNT; k++) {
        item = produce_item();
        down(&empty);   /* wait for an empty slot */
        down(&mutex);   /* enter the CS           */
        insert_item(item);
        up(&mutex);     /* leave the CS           */
        up(&full);      /* full count + 1         */
    }
    return NULL;
}

void *consumer(void *arg)
{
    int item;
    (void)arg;
    for (int k = 0; k < COUNT; k++) {
        down(&full);    /* wait for a full slot   */
        down(&mutex);
        item = remove_item();
        up(&mutex);
        up(&empty);     /* empty count + 1        */
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

    int e, f;
    sem_getvalue(&empty, &e);
    sem_getvalue(&full, &f);
    printf("consumed total = %ld (1 + ... + %d)\n", total, COUNT);
    printf("order %s\n", order_broken ? "BROKEN" : "preserved");
    printf("empty = %d, full = %d\n", e, f);
    sem_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);
    return 0;
}

/* Expected output (same on every run):
 * consumed total = 500500 (1 + ... + 1000)
 * order preserved
 * empty = 100, full = 0
 */
