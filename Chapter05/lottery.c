/* Lottery scheduling: a ticket is drawn in every time slice; the process
 * whose cumulative ticket count in the list exceeds the drawn number gets
 * the CPU. A has 30, B 10, C 60 tickets. The first three draws are fixed as
 * in the slides (47, 35, 12), then 10,000 draws are made with srand(42).
 * (OSTEP Chapter 9) */
#include <stdio.h>
#include <stdlib.h>

typedef struct node_t {
    const char *name;
    int tickets;
    struct node_t *next;
} node_t;

static const int fixed_draws[] = {47, 35, 12};
static int draw_no = 0;

/* A number in [low, high); the first three draws are fixed. */
static int get_random(int low, int high)
{
    if (draw_no < 3)
        return fixed_draws[draw_no++];
    return low + rand() % (high - low);
}

static int run_count[3];
static int verbose = 1;

static void run(node_t *n)
{
    if (verbose)
        printf("%s runs\n", n->name);
    run_count[n->name[0] - 'A']++;
}

static node_t *head;
static int totaltickets = 0;

static void time_slice(void)
{
    /* one draw per time slice */
    int counter = 0;
    int winner = get_random(0, totaltickets);
    node_t *current = head;
    while (current) {
        counter = counter + current->tickets;
        if (counter > winner)
            break;          /* found the winner */
        current = current->next;
    }
    /* current: process that gets the CPU */
    if (verbose)
        printf("winner=%d -> ", winner);
    run(current);
}

int main(void)
{
    static node_t C = {"C", 60, NULL};
    static node_t B = {"B", 10, &C};
    static node_t A = {"A", 30, &B};
    head = &A;
    for (node_t *n = head; n; n = n->next)
        totaltickets += n->tickets;

    for (int i = 0; i < 3; i++)
        time_slice();

    srand(42);
    verbose = 0;
    for (int i = 0; i < 3; i++)
        run_count[i] = 0;
    for (int i = 0; i < 10000; i++)
        time_slice();
    printf("10000 slices: A %d, B %d, C %d\n",
           run_count[0], run_count[1], run_count[2]);
    return 0;
}

/* Expected output (the numbers on the last line depend on glibc rand(); they are
 * about 3000 / 1000 / 6000):
 * winner=47 -> C runs
 * winner=35 -> B runs
 * winner=12 -> A runs
 * 10000 slices: A ~3000, B ~1000, C ~6000
 */
