/* CFS vruntime: on every tick the task with the smallest virtual runtime
 * (leftmost in the tree) runs; its vruntime grows by DELTA * 1024 / weight.
 * A linked list sorted by vruntime is used instead of a red-black tree.
 * A: nice 0 (weight 1024), B: nice 5 (weight 335). */
#include <stdio.h>

#define DELTA 3000  /* 3 ms, in microseconds */
/* nice 0 -> 1024, nice 5 -> 335 */

typedef struct Task {
    char name;
    int weight;
    long vruntime;
    int tick_count;
    struct Task *next;
} Task;

static Task *tree = NULL;       /* leftmost at the head */

/* Inserts AFTER equal vruntimes (the kernel's tree does the same). */
static void tree_insert(Task *g)
{
    Task **p = &tree;
    while (*p && (*p)->vruntime <= g->vruntime)
        p = &(*p)->next;
    g->next = *p;
    *p = g;
}

/* Removes the leftmost task from the tree (the running task is not in the tree). */
static Task *leftmost(void)
{
    Task *g = tree;
    tree = g->next;
    g->next = NULL;
    return g;
}

static void run(Task *g, int duration)
{
    (void)duration;             /* in reality: CPU for this long */
    g->tick_count++;
}

void tick(void)
{
    Task *g = leftmost();   /* leftmost */
    run(g, DELTA);
    g->vruntime += DELTA * 1024 / g->weight;
    tree_insert(g);     /* re-sort */
}

int main(void)
{
    Task A = {'A', 1024, 0, 0, NULL};
    Task B = {'B', 335, 0, 0, NULL};
    tree_insert(&A);
    tree_insert(&B);

    for (int i = 1; i <= 9; i++) {
        char running = tree->name;
        tick();
        printf("tick %d: %c  A.vruntime=%5ld  B.vruntime=%5ld\n",
               i, running, A.vruntime, B.vruntime);
    }
    printf("A=%d ticks, B=%d ticks\n", A.tick_count, B.tick_count);
    return 0;
}

/* Expected output:
 * tick 1: A  A.vruntime= 3000  B.vruntime=    0
 * tick 2: B  A.vruntime= 3000  B.vruntime= 9170
 * tick 3: A  A.vruntime= 6000  B.vruntime= 9170
 * tick 4: A  A.vruntime= 9000  B.vruntime= 9170
 * tick 5: A  A.vruntime=12000  B.vruntime= 9170
 * tick 6: B  A.vruntime=12000  B.vruntime=18340
 * tick 7: A  A.vruntime=15000  B.vruntime=18340
 * tick 8: A  A.vruntime=18000  B.vruntime=18340
 * tick 9: A  A.vruntime=21000  B.vruntime=18340
 * A=7 ticks, B=2 ticks
 */
