/* Space-time trade-off: four ways to count the 1 bits of a byte.
 * (a) loop, (b) macro, (c) 256-byte table, (d) 16-byte nibble table. In the slides
 * (a), (b), (c) all use the name bit_count; here the macros are defined in turn
 * and removed with #undef. */
#include <stdio.h>

/* (a) loop */
#define BYTE_SIZE 8
int bit_count(int byte)
{
    int i, count = 0;
    for (i = 0; i < BYTE_SIZE; i++)
        if ((byte >> i) & 1)
            count++;
    return count;
}

/* (b) macro: no loop */
#define bit_count(b) ((b&1) + ((b>>1)&1) + \
    ((b>>2)&1) + ((b>>3)&1) + ((b>>4)&1) + \
    ((b>>5)&1) + ((b>>6)&1) + ((b>>7)&1))
static int with_macro(int x) { return bit_count(x); }
#undef bit_count

/* (c) table: 256 bytes */
char bits[256] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
};
#define bit_count(b) (int) bits[b]
static int with_table(int x) { return bit_count(x); }
#undef bit_count

/* (d) nibble table */
char nib[16];            /* 16 bytes */
void build_table(void)
{
    nib[0] = 0;
    for (int i = 1; i < 16; i++)
        nib[i] = (i & 1) + nib[i >> 1];
}

int bit_cnt(unsigned char b)
{
    return nib[b & 0x0F] + nib[b >> 4];
}

int main(void)
{
    build_table();
    printf("nib[] =");
    for (int i = 0; i < 16; i++)
        printf(" %d", nib[i]);
    printf("\n");

    int samples[] = { 0xB7, 0x5C };
    for (int k = 0; k < 2; k++) {
        int b = samples[k];
        printf("0x%02X: loop %d, macro %d, table %d\n",
               b, bit_count(b), with_macro(b), with_table(b));
        printf("bit_cnt(0x%02X) = %d\n", b, bit_cnt((unsigned char)b));
    }
    return 0;
}

/* Expected output:
 * nib[] = 0 1 1 2 1 2 2 3 1 2 2 3 2 3 3 4
 * 0xB7: loop 6, macro 6, table 6
 * bit_cnt(0xB7) = 6
 * 0x5C: loop 4, macro 4, table 4
 * bit_cnt(0x5C) = 4
 */
