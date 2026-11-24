/* Direct-mapped cache: 64 B lines, 4 sets. Each block maps to exactly one set;
 * blocks that fall into the same set evict each other (conflict miss).
 * The whole block number is stored instead of a tag. */
#include <stdio.h>

int main(void)
{
    int addr[6] = {0, 8, 64, 256, 0, 72};
    int tag[4] = {-1, -1, -1, -1};
    int hits = 0, misses = 0;

    for (int i = 0; i < 6; i++) {
        int block = addr[i] / 64;
        int set = block % 4;
        int before = hits;
        if (tag[set] == block) hits++;
        else { tag[set] = block; misses++; }
        printf("address %3d: block %d, set %d -> %s\n", addr[i], block, set,
               hits > before ? "hit" : "miss");
    }
    printf("tag = [%d, %d, %d, %d]\n", tag[0], tag[1], tag[2], tag[3]);
    printf("hits = %d, misses = %d\n", hits, misses);
    return 0;
}

/* Expected output:
 * address   0: block 0, set 0 -> miss
 * address   8: block 0, set 0 -> hit
 * address  64: block 1, set 1 -> miss
 * address 256: block 4, set 0 -> miss
 * address   0: block 0, set 0 -> miss
 * address  72: block 1, set 1 -> hit
 * tag = [0, 1, -1, -1]
 * hits = 2, misses = 4
 */
