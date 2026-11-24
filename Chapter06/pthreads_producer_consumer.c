/* Pthreads producer-consumer (Tanenbaum Figure 2-32): a one-slot buffer, one mutex and two
 * condition variables (condp: the producer waits, condc: the consumer waits).
 * The producer puts 1..MAX, the consumer prints them in order. */
#include <stdio.h>
#include <pthread.h>

#define MAX 2

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condc = PTHREAD_COND_INITIALIZER,
               condp = PTHREAD_COND_INITIALIZER;
int buffer = 0;                    /* 0 = empty */

void *producer(void *p)
{
    (void)p;
    for (int i = 1; i <= MAX; i++) {  /* MAX = 2 */
        pthread_mutex_lock(&m);
        while (buffer != 0) pthread_cond_wait(&condp, &m);
        buffer = i;
        pthread_cond_signal(&condc);
        pthread_mutex_unlock(&m);
    }
    return NULL;
}

void *consumer(void *p)
{
    (void)p;
    for (int i = 1; i <= MAX; i++) {
        pthread_mutex_lock(&m);
        while (buffer == 0) pthread_cond_wait(&condc, &m);
        printf("%d\n", buffer); buffer = 0;
        pthread_cond_signal(&condp);
        pthread_mutex_unlock(&m);
    }
    return NULL;
}

int main(void)
{
    pthread_t pro, con;
    pthread_create(&con, NULL, consumer, NULL);
    pthread_create(&pro, NULL, producer, NULL);
    pthread_join(pro, NULL);
    pthread_join(con, NULL);
    pthread_cond_destroy(&condc);
    pthread_cond_destroy(&condp);
    pthread_mutex_destroy(&m);
    return 0;
}

/* Expected output (same on every run):
 * 1
 * 2
 */
