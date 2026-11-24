/* Same order: two processes acquire two resources in the same order (resource_1
 * first, then resource_2). Even if the release order differs there is no deadlock
 * (Tanenbaum Figure 6-2a). down/up are implemented with POSIX semaphores
 * (sem_wait/sem_post). */
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

#define ROUNDS 100000

static sem_t resource_1, resource_2;
static long counter = 0;        /* shared data protected by the two resources */

static void down(sem_t *s)
{
    while (sem_wait(s) != 0) {
        /* interrupted by a signal (EINTR): retry */
    }
}

static void up(sem_t *s)
{
    sem_post(s);
}

static void use_both_resources(void)
{
    counter++;
}

static void process_A(void)
{
    down(&resource_1);
    down(&resource_2);
    use_both_resources();
    up(&resource_2);
    up(&resource_1);
}

static void process_B(void)
{
    down(&resource_1);
    down(&resource_2);
    use_both_resources();
    up(&resource_2);
    up(&resource_1);
}

static void *run_A(void *arg)
{
    (void)arg;
    for (int i = 0; i < ROUNDS; i++)
        process_A();
    return NULL;
}

static void *run_B(void *arg)
{
    (void)arg;
    for (int i = 0; i < ROUNDS; i++)
        process_B();
    return NULL;
}

int main(void)
{
    pthread_t a, b;

    sem_init(&resource_1, 0, 1);
    sem_init(&resource_2, 0, 1);

    pthread_create(&a, NULL, run_A, NULL);
    pthread_create(&b, NULL, run_B, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);

    printf("counter = %ld (expected %d)\n", counter, 2 * ROUNDS);
    printf("no deadlock: both processes finished\n");

    sem_destroy(&resource_1);
    sem_destroy(&resource_2);
    return 0;
}

/* Expected output:
 * counter = 200000 (expected 200000)
 * no deadlock: both processes finished
 */
