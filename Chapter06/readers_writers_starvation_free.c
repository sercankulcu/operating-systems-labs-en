/* Readers-writers, Downey's starvation-free solution: everyone first passes through the same
 * turnstile. While a writer holds the turnstile no new reader can enter, so a steady stream
 * of readers cannot starve the writer. The first reader takes roomEmpty, the last reader
 * releases it (via rc). */
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>

/* wait/signal are renamed so they do not clash with <sys/wait.h> and <signal.h> */
#define wait   semaphore_wait
#define signal semaphore_signal

#define READERS 4
#define WRITERS 2
#define ROUNDS 2000

typedef sem_t semaphore;
static void wait(semaphore *s)   { sem_wait(s); }
static void signal(semaphore *s) { sem_post(s); }

semaphore turnstile;  /* = 1 */
semaphore roomEmpty;  /* = 1: nobody is in the room */
semaphore mutex;      /* guards rc = 1 */
int rc = 0;

static long data = 0;
static atomic_int reading = 0, writing = 0, violations = 0;
static atomic_long reads = 0, writes = 0;

static void write_data(void)
{
    if (atomic_fetch_add(&writing, 1) != 0 || atomic_load(&reading) != 0)
        atomic_fetch_add(&violations, 1);
    data++;
    atomic_fetch_add(&writes, 1);
    atomic_fetch_sub(&writing, 1);
}

static void read_data(void)
{
    atomic_fetch_add(&reading, 1);
    if (atomic_load(&writing) != 0)
        atomic_fetch_add(&violations, 1);
    long v = data;
    (void)v;
    atomic_fetch_add(&reads, 1);
    atomic_fetch_sub(&reading, 1);
}

static void *writer(void *arg)
{
    (void)arg;
    for (int k = 0; k < ROUNDS; k++) {
        wait(&turnstile);
        wait(&roomEmpty);
          write_data();
        signal(&turnstile);
        signal(&roomEmpty);
    }
    return NULL;
}

static void *reader(void *arg)
{
    (void)arg;
    for (int k = 0; k < ROUNDS; k++) {
        wait(&turnstile); signal(&turnstile);
        wait(&mutex);
        if (++rc == 1) wait(&roomEmpty);     /* the first reader takes roomEmpty */
        signal(&mutex);
          read_data();
        wait(&mutex);
        if (--rc == 0) signal(&roomEmpty);   /* the last reader releases it */
        signal(&mutex);
    }
    return NULL;
}

int main(void)
{
    pthread_t r[READERS], w[WRITERS];
    sem_init(&turnstile, 0, 1);
    sem_init(&roomEmpty, 0, 1);
    sem_init(&mutex, 0, 1);
    for (int i = 0; i < READERS; i++)
        pthread_create(&r[i], NULL, reader, NULL);
    for (int i = 0; i < WRITERS; i++)
        pthread_create(&w[i], NULL, writer, NULL);
    for (int i = 0; i < READERS; i++)
        pthread_join(r[i], NULL);
    for (int i = 0; i < WRITERS; i++)
        pthread_join(w[i], NULL);

    printf("reads = %ld, writes = %ld, data = %ld\n",
           atomic_load(&reads), atomic_load(&writes), data);
    printf("rule violations = %d, rc = %d\n", atomic_load(&violations), rc);
    sem_destroy(&turnstile);
    sem_destroy(&roomEmpty);
    sem_destroy(&mutex);
    return 0;
}

/* Expected output (same on every run):
 * reads = 8000, writes = 4000, data = 4000
 * rule violations = 0, rc = 0
 */
