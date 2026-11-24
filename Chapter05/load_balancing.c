/* Load balancing (pull): a CPU whose queue has emptied finds the busiest CPU
 * and moves half the difference in jobs from the tail of its queue to its own. */
#include <stdio.h>

#define NCPU 4
#define CAP 8

struct queue { char job[CAP]; int n; };
struct queue q[NCPU] = {
    {{'A', 'B', 'C'}, 3},
    {{0}, 0},
    {{'D', 'E', 'F', 'G', 'H'}, 5},
    {{'I', 'J'}, 2},
};

/* Moves the job at the tail of the source queue to the tail of the target. */
static void migrate(struct queue *source, struct queue *target)
{
    char job = source->job[--source->n];
    target->job[target->n++] = job;
    printf("  %c migrates\n", job);
}

void balance(int me)     /* idle CPU */
{
    int max = me;
    for (int c = 0; c < NCPU; c++)
        if (q[c].n > q[max].n) /* busier */
            max = c;
    int t = (q[max].n - q[me].n) / 2;
    printf("balance(%d): busiest CPU %d, %d jobs to move\n", me, max, t);
    for (int k = 0; k < t; k++)
        migrate(&q[max], &q[me]); /* from tail */
}

static void print_queues(void)
{
    for (int c = 0; c < NCPU; c++) {
        printf("  CPU %d (n=%d):", c, q[c].n);
        for (int i = 0; i < q[c].n; i++)
            printf(" %c", q[c].job[i]);
        printf("\n");
    }
}

int main(void)
{
    printf("before:\n");
    print_queues();
    balance(1);
    printf("after:\n");
    print_queues();
    return 0;
}

/* Expected output:
 * before:
 *   CPU 0 (n=3): A B C
 *   CPU 1 (n=0):
 *   CPU 2 (n=5): D E F G H
 *   CPU 3 (n=2): I J
 * balance(1): busiest CPU 2, 2 jobs to move
 *   H migrates
 *   G migrates
 * after:
 *   CPU 0 (n=3): A B C
 *   CPU 1 (n=2): H G
 *   CPU 2 (n=3): D E F
 *   CPU 3 (n=2): I J
 */
