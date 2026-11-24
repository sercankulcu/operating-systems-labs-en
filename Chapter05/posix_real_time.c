/* POSIX real-time scheduling: a thread is to be created with the SCHED_FIFO
 * policy and priority 50. This needs privilege (root or CAP_SYS_NICE); without
 * it pthread_create returns EPERM and the program continues with the default
 * policy. The shell equivalent: chrt -f 50 ./prog */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>

static void *runner(void *param)
{
    (void)param;
    int policy;
    struct sched_param sp;
    pthread_getschedparam(pthread_self(), &policy, &sp);
    printf("runner: policy=%s priority=%d\n",
           policy == SCHED_FIFO ? "SCHED_FIFO" :
           policy == SCHED_RR ? "SCHED_RR" : "SCHED_OTHER",
           sp.sched_priority);
    return NULL;
}

int main(void)
{
    pthread_t tid;
    pthread_attr_t attr;
    struct sched_param sp;
    int policy;
    pthread_attr_init(&attr);
    pthread_attr_getschedpolicy(&attr, &policy);
    printf("default policy: %s\n",
           policy == SCHED_OTHER ? "SCHED_OTHER" : "other");
    /* policy is ignored unless inheritance is off */
    pthread_attr_setinheritsched(&attr,
                          PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    sp.sched_priority = 50;          /* 1..99 */
    pthread_attr_setschedparam(&attr, &sp);
    int r = pthread_create(&tid, &attr, runner, NULL);
    if (r != 0) {               /* EPERM expected */
        printf("SCHED_FIFO %s (%s); using the default policy\n",
               r == EPERM ? "not permitted" : "rejected", strerror(r));
        r = pthread_create(&tid, NULL, runner, NULL);
    }
    if (r != 0) {
        printf("pthread_create: %s\n", strerror(r));
        return 1;
    }
    pthread_join(tid, NULL);
    pthread_attr_destroy(&attr);
    return 0;
}

/* Expected output (as an ordinary user):
 * default policy: SCHED_OTHER
 * SCHED_FIFO not permitted (Operation not permitted); using the default policy
 * runner: policy=SCHED_OTHER priority=0
 * As root or with CAP_SYS_NICE, in place of the second and third lines:
 * runner: policy=SCHED_FIFO priority=50
 */
