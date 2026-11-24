/* Translation with a two-level page table: 32-bit address = 10 + 10 + 12 bits.
 * In the outer table only entry 2 is valid; in its second-level table entry 1 ->
 * frame 0x2F. page_fault() imitates the MMU trap with setjmp/longjmp. */
#include <inttypes.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>

typedef struct pte {
    unsigned p;                 /* present bit */
    uint32_t frame;             /* second level: physical frame */
    struct pte *table;          /* outer level: location of the second-level table */
} pte_t;

static pte_t outer[1024];       /* outer (page directory) table */
static pte_t second[1024];      /* second-level table that outer[2] points to */
static jmp_buf trap;

static _Noreturn void page_fault(uint32_t va)
{
    printf("0x%08" PRIX32 " -> PAGE FAULT (trap to the kernel)\n", va);
    longjmp(trap, 1);
}

/* 32 bits: 10 + 10 + 12 bits */
uint32_t translate(uint32_t va)
{
    uint32_t p1 = va >> 22;
    uint32_t p2 = (va >> 12) & 0x3FF;
    uint32_t d  = va & 0xFFF;
    printf("va = 0x%08" PRIX32 ": p1 = %" PRIu32 ", p2 = %" PRIu32
           ", d = 0x%03" PRIX32 "\n", va, p1, p2, d);
    pte_t e1 = outer[p1];
    if (!e1.p) page_fault(va);
    pte_t e2 = e1.table[p2];
    if (!e2.p) page_fault(va);
    return (e2.frame << 12) | d;
}

static const uint32_t addr[2] = { 0x00801A3C, 0x01403004 };
static int idx;                 /* not a local, so longjmp does not affect it */

static void attempt(void)
{
    if (setjmp(trap) == 0) {
        uint32_t pa = translate(addr[idx]);
        printf("0x%08" PRIX32 " -> 0x%08" PRIX32 "\n", addr[idx], pa);
    }
}

int main(void)
{
    outer[2].p = 1;
    outer[2].table = second;
    second[1].p = 1;
    second[1].frame = 0x2F;

    for (idx = 0; idx < 2; idx++)
        attempt();
    return 0;
}

/* Expected output:
 * va = 0x00801A3C: p1 = 2, p2 = 1, d = 0xA3C
 * 0x00801A3C -> 0x0002FA3C
 * va = 0x01403004: p1 = 5, p2 = 3, d = 0x004
 * 0x01403004 -> PAGE FAULT (trap to the kernel)
 */
