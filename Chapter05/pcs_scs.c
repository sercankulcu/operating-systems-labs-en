/* Thread scheduling scope: PCS (contention within the process) and SCS
 * (system-wide contention). Linux, with its 1:1 model, supports only
 * PTHREAD_SCOPE_SYSTEM; a PTHREAD_SCOPE_PROCESS request is rejected with ENOTSUP. */
#include <stdio.h>
#include <string.h>
#include <pthread.h>

static void *runner(void *param)
{
    (void)param;
    printf("runner ran\n");
    return NULL;
}

int main(void)
{
    pthread_t tid;
    pthread_attr_t attr;
    int scope;
    pthread_attr_init(&attr);
    pthread_attr_getscope(&attr, &scope);
    if (scope == PTHREAD_SCOPE_PROCESS)
        printf("PCS\n");
    else if (scope == PTHREAD_SCOPE_SYSTEM)
        printf("SCS\n");

    int r = pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
    printf("PROCESS scope: %s\n", r == 0 ? "accepted" : strerror(r));

    /* Linux and macOS: SYSTEM only */
    pthread_attr_setscope(&attr,
                          PTHREAD_SCOPE_SYSTEM);
    pthread_create(&tid, &attr, runner, NULL);
    pthread_join(tid, NULL);
    pthread_attr_destroy(&attr);
    return 0;
}

/* Expected output (Linux/glibc):
 * SCS
 * PROCESS scope: Operation not supported
 * runner ran
 */
