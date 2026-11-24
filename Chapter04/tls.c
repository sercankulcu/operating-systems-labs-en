/* Thread-local storage (TLS): a plain global is shared by all threads;
 * _Thread_local (C11) gives each thread its own copy. */
#include <stdio.h>
#include <pthread.h>

int shared_error;               /* plain global: a single copy */
_Thread_local int error;        /* a separate copy for each thread */

struct result { int error, shared_error; };

static void *run(void *arg)
{
    struct result *r = arg;
    error = r->error;           /* writes to its own copy */
    shared_error = r->error;    /* writes to what everyone sees */
    r->error = error;           /* reads its own copy back */
    return NULL;
}

int main(void)
{
    struct result A = {1, 0}, B = {2, 0};
    pthread_t ta, tb;
    pthread_create(&ta, NULL, run, &A);
    pthread_join(ta, NULL);
    pthread_create(&tb, NULL, run, &B);
    pthread_join(tb, NULL);

    printf("A: error=%d\n", A.error);
    printf("B: error=%d\n", B.error);
    printf("main: error=%d (its own copy, initial value)\n", error);
    printf("main: shared_error=%d (last writer was B)\n", shared_error);
    return 0;
}

/* Expected output:
 * A: error=1
 * B: error=2
 * main: error=0 (its own copy, initial value)
 * main: shared_error=2 (last writer was B)
 */
