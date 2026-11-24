/* FAT chain: in the in-memory FAT every entry points to the file's next block.
 * find_block follows the chain to find which disk block holds a given byte offset.
 * File A: 4 -> 7 -> 2 -> 10 -> 12, file B: 6 -> 3 -> 11 -> 14. BLOCK = 1024. */
#include <stdio.h>

#define BLOCK 1024
#define END  (-1)               /* end of chain */

static int fat[16] = {         /* unspecified entries are 0: free block */
    [2] = 10, [3] = 11, [4] = 7, [6] = 3, [7] = 2,
    [10] = 12, [11] = 14, [12] = END, [14] = END,
};

int find_block(int first, long off)
{
    int b = first;
    long n = off / BLOCK;  /* block index */
    while (n > 0 && b != END) {
        b = fat[b];          /* follow chain */
        n--;
    }
    return b;          /* END: past EOF */
}

static void try_find(int first, long off)
{
    int b = find_block(first, off);

    if (b == END)
        printf("find_block(%d, %ld) = END (past EOF)\n", first, off);
    else
        printf("find_block(%d, %ld) = %d, byte %ld within the block\n", first, off, b,
               off % BLOCK);
}

int main(void)
{
    try_find(4, 3500);
    try_find(6, 5000);
    return 0;
}

/* Expected output:
 * find_block(4, 3500) = 10, byte 428 within the block
 * find_block(6, 5000) = END (past EOF)
 */
