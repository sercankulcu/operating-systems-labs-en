/* Buddy allocator: starts with one 256 KB block (order 8, unit KB).
 * alloc(k) returns a 2^k KB block, splitting a larger block if needed; release()
 * merges with the buddy (address XOR size) if it is free. A = 21 KB -> 32 KB, B = 64 KB. */
#include <stdio.h>
#include <stdlib.h>

#define MAX 8                   /* 2^8 = 256 KB */

struct node {
    int addr;
    struct node *next;
};

static struct node *fl[MAX + 1];  /* fl[k]: free blocks of 2^k KB */

static void push(int k, int a)
{
    struct node *d = malloc(sizeof *d);
    d->addr = a;
    d->next = fl[k];
    fl[k] = d;
}

static int pop(int k)                /* take the block at the head of fl[k] */
{
    struct node *d = fl[k];
    int a = d->addr;
    fl[k] = d->next;
    free(d);
    return a;
}

static int remove_block(int k, int a)   /* if a is in fl[k], remove it: 1 */
{
    for (struct node **p = &fl[k]; *p != NULL; p = &(*p)->next) {
        if ((*p)->addr == a) {
            struct node *d = *p;
            *p = d->next;
            free(d);
            return 1;
        }
    }
    return 0;
}

int alloc(int k)           /* 2^k KB; enough free blocks assumed */
{
    int j = k;
    while (fl[j] == NULL) j++;
    int a = pop(j);
    while (j > k) {   /* split, right half free */
        j--; push(j, a + (1 << j));
    }
    return a;
}

void release(int a, int k)
{
    int b = a ^ (1 << k);    /* buddy */
    while (k < MAX && remove_block(k, b)) {
        a = a < b ? a : b;     /* merge */
        k++; b = a ^ (1 << k);
    }
    push(k, a);
}

static void print_lists(const char *title)
{
    printf("%s\n", title);
    for (int k = 0; k <= MAX; k++) {
        if (fl[k] == NULL)
            continue;
        printf("  fl[%d] (%3d KB):", k, 1 << k);
        for (struct node *d = fl[k]; d != NULL; d = d->next)
            printf(" @%d", d->addr);
        printf("\n");
    }
}

static int order(int kb)             /* round the request up to a power of 2 */
{
    int k = 0;
    while ((1 << k) < kb)
        k++;
    return k;
}

int main(void)
{
    push(MAX, 0);
    print_lists("Start:");

    int A = alloc(order(21));
    printf("A = @%d (%d KB), internal fragmentation %d KB\n", A, 1 << order(21),
           (1 << order(21)) - 21);
    int B = alloc(order(64));
    printf("B = @%d (%d KB)\n", B, 1 << order(64));
    print_lists("After A and B:");

    release(A, order(21));
    print_lists("release(0, 5): merged with @32, @64 is in use -> stopped:");
    release(B, order(64));
    print_lists("release(64, 6): 64 -> 128 -> 256 KB:");

    while (fl[MAX] != NULL)
        pop(MAX);
    return 0;
}

/* Expected output:
 * Start:
 *   fl[8] (256 KB): @0
 * A = @0 (32 KB), internal fragmentation 11 KB
 * B = @64 (64 KB)
 * After A and B:
 *   fl[5] ( 32 KB): @32
 *   fl[7] (128 KB): @128
 * release(0, 5): merged with @32, @64 is in use -> stopped:
 *   fl[6] ( 64 KB): @0
 *   fl[7] (128 KB): @128
 * release(64, 6): 64 -> 128 -> 256 KB:
 *   fl[8] (256 KB): @0
 */
