/* A barrier with semaphores (turnstile): N = 3 threads arrive at barrier(); the last one to
 * arrive opens the turnstile, and each thread that passes lets the one behind it through.
 * down/up are written with POSIX sem_wait/sem_post. No thread can pass the barrier
 * before all three have arrived. */
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>

#define N 3

typedef sem_t semaphore;

static void down(semaphore *s) { sem_wait(s); }
static void up(semaphore *s)   { sem_post(s); }

semaphore mutex, turnstile;         /* mutex = 1, turnstile = 0 */
int count = 0;

static atomic_int arrived = 0;      /* threads that reached the barrier */
static atomic_int passed_early = 0; /* passed before everyone arrived (must not happen) */

void barrier(void)
{
    down(&mutex);
    count = count + 1;
    if (count == N) up(&turnstile); /* last */
    up(&mutex);
    down(&turnstile);               /* pass through */
    up(&turnstile);                 /* let the next one in */
}

static void *worker(void *arg)
{
    (void)arg;
    atomic_fetch_add(&arrived, 1);
    barrier();
    if (atomic_load(&arrived) != N)
        atomic_fetch_add(&passed_early, 1);
    return NULL;
}

int main(void)
{
    pthread_t t[N];
    sem_init(&mutex, 0, 1);
    sem_init(&turnstile, 0, 0);
    for (int i = 0; i < N; i++)
        pthread_create(&t[i], NULL, worker, NULL);
    for (int i = 0; i < N; i++)
        pthread_join(t[i], NULL);

    int m, tk;
    sem_getvalue(&mutex, &m);
    sem_getvalue(&turnstile, &tk);
    printf("mutex = %d, turnstile = %d, count = %d\n", m, tk, count);
    printf("passed before everyone arrived: %d\n", atomic_load(&passed_early));
    sem_destroy(&mutex);
    sem_destroy(&turnstile);
    return 0;
}

/* Expected output (same on every run; the turnstile ends at 1, so this barrier
 * can be used only once; reuse needs two turnstiles):
 * mutex = 1, turnstile = 1, count = 3
 * passed before everyone arrived: 0
 */
