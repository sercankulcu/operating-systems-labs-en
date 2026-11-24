/* Selecting pending signals: on return to user mode the kernel picks the
 * lowest-numbered signal from the set ready = pending & ~blocked and delivers it.
 * A blocked signal is not lost, it waits. The task structure and the delivery are
 * simulated in user space (SIGINT does not really terminate the process). */
#include <stdio.h>
#include <signal.h>

typedef struct {
    unsigned long pend;      /* pending signals: bit (sig - 1) */
    unsigned long blk;       /* blocked signals */
} task;

#define BIT(sig) (1UL << ((sig) - 1))

static void deliver(task *t, int sig)
{
    (void)t;
    if (sig == SIGUSR1)
        printf("deliver: %d (SIGUSR1)\n", sig);
    else if (sig == SIGCHLD)
        printf("deliver: %d (SIGCHLD)\n", sig);
    else if (sig == SIGINT)
        printf("deliver: %d (SIGINT -> default: terminate)\n", sig);
    else
        printf("deliver: %d\n", sig);
}

/* on return to user mode (kernel) */
void check_signals(task *t)
{
    unsigned long h = t->pend & ~t->blk;
    while (h) {
        int sig = __builtin_ctzl(h) + 1;
        t->pend &= ~(1UL << (sig - 1));
        deliver(t, sig); /* handler/default */
        h = t->pend & ~t->blk;
    }
}

int main(void)
{
    task t = { 0, 0 };

    /* before:  sigprocmask(SIG_BLOCK, {SIGINT}) */
    t.blk |= BIT(SIGINT);
    /* while the process sleeps: Ctrl-C (2), kill -USR1 (10), child exited (17) */
    t.pend |= BIT(SIGINT) | BIT(SIGUSR1) | BIT(SIGCHLD);
    printf("return to user mode:\n");
    check_signals(&t);
    printf("still pending: %s\n", (t.pend & BIT(SIGINT)) ? "SIGINT" : "-");

    /* after: sigprocmask(SIG_UNBLOCK, {SIGINT}) */
    t.blk &= ~BIT(SIGINT);
    printf("on return from sigprocmask(SIG_UNBLOCK):\n");
    check_signals(&t);
    return 0;
}

/* Expected output (Linux x86-64: SIGINT 2, SIGUSR1 10, SIGCHLD 17):
 * return to user mode:
 * deliver: 10 (SIGUSR1)
 * deliver: 17 (SIGCHLD)
 * still pending: SIGINT
 * on return from sigprocmask(SIG_UNBLOCK):
 * deliver: 2 (SIGINT -> default: terminate)
 */
