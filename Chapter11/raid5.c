/* RAID 5 small write and recovery: when a single data block changes, only the old
 * data and the old parity are read (4 I/Os); a failed disk is rebuilt as the XOR
 * of the others. One stripe: D0, D1, D2 and parity P (one byte each). */
#include <stdio.h>
#include <stdint.h>

#define P 3                   /* number of the parity disk */

static uint8_t disk[P + 1] = { 0xB2, 0x69, 0xC5, 0x1E };   /* D0 D1 D2 P */
static int io_count = 0;

static uint8_t rd(int d)
{
    io_count++;
    return disk[d];
}

static void wr(int d, uint8_t v)
{
    io_count++;
    disk[d] = v;
}

static const char *binary(uint8_t v)
{
    static char s[2][10];
    static int idx = 0;
    char *b = s[idx];
    idx ^= 1;
    for (int i = 0; i < 8; i++) {
        b[i + (i >= 4)] = (char)('0' + ((v >> (7 - i)) & 1));
    }
    b[4] = ' ';
    b[9] = '\0';
    return b;
}

void small_write(int d, uint8_t nv)
{
    uint8_t ov = rd(d);      /* I/O 1 */
    uint8_t p = rd(P);       /* I/O 2 */
    p = p ^ ov ^ nv;
    wr(d, nv);               /* I/O 3 */
    wr(P, p);                /* I/O 4 */
}

uint8_t rebuild(int bad)
{
    uint8_t x = 0;
    for (int d = 0; d <= P; d++)
        if (d != bad)
            x ^= rd(d);          /* incl. P */
    return x;
}

int main(void)
{
    printf("P  = %s (D0 ^ D1 ^ D2)\n", binary(disk[P]));

    small_write(1, 0x33);
    printf("small_write(1, %s): %d I/Os, new P = %s\n",
           binary(0x33), io_count, binary(disk[P]));

    uint8_t lost = disk[2];
    disk[2] = 0;             /* disk 2 failed */
    io_count = 0;
    uint8_t x = rebuild(2);
    printf("rebuild(2) = %s, %d reads, %s\n", binary(x), io_count,
           x == lost ? "same as the lost D2" : "ERROR");
    return 0;
}

/* Expected output:
 * P  = 0001 1110 (D0 ^ D1 ^ D2)
 * small_write(1, 0011 0011): 4 I/Os, new P = 0100 0100
 * rebuild(2) = 1100 0101, 3 reads, same as the lost D2
 */
