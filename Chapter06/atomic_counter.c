/* Atomic variables (C11 <stdatomic.h>): atomic_fetch_add is lock-free and correct.
 * For comparison the same loop is also run with a plain int (race condition). */
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_int counter = 0;     /* _Atomic int */
static volatile int plain = 0;   /* unprotected: counter++ is three instructions */

void *work(void *arg)
{
    (void)arg;
    for (int n = 0; n < 100000; n++)
        atomic_fetch_add(&counter, 1);
    return NULL;
}

static void *plain_work(void *arg)
{
    (void)arg;
    for (int n = 0; n < 100000; n++)
        plain = plain + 1;
    return NULL;
}

int main(void)
{
    pthread_t t[2];
    for (int i = 0; i < 2; i++)
        pthread_create(&t[i], NULL, work, NULL);
    for (int i = 0; i < 2; i++)
        pthread_join(t[i], NULL);
    printf("atomic_fetch_add: %d\n", atomic_load(&counter));   /* 2 threads + join -> 200000 */

    for (int i = 0; i < 2; i++)
        pthread_create(&t[i], NULL, plain_work, NULL);
    for (int i = 0; i < 2; i++)
        pthread_join(t[i], NULL);
    printf("plain int       : %d\n", plain);
    return 0;
}

/* Expected output (the first line is always 200000; the second line is usually
 * below 200000 because of the race condition and varies on every run):
 * atomic_fetch_add: 200000
 * plain int       : 1xxxxx
 */
