/* Non-reproducible execution: the same two processes give different results with a different
 * interleaving. First three possible orders are played by hand (deterministically) with
 * n = 5, then count++ and count-- are interleaved at the machine-instruction level (T0..T5). */
#include <stdio.h>

/* Process 1:  repeat  n = n + 1;
 * Process 2:  repeat  print(n); n = 0;
 * Runs a single round in the given order: 'A' = n = n + 1, 'P' = print, 'S' = n = 0 */
static void run_order(const char *name, const char *order)
{
    int n = 5;
    int printed = -1;
    for (const char *p = order; *p != '\0'; p++) {
        if (*p == 'A')
            n = n + 1;
        else if (*p == 'P')
            printed = n;          /* print(n) */
        else if (*p == 'S')
            n = 0;
    }
    printf("%-28s -> prints %d, ends with n = %d\n", name, printed, n);
}

int main(void)
{
    printf("possible orders with n = 5:\n");
    run_order("n=n+1; print; n=0", "APS");
    run_order("print; n=0; n=n+1", "PSA");
    run_order("print; n=n+1; n=0", "PAS");   /* the increment is lost */

    /* count++ and count-- are three instructions on a machine; the T0..T5 order from the slide */
    int count = 5;
    int reg1, reg2;
    reg1 = count;          /* T0 producer: reg1 = count     (5) */
    reg1 = reg1 + 1;       /* T1 producer: reg1 = reg1 + 1  (6) */
    reg2 = count;          /* T2 consumer: reg2 = count     (5) */
    reg2 = reg2 - 1;       /* T3 consumer: reg2 = reg2 - 1  (4) */
    count = reg1;          /* T4 producer: count = reg1     (6) */
    printf("count after T4 = %d\n", count);
    count = reg2;          /* T5 consumer: count = reg2     (4) */
    printf("count after T5 = %d (should be 5)\n", count);
    return 0;
}

/* Expected output:
 * possible orders with n = 5:
 * n=n+1; print; n=0            -> prints 6, ends with n = 0
 * print; n=0; n=n+1            -> prints 5, ends with n = 1
 * print; n=n+1; n=0            -> prints 5, ends with n = 0
 * count after T4 = 6
 * count after T5 = 4 (should be 5)
 */
