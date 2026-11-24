/* Bitmap block allocation: on a 16-block disk (1 = used, 0 = free) the search starts
 * at the hint (just after the file's last block) and wraps around. The file ends at
 * block 3 and needs 3 more blocks: alloc_block(4), alloc_block(7), alloc_block(8). */
#include <stdio.h>

#define N 16

static int bitmap[N] = {1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1};

int alloc_block(int hint)
{
    for (int k = 0; k < N; k++) {
        int i = (hint + k) % N;
        if (bitmap[i] == 0) {  /* free? */
            bitmap[i] = 1;
            return i;
        }
    }
    return -1;               /* disk full */
}

int main(void)
{
    int last = 3;            /* last block of the file */

    printf("allocated blocks:");
    for (int j = 0; j < 3; j++) {
        last = alloc_block(last + 1);
        printf(" %d", last);
    }
    printf("\nbitmap = ");
    for (int i = 0; i < N; i++)
        printf("%d", bitmap[i]);
    printf("\n");
    return 0;
}

/* Expected output:
 * allocated blocks: 6 7 11
 * bitmap = 1101111111110111
 */
