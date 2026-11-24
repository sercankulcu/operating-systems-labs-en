/* Diffie-Hellman key exchange: modpow by square-and-multiply.
 * Public p = 23, g = 5; Alice's secret a = 6, Bob's secret b = 15.
 * Both compute the same shared secret (5^(a*b) mod 23). */
#include <stdio.h>

long modpow(long g, long e, long p)
{
    long r = 1;
    while (e > 0) {
        if (e & 1) r = r * g % p;
        g = g * g % p;
        e >>= 1;
    }
    return r;
}

int main(void)
{
    /* public: p = 23, g = 5 */
    long A  = modpow(5, 6, 23);
    long B  = modpow(5, 15, 23);
    long sA = modpow(B, 6, 23);
    long sB = modpow(A, 15, 23);
    printf("A = %ld\n", A);
    printf("B = %ld\n", B);
    printf("sA = %ld\n", sA);
    printf("sB = %ld\n", sB);
    return 0;
}

/* Expected output:
 * A = 8
 * B = 19
 * sA = 2
 * sB = 2
 */
