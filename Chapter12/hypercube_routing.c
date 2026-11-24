/* Hypercube routing: the message hops to neighbor nodes by fixing the differing bits
 * of the address one by one from left to right; the shortest path equals the number
 * of differing bits. Example: in a 4-dimensional cube 0110 (6) -> 1101 (13). */
#include <stdio.h>

void route(int src, int dst, int d)
{
    int u = src;
    for (int b = d - 1; b >= 0; b--) {
        int m = 1 << b;
        if ((u ^ dst) & m) {  /* bit differs */
            u ^= m;           /* go to neighbor */
            printf("%d ", u);
        }
    }
}

int main(void)
{
    printf("6 -> 13: ");
    route(6, 13, 4);
    printf("\n0 -> 15: ");
    route(0, 15, 4);
    printf("\n");
    return 0;
}

/* Expected output:
 * 6 -> 13: 14 12 13
 * 0 -> 15: 8 12 14 15
 */
