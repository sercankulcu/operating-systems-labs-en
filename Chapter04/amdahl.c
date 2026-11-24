/* Amdahl's law: if a fraction S of the program is serial, the speedup on N
 * cores is 1 / (S + (1 - S) / N), and it never exceeds 1 / S. */
#include <stdio.h>

int main(void)
{
    double S = 0.1;            /* serial fraction */
    int N = 1;                 /* cores */
    while (N <= 16) {
        double sp = 1 / (S + (1 - S) / N);
        printf("N=%2d  %.2fx\n", N, sp);
        N = N * 2;
    }
    printf("limit %.0fx\n", 1 / S);
    return 0;
}

/* Expected output:
 * N= 1  1.00x
 * N= 2  1.82x
 * N= 4  3.08x
 * N= 8  4.71x
 * N=16  6.40x
 * limit 10x
 */
