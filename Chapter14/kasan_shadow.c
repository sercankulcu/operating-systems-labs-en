/* KASAN shadow memory check: 1 shadow byte for every 8-byte region.
 * 0: all valid, 1..7: first k bytes valid, < 0: redzone or freed.
 * A small "slab" (24 bytes) and its shadow are simulated in user space;
 * kasan_check is called before every access (like the check the compiler
 * inserts). */
#include <stdio.h>
#include <stdint.h>

#define KASAN_FREE       ((int8_t)0xfb)   /* -5 */
#define KASAN_REDZONE    ((int8_t)0xfc)   /* -4 */

static _Alignas(8) unsigned char slab[24];   /* 16-byte slot + redzone */
static int8_t shadow[sizeof slab / 8];
static uintptr_t SHADOW_OFS;
static unsigned char *p;                     /* start of the object, for the report */

static void report(void *a, int s)
{
    printf("BUG: KASAN: %s p+%td\n",
           s == KASAN_FREE ? "use-after-free" : "slab-out-of-bounds",
           (unsigned char *)a - p);
}

void kasan_check(void *a, int size)
{
    int8_t s = *(int8_t *)(((uintptr_t)a >> 3)
                           + SHADOW_OFS);
    if (s == 0) return;            /* fast path */
    if ((int)((uintptr_t)a & 7) + size - 1 >= s)
        report(a, s);
}

static void print_shadow(const char *label)
{
    printf("%-14s shadow =", label);
    for (size_t i = 0; i < sizeof shadow; i++)
        printf(" %02x", (unsigned)(uint8_t)shadow[i]);
    printf("\n");
}

static void *kmalloc(int n)
{
    /* single-slot allocator: a 16-byte slot followed by a redzone */
    for (size_t i = 0; i < sizeof shadow; i++)
        shadow[i] = KASAN_REDZONE;
    for (int i = 0; i < n / 8; i++)
        shadow[i] = 0;
    if (n % 8)
        shadow[n / 8] = (int8_t)(n % 8);
    return slab;
}

static void kfree(void *q)
{
    (void)q;
    shadow[0] = shadow[1] = KASAN_FREE;   /* slot goes to quarantine */
}

/* the check the compiler inserts + the read */
#define READ(q, i) (kasan_check(&(q)[i], 1), (q)[i])

int main(void)
{
    SHADOW_OFS = (uintptr_t)shadow - ((uintptr_t)slab >> 3);
    volatile unsigned char v;

    /* p = kmalloc(13); p[3]; p[12]; p[13]; */
    p = kmalloc(13);
    print_shadow("kmalloc(13):");
    v = READ(p, 3);
    v = READ(p, 12);
    v = READ(p, 13);
    /* kfree(p); p[0]; */
    kfree(p);
    print_shadow("kfree(p):");
    v = READ(p, 0);
    (void)v;
    return 0;
}

/* Expected output:
 * kmalloc(13):   shadow = 00 05 fc
 * BUG: KASAN: slab-out-of-bounds p+13
 * kfree(p):      shadow = fb fb fc
 * BUG: KASAN: use-after-free p+0
 * Note: the offset is cast to (int), so the comparison is signed; a negative
 * shadow value (redzone or freed) is reported at every offset. Without the cast
 * the comparison would be unsigned because of uintptr_t and freed memory would
 * never be reported.
 */
