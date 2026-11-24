/* Multiprogramming model: if each process spends a fraction p of its time
 * waiting for I/O, CPU utilization with n processes in memory is 1 - p^n. */
#include <stdio.h>

int main(void)
{
    double p = 0.8;      /* I/O fraction */
    double pn = 1.0;
    for (int n = 1; n <= 5; n++) {
        pn = pn * p;       /* all waiting */
        double util = 1 - pn;
        printf("n=%d %.0f%%\n", n, util * 100);
    }
    return 0;
}

/* Expected output:
 * n=1 20%
 * n=2 36%
 * n=3 49%
 * n=4 59%
 * n=5 67%
 */
