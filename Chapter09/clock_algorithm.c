/* Clock (second chance) page replacement: the hand clears R of frames with R = 1
 * and moves on; the first frame with R = 0 is the victim. A dirty (M = 1) victim is
 * written to disk first. Four frames, hand at frame 2; fault(9) and fault(4). */
#include <stdio.h>

#define N 4

struct frame { int page, R, M; };

static struct frame frm[N] = {
    {3, 1, 0}, {8, 0, 1}, {5, 1, 0}, {6, 1, 1},
};
static int hand = 2;
static int written = 0;

static void write_back(int k)
{
    (void)k;
    written++;
}

int pick_victim(void)
{
    while (frm[hand].R == 1) { /* 2nd chance */
        frm[hand].R = 0;
        hand = (hand + 1) % N;
    }
    int k = hand;            /* victim */
    hand = (hand + 1) % N;
    return k;
}

void fault(int p)
{
    int k = pick_victim();
    if (frm[k].M) write_back(k); /* dirty */
    frm[k].page = p; frm[k].R = 1;
    frm[k].M = 0;
}

static void print_state(void)
{
    for (int i = 0; i < N; i++)
        printf("  f%d: p%d R=%d M=%d\n", i, frm[i].page, frm[i].R, frm[i].M);
    printf("  hand = %d, written to disk: %d\n", hand, written);
}

static void fault_and_report(int p)
{
    struct frame before[N];

    for (int i = 0; i < N; i++)
        before[i] = frm[i];
    fault(p);
    for (int k = 0; k < N; k++)
        if (frm[k].page == p)
            printf("fault(%d): victim f%d (p%d, %s)\n", p, k, before[k].page,
                   before[k].M ? "dirty" : "clean");
}

int main(void)
{
    printf("Start:\n");
    print_state();
    fault_and_report(9);
    fault_and_report(4);
    printf("Final state:\n");
    print_state();
    return 0;
}

/* Expected output:
 * Start:
 *   f0: p3 R=1 M=0
 *   f1: p8 R=0 M=1
 *   f2: p5 R=1 M=0
 *   f3: p6 R=1 M=1
 *   hand = 2, written to disk: 0
 * fault(9): victim f1 (p8, dirty)
 * fault(4): victim f2 (p5, clean)
 * Final state:
 *   f0: p3 R=0 M=0
 *   f1: p9 R=1 M=0
 *   f2: p4 R=1 M=0
 *   f3: p6 R=0 M=1
 *   hand = 3, written to disk: 1
 */
