/* Best fit: the smallest hole that satisfies the request is chosen and the space
 * is taken from it. Holes: 120, 450, 230, 300, 180 K; requests 210 K and 160 K. */
#include <stdio.h>

int best_fit(int hole[], int n, int request)
{
    int best = -1;
    for (int i = 0; i < n; i++)
        if (hole[i] >= request &&
            (best == -1 || hole[i] < hole[best]))
            best = i;
    if (best != -1) hole[best] -= request;
    return best;
}

static void print_holes(const int hole[], int n)
{
    printf("  hole = [");
    for (int i = 0; i < n; i++)
        printf("%d%s", hole[i], i + 1 < n ? " " : "]\n");
}

int main(void)
{
    int hole[5] = {120, 450, 230, 300, 180};      /* K */

    printf("best_fit(210) = %d\n", best_fit(hole, 5, 210));
    print_holes(hole, 5);
    printf("best_fit(160) = %d\n", best_fit(hole, 5, 160));
    print_holes(hole, 5);
    return 0;
}

/* Expected output:
 * best_fit(210) = 2
 *   hole = [120 450 20 300 180]
 * best_fit(160) = 4
 *   hole = [120 450 20 300 20]
 */
