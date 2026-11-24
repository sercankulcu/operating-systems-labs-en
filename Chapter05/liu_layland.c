/* Liu-Layland test (rate-monotonic, RM): if total utilization U <= n(2^(1/n) - 1),
 * RM meets all deadlines (1). If U <= 1 the test cannot decide (0); if
 * U > 1 no algorithm can meet them (-1). */
#include <stdio.h>
#include <math.h>

typedef struct { int p, t; } Task;   /* p: period, t: execution time */

int rm_test(Task g[], int n)
{
    double U = 0;
    for (int i = 0; i < n; i++)
        U += (double) g[i].t / g[i].p;
    double bound = n * (pow(2, 1.0/n) - 1);
    printf("  U=%.4f bound=%.4f\n", U, bound);
    if (U <= bound) return 1; /* enough */
    if (U <= 1.0) return 0;   /* unknown */
    return -1;            /* impossible */
}

int main(void)
{
    Task g1[] = { {20, 5}, {50, 10}, {100, 30} };
    Task g2[] = { {50, 25}, {80, 35} };
    Task g3[] = { {10, 6}, {10, 5} };       /* extra example: U > 1 */

    printf("rm_test = %d\n", rm_test(g1, 3));
    printf("rm_test = %d\n", rm_test(g2, 2));
    printf("rm_test = %d\n", rm_test(g3, 2));
    return 0;
}

/* Expected output:
 *   U=0.7500 bound=0.7798
 * rm_test = 1
 *   U=0.9375 bound=0.8284
 * rm_test = 0
 *   U=1.1000 bound=0.8284
 * rm_test = -1
 */
