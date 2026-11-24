/* Non-deadlock bug 1: atomicity violation. T1 checks "name is not NULL?"; before
 * it uses the value, T2 steps in and sets name to NULL. Fix: the check + use and
 * T2's write are under the same mutex. The window is widened with usleep. */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

struct session_t {
    const char *name;
    pthread_mutex_t lock;
};

static struct session_t session_obj = { "ayse", PTHREAD_MUTEX_INITIALIZER };
static struct session_t *sess = &session_obj;

/* ---- Buggy version ---- */
static void *T1_buggy(void *arg)
{
    (void)arg;
    if (sess->name != NULL) {
        usleep(100000);                         /* window for the interleaving */
        const char *name = sess->name;
        if (name != NULL)
            puts(name);
        else    /* the slide's puts(sess->name) would be puts(NULL) here: crash */
            puts("T1: name was not NULL at the check, now NULL! (atomicity violation)");
    }
    return NULL;
}

static void *T2_buggy(void *arg)
{
    (void)arg;
    usleep(20000);                              /* after T1 has checked */
    sess->name = NULL;
    return NULL;
}

/* ---- Fixed version: same mutex ---- */
static void *T1(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&sess->lock);
    if (sess->name != NULL) {
        usleep(100000);                         /* same window, but under the lock */
        puts(sess->name);  /* can no longer be NULL */
    }
    pthread_mutex_unlock(&sess->lock);
    return NULL;
}

static void *T2(void *arg)
{
    (void)arg;
    usleep(20000);
    pthread_mutex_lock(&sess->lock);
    sess->name = NULL;
    pthread_mutex_unlock(&sess->lock);
    return NULL;
}

static void run(void *(*f1)(void *), void *(*f2)(void *))
{
    pthread_t a, b;

    sess->name = "ayse";
    pthread_create(&a, NULL, f1, NULL);
    pthread_create(&b, NULL, f2, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("after: sess->name = %s\n\n", sess->name ? sess->name : "NULL");
}

int main(void)
{
    printf("Buggy version:\n");
    run(T1_buggy, T2_buggy);
    printf("With mutex:\n");
    run(T1, T2);
    return 0;
}

/* Expected output (timing dependent; the 100 ms window makes this result almost certain):
 * Buggy version:
 * T1: name was not NULL at the check, now NULL! (atomicity violation)
 * after: sess->name = NULL
 *
 * With mutex:
 * ayse
 * after: sess->name = NULL
 */
