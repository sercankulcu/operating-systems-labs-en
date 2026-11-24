/* Exercises: the parent calls wait_for_child(), the child thread calls done() when its work is finished.
 * (a) no state variable: if the child finishes first, the signal is lost.
 * (b) no lock, with an if: the signal is lost between the check and the sleep.
 * Correct version: done flag + lock + while. Finally sem_down / sem_up built from one mutex
 * and one condition variable. Instead of sleeping forever, wait returns after 500 ms. */
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <errno.h>

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t c = PTHREAD_COND_INITIALIZER;
static int timed_out = 0;            /* did a wait return without anyone waking it */

static void lock(pthread_mutex_t *x)   { pthread_mutex_lock(x); }
static void unlock(pthread_mutex_t *x) { pthread_mutex_unlock(x); }
static void cond_signal_wrapper(pthread_cond_t *k) { pthread_cond_signal(k); }

static void deadline_in_500ms(struct timespec *deadline)
{
    clock_gettime(CLOCK_REALTIME, deadline);
    deadline->tv_nsec += 500000000L;      /* 500 ms */
    if (deadline->tv_nsec >= 1000000000L) {
        deadline->tv_sec += 1;
        deadline->tv_nsec -= 1000000000L;
    }
}

/* wait(&c, &m): together with the lock (bounded to 500 ms) */
static void cond_wait2(pthread_cond_t *k, pthread_mutex_t *x)
{
    struct timespec deadline;
    deadline_in_500ms(&deadline);
    if (pthread_cond_timedwait(k, x, &deadline) == ETIMEDOUT)
        timed_out = 1;
}

/* wait(&c): without a shared lock, as in (b); pthreads needs a mutex, so a mutex
 * private to this call is used (which means no lock) */
static void cond_wait1(pthread_cond_t *k)
{
    pthread_mutex_t private_lock;
    pthread_mutex_init(&private_lock, NULL);
    pthread_mutex_lock(&private_lock);
    cond_wait2(k, &private_lock);
    pthread_mutex_unlock(&private_lock);
    pthread_mutex_destroy(&private_lock);
}

/* The wait(&c, &m) and wait(&c) forms used on the slides: chosen by argument count.
 * They are macros so the names do not clash with <sys/wait.h> and <signal.h>. */
#define WAIT_SEL(_1, _2, NAME, ...) NAME
#define wait(...) WAIT_SEL(__VA_ARGS__, cond_wait2, cond_wait1, 0)(__VA_ARGS__)
#define signal cond_signal_wrapper

/* ---- (a) no state variable ---- */
void done_a(void)
{
    lock(&m);
    signal(&c);
    unlock(&m);
}

void wait_for_child_a(void)
{
    lock(&m);
    wait(&c, &m);
    unlock(&m);
}

/* ---- (b) no lock, with an if ---- */
static volatile int done_flag = 0;
static atomic_int checked = 0, child_finished = 0;

void done_b(void)
{
    done_flag = 1;
    signal(&c);
}

void wait_for_child_b(void)
{
    if (done_flag == 0) {
        /* <- the parent is interrupted here: the child sets done_flag = 1 and signals in vain */
        atomic_store(&checked, 1);
        while (!atomic_load(&child_finished)) {
            struct timespec ts = { 0, 1000000 };
            nanosleep(&ts, NULL);
        }
        wait(&c);
    }
}

/* ---- correct version ---- */
static int done_ok = 0;

void done(void)
{
    lock(&m);
    done_ok = 1;               /* the event is remembered */
    signal(&c);
    unlock(&m);
}

void wait_for_child(void)
{
    lock(&m);
    while (done_ok == 0)       /* check and sleep under the same lock */
        wait(&c, &m);
    unlock(&m);
}

/* ---- sem_down / sem_up: one mutex + one condition ---- */
static int value = 0;

void sem_down(void)
{
    lock(&m);
    while (value <= 0)
        wait(&c, &m);
    value--;
    unlock(&m);
}

void sem_up(void)
{
    lock(&m);
    value++;
    signal(&c);
    unlock(&m);
}

/* ---- child threads ---- */
static void *child_a(void *arg) { (void)arg; done_a(); return NULL; }
static void *child_b(void *arg)
{
    (void)arg;
    while (!atomic_load(&checked)) {
        struct timespec ts = { 0, 1000000 };
        nanosleep(&ts, NULL);
    }
    done_b();
    atomic_store(&child_finished, 1);
    return NULL;
}
static void *child_d(void *arg) { (void)arg; done(); return NULL; }
static atomic_int passed = 0;
static void *sem_worker(void *arg) { (void)arg; sem_down(); atomic_fetch_add(&passed, 1); return NULL; }

int main(void)
{
    pthread_t t;

    /* (a) the child finishes before the parent enters wait_for_child() */
    timed_out = 0;
    pthread_create(&t, NULL, child_a, NULL);
    pthread_join(t, NULL);
    wait_for_child_a();
    printf("(a) %s\n", timed_out ? "signal lost: the parent would never wake up"
                                 : "parent woke up");

    /* (b) the parent checks, is interrupted, then sleeps */
    timed_out = 0;
    pthread_create(&t, NULL, child_b, NULL);
    wait_for_child_b();
    pthread_join(t, NULL);
    printf("(b) %s\n", timed_out ? "signal lost: the parent would never wake up"
                                 : "parent woke up");

    /* correct version: even if the child finishes first, the event is kept in done_ok */
    timed_out = 0;
    pthread_create(&t, NULL, child_d, NULL);
    pthread_join(t, NULL);
    wait_for_child();
    printf("correct: %s\n", timed_out ? "timeout" : "parent continued immediately");

    /* sem_down / sem_up */
    pthread_t s[3];
    for (int i = 0; i < 3; i++)
        pthread_create(&s[i], NULL, sem_worker, NULL);
    for (int i = 0; i < 3; i++)
        sem_up();
    for (int i = 0; i < 3; i++)
        pthread_join(s[i], NULL);
    printf("sem_down/sem_up: %d threads passed, value = %d\n",
           atomic_load(&passed), value);
    return 0;
}

/* Expected output (same on every run; (a) and (b) take 500 ms each):
 * (a) signal lost: the parent would never wake up
 * (b) signal lost: the parent would never wake up
 * correct: parent continued immediately
 * sem_down/sem_up: 3 threads passed, value = 0
 */
