/* Pthreads: create and wait. The runner thread writes the sum 1..5 into the
 * shared sum variable; main waits with join and prints the result.
 * (Adapted from Silberschatz Figure 4.11.) */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int sum;              /* shared */

void *runner(void *param)
{
    int upper = atoi(param);
    sum = 0;
    for (int i = 1; i <= upper; i++)
        sum += i;
    pthread_exit(0);
}

int main(void)
{
    pthread_t tid;
    pthread_create(&tid, NULL, runner, "5");
    pthread_join(tid, NULL);
    printf("sum = %d\n", sum);
    return 0;
}

/* Expected output:
 * sum = 15
 */
