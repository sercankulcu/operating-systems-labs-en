/* Program structure and locality: a 128 x 128 int array, each row is one page
 * (128 words), the process has 64 frames (LRU). Column-by-column traversal faults
 * on every access, row-by-row traversal faults once per row. Page faults are counted
 * by simulation. */
#include <stdio.h>

#define FRAMES 64

int data[128][128];   /* row = 1 page */

static int frm[FRAMES];         /* page in the frame, -1 free */
static long last_use[FRAMES];
static long clk;
static long faults;

static void reset(void)
{
    for (int k = 0; k < FRAMES; k++) {
        frm[k] = -1;
        last_use[k] = 0;
    }
    clk = 0;
    faults = 0;
}

/* access to data[i][...]: is page i in memory? if not, load it over the LRU victim */
static void touch(int page)
{
    int victim = 0;

    clk++;
    for (int k = 0; k < FRAMES; k++) {
        if (frm[k] == page) {
            last_use[k] = clk;
            return;
        }
        if (last_use[k] < last_use[victim])
            victim = k;
    }
    faults++;
    frm[victim] = page;
    last_use[victim] = clk;
}

int main(void)
{
    reset();
    /* A: column by column */
    for (int j = 0; j < 128; j++)
        for (int i = 0; i < 128; i++) {
            touch(i);
            data[i][j] = 0;
        }
    printf("A (column by column): %ld faults\n", faults);

    reset();
    /* B: row by row */
    for (int i = 0; i < 128; i++)
        for (int j = 0; j < 128; j++) {
            touch(i);
            data[i][j] = 0;
        }
    printf("B (row by row): %ld faults\n", faults);
    return 0;
}

/* Expected output:
 * A (column by column): 16384 faults
 * B (row by row): 128 faults
 */
