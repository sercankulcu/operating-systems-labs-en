/* Pop-up threads: for each incoming message a new thread is created to
 * handle it and detached; it is never joined. Before the program exits, a
 * counter is used to wait until all detached threads have finished. */
#include <stdio.h>
#include <pthread.h>

#define MESSAGE_COUNT 3

static const char *messages[MESSAGE_COUNT] = {"GET /a", "GET /b", "GET /c"};
static char result[MESSAGE_COUNT][64];
static int finished = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t done = PTHREAD_COND_INITIALIZER;

static void *handle(void *arg)
{
    const char **msg = arg;
    int no = (int)(msg - messages);
    snprintf(result[no], sizeof result[no], "message %d (%s) handled", no, *msg);
    pthread_mutex_lock(&lock);
    finished++;
    pthread_cond_signal(&done);
    pthread_mutex_unlock(&lock);
    return NULL;
}

int main(void)
{
    for (int i = 0; i < MESSAGE_COUNT; i++) {
        const char **msg = &messages[i];
        /* on message arrival */
        pthread_t t;
        pthread_create(&t, NULL, handle, msg);
        pthread_detach(t); /* no join */
    }

    /* Detached threads cannot be joined: wait on the finish counter. */
    pthread_mutex_lock(&lock);
    while (finished < MESSAGE_COUNT)
        pthread_cond_wait(&done, &lock);
    pthread_mutex_unlock(&lock);

    for (int i = 0; i < MESSAGE_COUNT; i++)   /* print in order */
        printf("%s\n", result[i]);
    return 0;
}

/* Expected output (threads may finish in any order; the print order is
 * fixed):
 * message 0 (GET /a) handled
 * message 1 (GET /b) handled
 * message 2 (GET /c) handled
 */
