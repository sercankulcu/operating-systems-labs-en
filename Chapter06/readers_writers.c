/* The readers-writers problem (readers' preference): the first reader locks the database (db),
 * the last reader releases it; a writer takes db alone. 4 readers and 2 writers run for a
 * bounded number of rounds; rule violations (a reader with a writer, or two writers at the
 * same time) are counted. */
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>

#define READERS 4
#define WRITERS 2
#define ROUNDS 2000              /* instead of while (true) */

typedef sem_t semaphore;
static void down(semaphore *s) { sem_wait(s); }
static void up(semaphore *s)   { sem_post(s); }

semaphore mutex;      /* guards rc        = 1 */
semaphore db;         /* guards database  = 1 */
int rc = 0;           /* reader count     */

static long data = 0;                     /* the "database" */
static atomic_int reading = 0, writing = 0, violations = 0;
static atomic_int max_readers = 0;
static atomic_long reads = 0, writes = 0;

static void think_up_data(void) { }
static void use_data_read(void) { }

static void write_data_base(void)
{
    if (atomic_fetch_add(&writing, 1) != 0 || atomic_load(&reading) != 0)
        atomic_fetch_add(&violations, 1);
    data++;
    atomic_fetch_add(&writes, 1);
    atomic_fetch_sub(&writing, 1);
}

static void read_data_base(void)
{
    int r = atomic_fetch_add(&reading, 1) + 1;
    int m = atomic_load(&max_readers);
    while (r > m && !atomic_compare_exchange_weak(&max_readers, &m, r))
        ;
    if (atomic_load(&writing) != 0)
        atomic_fetch_add(&violations, 1);
    long v = data;                        /* read */
    (void)v;
    atomic_fetch_add(&reads, 1);
    atomic_fetch_sub(&reading, 1);
}

void *writer(void *arg)
{
    (void)arg;
    for (int k = 0; k < ROUNDS; k++) {
        think_up_data();
        down(&db);  write_data_base();  up(&db);
    }
    return NULL;
}

void *reader(void *arg)
{
    (void)arg;
    for (int k = 0; k < ROUNDS; k++) {
        down(&mutex);
        if (++rc == 1) down(&db);  /* first reader */
        up(&mutex);
        read_data_base();
        down(&mutex);
        if (--rc == 0) up(&db);    /* last reader  */
        up(&mutex);
        use_data_read();
    }
    return NULL;
}

int main(void)
{
    pthread_t r[READERS], w[WRITERS];
    sem_init(&mutex, 0, 1);
    sem_init(&db, 0, 1);
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
    printf("most simultaneous readers = %d (varies, 1..%d)\n",
           atomic_load(&max_readers), READERS);
    sem_destroy(&mutex);
    sem_destroy(&db);
    return 0;
}

/* Expected output (the last line depends on timing):
 * reads = 8000, writes = 4000, data = 4000
 * rule violations = 0, rc = 0
 * most simultaneous readers = N (varies, 1..4)
 */
