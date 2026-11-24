/* Non-deadlock bug 2: order violation. T1 assumes that the assignment
 * "worker = thread_create(body)" happens before the new thread (T2) enters body();
 * nothing enforces this order. Fix: a ready flag + condition variable. */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct worker_t {
    pthread_t tid;
    int state;
};

static struct worker_t *worker;             /* T1 assigns, T2 reads */
static int fixed;                           /* 0: buggy, 1: with ready flag */
static int ready;
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t c = PTHREAD_COND_INITIALIZER;

/* helper wrapping pthread_create; it returns 50 ms late (slow return) */
static struct worker_t *thread_create(void *(*f)(void *))
{
    struct worker_t *w = malloc(sizeof *w);
    w->state = 7;
    pthread_create(&w->tid, NULL, f, NULL);
    usleep(50000);
    return w;
}

/* T2: body(), the new thread */
static void *body(void *arg)
{
    (void)arg;
    int s;

    if (fixed) {
        pthread_mutex_lock(&m);
        while (!ready)
            pthread_cond_wait(&c, &m);
        pthread_mutex_unlock(&m);
    }
    pthread_mutex_lock(&m);                 /* to make the read visible */
    struct worker_t *w = worker;
    pthread_mutex_unlock(&m);
    if (w == NULL) {                        /* the slide's worker->state would crash here */
        printf("T2: worker not assigned yet (NULL)! order violation\n");
        return NULL;
    }
    s = w->state;  /* is worker set? */
    printf("T2: s = worker->state = %d\n", s);
    return NULL;
}

/* T1: init() */
static void init(void)
{
    struct worker_t *w = thread_create(body);

    pthread_mutex_lock(&m);
    worker = w;
    ready = 1;
    pthread_cond_signal(&c);
    pthread_mutex_unlock(&m);
}

static void cleanup(void)
{
    pthread_join(worker->tid, NULL);
    free(worker);
    worker = NULL;
    ready = 0;
}

int main(void)
{
    printf("Buggy version:\n");
    fixed = 0;
    init();
    cleanup();

    printf("with ready flag + condition variable:\n");
    fixed = 1;
    init();
    cleanup();
    return 0;
}

/* Expected output (timing dependent; the 50 ms delay makes this result almost certain):
 * Buggy version:
 * T2: worker not assigned yet (NULL)! order violation
 * with ready flag + condition variable:
 * T2: s = worker->state = 7
 */
