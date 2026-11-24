/* Average access time in a memory hierarchy: L1, L2, L3, DRAM.
 * Start from the bottom (DRAM); each level is t = time + miss * (time of the level below). */
#include <stdio.h>

int main(void)
{
    /* L1, L2, L3, DRAM access time (ns) */
    double time_ns[4] = {1, 4, 20, 80};
    double miss[3] = {0.10, 0.40, 0.50};
    double t = time_ns[3];      /* DRAM */
    for (int k = 2; k >= 0; k--)
        t = time_ns[k] + miss[k] * t;
    printf("average = %.1f ns\n", t);
    return 0;
}

/* Expected output (intermediate values: t = 80 -> 60 -> 28 -> 3.8):
 * average = 3.8 ns
 */
