/* Translation inside the MMU (Tanenbaum Figure 3-9/3-10): 16-bit virtual address, 4 KiB pages.
 * The upper 4 bits are the page number, the lower 12 bits are copied as they are.
 * Instructions on the slide: MOV REG, 0 / 8192 / 20500 / 8196 / 32780 */
#include <stdint.h>
#include <stdio.h>

#define NUM_PAGES 16

struct entry {
    int present;                /* present/absent bit */
    unsigned frame;             /* 3 bits: 8 frames, 32 KiB physical */
};

/* page table (page -> frame): 0->2 1->1 2->6 3->0 4->4 5->3 9->5 11->7 */
static const struct entry table[NUM_PAGES] = {
    [0] = {1, 2}, [1] = {1, 1}, [2] = {1, 6}, [3] = {1, 0},
    [4] = {1, 4}, [5] = {1, 3}, [9] = {1, 5}, [11] = {1, 7},
};

/* Returns: physical address, or -1 if the page is absent (page fault) */
static long mmu_translate(uint16_t virt)
{
    unsigned p = virt >> 12;
    unsigned d = virt & 0xFFF;

    printf("virtual %5u: p = %2u, d = %4u -> ", (unsigned)virt, p, d);
    if (!table[p].present) {
        printf("page %u not mapped: PAGE FAULT\n", p);
        return -1;
    }
    long physical = ((long)table[p].frame << 12) | d;
    printf("frame %u, physical %ld\n", table[p].frame, physical);
    return physical;
}

int main(void)
{
    const uint16_t instr[] = {0, 8192, 20500, 8196, 32780};   /* MOV REG, address */

    for (size_t i = 0; i < sizeof instr / sizeof instr[0]; i++)
        mmu_translate(instr[i]);
    return 0;
}

/* Expected output:
 * virtual     0: p =  0, d =    0 -> frame 2, physical 8192
 * virtual  8192: p =  2, d =    0 -> frame 6, physical 24576
 * virtual 20500: p =  5, d =   20 -> frame 3, physical 12308
 * virtual  8196: p =  2, d =    4 -> frame 6, physical 24580
 * virtual 32780: p =  8, d =   12 -> page 8 not mapped: PAGE FAULT
 */
