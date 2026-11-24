/* Bitmap free-space search: in a 10-unit memory (1 = used, 0 = free) look for k
 * consecutive free units. The area found is allocated and its bits set to 1. */
#include <stdio.h>

/* look for k consecutive free (0) units */
int find_free(const char bit[], int n, int k)
{
    int start = 0, len = 0;
    for (int i = 0; i < n; i++) {
        if (bit[i] == 0) {
            if (len == 0) start = i;  /* new run */
            if (++len == k) return start;
        } else
            len = 0;                  /* run broken */
    }
    return -1;
}

static void print_bits(const char bit[], int n)
{
    printf("bit = ");
    for (int i = 0; i < n; i++)
        printf("%d", bit[i]);
    printf("\n");
}

int main(void)
{
    char bit[10] = {1, 1, 0, 0, 1, 0, 0, 0, 1, 1};
    int b;

    print_bits(bit, 10);
    b = find_free(bit, 10, 3);
    printf("find_free = %d\n", b);
    for (int i = b; i >= 0 && i < b + 3; i++)
        bit[i] = 1;                         /* allocate */
    print_bits(bit, 10);
    printf("find_free(k = 2) = %d\n", find_free(bit, 10, 2));
    return 0;
}

/* Expected output:
 * bit = 1100100011
 * find_free = 5
 * bit = 1100111111
 * find_free(k = 2) = 2
 */
