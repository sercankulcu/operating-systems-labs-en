/* Round Robin (q = 4): the process at the head runs for at most q; if it is
 * not finished it goes to the end of the queue. Processes that arrive during
 * the slice are added to the queue BEFORE the preempted process. If the queue
 * is empty while a process has not yet arrived, the CPU idles and the clock
 * jumps to the next arrival. Output: the completion time of each process. */
#include <stdio.h>

typedef struct {
    const char *name;
    int arrival, burst, left;
} Proc;

/* ---- ready queue (circular array) ---- */
#define CAP 16
static Proc *queue[CAP];
static int head = 0, count = 0;

static int empty(void) { return count == 0; }

static void enqueue(Proc *s)
{
    queue[(head + count) % CAP] = s;
    count++;
}

static Proc *dequeue(void)
{
    Proc *s = queue[head];
    head = (head + 1) % CAP;
    count--;
    return s;
}

static int min(int x, int y) { return x < y ? x : y; }

/* Adds the processes that have arrived by time t and are not yet queued. */
static void add_arrivals(Proc p[], int n, int *k, int t)
{
    while (*k < n && p[*k].arrival <= t) {
        enqueue(&p[*k]);
        (*k)++;
    }
}

/* p[] sorted by arrival time */
void rr(Proc p[], int n, int q)
{
    int t = 0, k = 0;     /* k: next arrival */
    add_arrivals(p, n, &k, t);
    while (!empty() || k < n) {
        if (empty()) {       /* CPU idle */
            t = p[k].arrival;  /* next arrival */
            add_arrivals(p, n, &k, t);
        }
        Proc *s = dequeue(); /* head */
        int slice = min(q, s->left);
        t += slice; s->left -= slice;
        add_arrivals(p, n, &k, t); /* first */
        if (s->left > 0) enqueue(s);   /* then */
        else printf("%s %d\n", s->name, t);
    }
}

int main(void)
{
    Proc p[] = {
        {"P1", 0, 8, 8},
        {"P2", 1, 4, 4},
        {"P3", 2, 9, 9},
        {"P4", 3, 5, 5},
    };
    rr(p, 4, 4);
    return 0;
}

/* Expected output (process and completion time):
 * P2 8
 * P1 20
 * P4 25
 * P3 26
 */
