/* Lock-free stack: push uses a single atomic top pointer and a CAS loop instead of a lock.
 * If the CAS fails, C11 writes the current top into old and the loop retries. First the
 * trace from the slide (T1 is pushing X while T2 pushes Y) is played, then 4 threads
 * push 10000 nodes each. */
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>

typedef struct node {
    char name;
    int value;
    struct node *next;
} node_t;

static _Atomic(node_t *) top = NULL;

void push(node_t *n)
{
    node_t *old = atomic_load(&top);
    do {
        n->next = old;
    } while (!atomic_compare_exchange_weak(
                 &top, &old, n));
}  /* on failure old = current top */

static void print_stack(const char *title)
{
    printf("%-34s top", title);
    for (node_t *p = atomic_load(&top); p != NULL; p = p->next)
        printf(" -> %c", p->name);
    printf("\n");
}

#define NUM_THREADS 4
#define COUNT 10000
static node_t nodes[NUM_THREADS][COUNT];

static void *worker(void *arg)
{
    int t = *(int *)arg;
    for (int k = 0; k < COUNT; k++) {
        nodes[t][k].name = '#';
        nodes[t][k].value = 1;
        push(&nodes[t][k]);
    }
    return NULL;
}

int main(void)
{
    /* 1) The trace from the slide */
    node_t A = { 'A', 0, NULL }, B = { 'B', 0, NULL };
    node_t X = { 'X', 0, NULL }, Y = { 'Y', 0, NULL };
    push(&B);
    push(&A);
    print_stack("initial:");

    node_t *old = atomic_load(&top);           /* T1: old = A */
    X.next = old;                               /* T1: X.next = A */
    push(&Y);                                   /* T2: CAS(top, A, Y) succeeds */
    print_stack("T2 pushed Y:");
    bool ok = atomic_compare_exchange_strong(&top, &old, &X);
    printf("T1: CAS(top, A, X) %s, old = %c\n", ok ? "succeeded" : "failed", old->name);
    X.next = old;                               /* retry: X.next = Y */
    ok = atomic_compare_exchange_strong(&top, &old, &X);
    printf("T1: CAS(top, Y, X) %s\n", ok ? "succeeded" : "failed");
    print_stack("final:");

    /* 2) Real threads */
    atomic_store(&top, NULL);
    pthread_t t[NUM_THREADS];
    int id[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        id[i] = i;
        pthread_create(&t[i], NULL, worker, &id[i]);
    }
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(t[i], NULL);
    long n = 0;
    for (node_t *p = atomic_load(&top); p != NULL; p = p->next)
        n += p->value;
    printf("%d threads x %d pushes: nodes on the stack = %ld\n", NUM_THREADS, COUNT, n);
    return 0;
}

/* Expected output (same on every run):
 * initial:                           top -> A -> B
 * T2 pushed Y:                       top -> Y -> A -> B
 * T1: CAS(top, A, X) failed, old = Y
 * T1: CAS(top, Y, X) succeeded
 * final:                             top -> X -> Y -> A -> B
 * 4 threads x 10000 pushes: nodes on the stack = 40000
 */
