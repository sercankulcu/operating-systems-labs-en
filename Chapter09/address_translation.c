/* Address translation in demand paging: TLB first, page table on a miss,
 * and a page fault (read from disk into a free frame) if the page is not in memory.
 * Start: TLB holds (0,1) and (1,4); page 2 is in frame 5; page 3 is on disk. */
#include <stdio.h>

#define PAGES 16
#define TLB_SIZE 4

struct pt_entry { int valid, frame, R; };
struct tlb_entry { int used, p, f; };

static struct pt_entry pt[PAGES] = {
    [0] = {1, 1, 0}, [1] = {1, 4, 0}, [2] = {1, 5, 0}, [3] = {0, -1, 0},
};
static struct tlb_entry tlb[TLB_SIZE] = { {1, 0, 1}, {1, 1, 4} };
static int tlb_next = 2;        /* next slot to fill, FIFO */
static int free_frame = 7;
static int hits, misses, faults;

static int tlb_find(int p)
{
    for (int i = 0; i < TLB_SIZE; i++)
        if (tlb[i].used && tlb[i].p == p) {
            hits++;
            return tlb[i].f;
        }
    misses++;
    return -1;
}

static void tlb_add(int p, int f)
{
    tlb[tlb_next] = (struct tlb_entry){1, p, f};
    tlb_next = (tlb_next + 1) % TLB_SIZE;
}

static void page_fault(int p)
{
    faults++;
    printf("  page fault: reading page %d from disk into frame %d\n", p, free_frame);
    pt[p].frame = free_frame++;
    pt[p].valid = 1;
}

int translate(int va)
{
    int p = va >> 12;      /* page no */
    int d = va & 0xFFF;    /* offset */
    int f = tlb_find(p);   /* -1: miss */
    if (f < 0) {
        if (!pt[p].valid)    /* not in memory */
            page_fault(p);     /* from disk */
        f = pt[p].frame;
        tlb_add(p, f);
    }
    pt[p].R = 1;
    return (f << 12) | d;
}

int main(void)
{
    const int va[3] = {0x2A10, 0x2B04, 0x3008};

    for (int i = 0; i < 3; i++) {
        int pa = translate(va[i]);
        printf("0x%X -> 0x%X\n", (unsigned)va[i], (unsigned)pa);
    }
    printf("TLB hits: %d, misses: %d, page faults: %d\n", hits, misses, faults);
    return 0;
}

/* Expected output:
 * 0x2A10 -> 0x5A10
 * 0x2B04 -> 0x5B04
 *   page fault: reading page 3 from disk into frame 7
 * 0x3008 -> 0x7008
 * TLB hits: 1, misses: 2, page faults: 1
 */
