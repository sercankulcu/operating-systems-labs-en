/* Semaphore wait queue: wait (P/down) and signal (V/up) with a value and a FIFO process
 * list. Kernel simulation: a single "kernel" mutex makes wait/signal atomic (in place of
 * disabling interrupts), and sleep/wakeup are done with each process's own condition
 * variable. The trace from the slide: S = 1; P1 enters, P2 and P3 block (value = -2),
 * then they are woken in order and value returns to 1. */
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

/* wait, signal and sleep also exist in libc with other meanings (<sys/wait.h>,
 * <signal.h>, <unistd.h>). To avoid clashes they are renamed AFTER the headers;
 * the source uses the names from the slides. */
#define wait   semaphore_wait
#define signal semaphore_signal
#define sleep  process_sleep

struct process {
    int id;
    int woken;
    pthread_cond_t wake;
    struct process *next;
};

typedef struct {
    int value;
    struct process *list;
} semaphore;

static pthread_mutex_t kernel = PTHREAD_MUTEX_INITIALIZER;
static _Thread_local struct process *this_proc;

static void add(struct process **list, struct process *p)    /* to the end of the queue */
{
    p->next = NULL;
    while (*list != NULL)
        list = &(*list)->next;
    *list = p;
}

static struct process *remove_first(struct process **list)   /* FIFO: from the front */
{
    struct process *p = *list;
    *list = p->next;
    p->next = NULL;
    return p;
}

static void print_queue(const semaphore *S)
{
    printf(" (queue:");
    for (const struct process *p = S->list; p != NULL; p = p->next)
        printf(" P%d", p->id);
    printf(")\n");
}

/* called with the kernel lock held; the lock is released while sleeping */
static void sleep(void)
{
    while (!this_proc->woken)
        pthread_cond_wait(&this_proc->wake, &kernel);
    this_proc->woken = 0;
}

static void wakeup(struct process *P)
{
    P->woken = 1;
    pthread_cond_signal(&P->wake);
}

void wait(semaphore *S)          /* P / down */
{
    pthread_mutex_lock(&kernel);
    S->value--;
    printf("P%d wait  : value = %2d", this_proc->id, S->value);
    if (S->value < 0) {
        add(&S->list, this_proc);
        printf(" -> blocked");
        print_queue(S);
        sleep();                 /* block */
        printf("P%d woke up -> critical section\n", this_proc->id);
    } else {
        printf(" -> critical section\n");
    }
    pthread_mutex_unlock(&kernel);
}

void signal(semaphore *S)        /* V / up */
{
    struct process *P;
    pthread_mutex_lock(&kernel);
    S->value++;
    printf("P%d signal: value = %2d", this_proc->id, S->value);
    if (S->value <= 0) {
        P = remove_first(&S->list);   /* FIFO */
        printf(" -> P%d woken", P->id);
        print_queue(S);
        wakeup(P);
    } else {
        printf(" -> queue empty\n");
    }
    pthread_mutex_unlock(&kernel);
}

/* ---- trace scenario ---- */
static semaphore S = { 1, NULL };
static atomic_int cs_owner = 0;
static atomic_int go_on[4];

static void short_wait(void)
{
    struct timespec ts = { 0, 1000000 };   /* 1 ms */
    nanosleep(&ts, NULL);
}

static void *process(void *arg)
{
    struct process me;
    me.id = *(int *)arg;
    me.woken = 0;
    me.next = NULL;
    pthread_cond_init(&me.wake, NULL);
    this_proc = &me;

    wait(&S);
    atomic_store(&cs_owner, me.id);          /* critical section */
    while (!atomic_load(&go_on[me.id]))
        short_wait();
    signal(&S);

    pthread_cond_destroy(&me.wake);
    return NULL;
}

static int read_value(void)
{
    pthread_mutex_lock(&kernel);
    int v = S.value;
    pthread_mutex_unlock(&kernel);
    return v;
}

int main(void)
{
    pthread_t t[4];
    int id[4] = { 0, 1, 2, 3 };

    printf("S.value = %d\n", S.value);
    pthread_create(&t[1], NULL, process, &id[1]);
    while (atomic_load(&cs_owner) != 1)
        short_wait();
    pthread_create(&t[2], NULL, process, &id[2]);
    while (read_value() != -1)
        short_wait();
    pthread_create(&t[3], NULL, process, &id[3]);
    while (read_value() != -2)
        short_wait();
    for (int i = 1; i <= 3; i++) {          /* P1, P2, P3 leave in turn */
        while (atomic_load(&cs_owner) != i)
            short_wait();
        atomic_store(&go_on[i], 1);
    }
    for (int i = 1; i <= 3; i++)
        pthread_join(t[i], NULL);
    printf("S.value = %d\n", S.value);
    return 0;
}

/* Expected output (same on every run):
 * S.value = 1
 * P1 wait  : value =  0 -> critical section
 * P2 wait  : value = -1 -> blocked (queue: P2)
 * P3 wait  : value = -2 -> blocked (queue: P2 P3)
 * P1 signal: value = -1 -> P2 woken (queue: P3)
 * P2 woke up -> critical section
 * P2 signal: value =  0 -> P3 woken (queue:)
 * P3 woke up -> critical section
 * P3 signal: value =  1 -> queue empty
 * S.value = 1
 */
