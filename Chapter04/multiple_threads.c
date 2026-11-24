/* Multiple threads (data parallelism): the sum 1..1000 is split among four
 * threads; each one writes only to its own partial[k] cell. */
#include <pthread.h>
#include <stdio.h>

#define N 4
long partial[N];         /* one cell each */

void *add_range(void *arg)
{
    long k = (long)arg;    /* 0 .. N-1 */
    for (long i = k*250 + 1; i <= (k+1)*250; i++)
        partial[k] += i;
    return NULL;
}

int main(void)
{
    pthread_t t[N];
    for (long k = 0; k < N; k++)
        pthread_create(&t[k], NULL, add_range, (void *)k);
    long total = 0;
    for (int k = 0; k < N; k++) {
        pthread_join(t[k], NULL);
        total += partial[k];
    }
    printf("%ld\n", total);  /* 500500 */
    return 0;
}

/* Expected output:
 * 500500
 */
