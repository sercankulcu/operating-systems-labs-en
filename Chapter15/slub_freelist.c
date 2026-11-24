/* SLUB free list: a 4 KiB slab page is divided into four 1 KiB objects; free
 * objects are kept in a list, with the "next" pointer stored inside the free object
 * itself. Allocation takes the head of the list (LIFO); when the list runs out the
 * slow path allocates a new slab. kmem_cache is simulated in user space. */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#define PAGE 4096
#define OBJ 1024
#define MAXPAGES 8

typedef struct {
    void *freelist;
    char *page[MAXPAGES];    /* simulation: to print positions and to free */
    int npages;
} kmem_cache;

/* slow path: get a new page from the page allocator, split it into objects, return the first */
static void *new_slab(kmem_cache *c)
{
    char *s = aligned_alloc(PAGE, PAGE);
    if (!s || c->npages == MAXPAGES) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    c->page[c->npages++] = s;
    for (int i = 0; i < PAGE / OBJ; i++)
        *(void **)(s + i * OBJ) = (i + 1 < PAGE / OBJ) ? s + (i + 1) * OBJ : NULL;
    return s;
}

void *slab_alloc(kmem_cache *c)
{
    void *obj = c->freelist;      /* head of list */
    if (!obj)
        obj = new_slab(c);        /* slow path */
    c->freelist = *(void **)obj;   /* next */
    return obj;
}

void slab_free(kmem_cache *c, void *obj)
{
    *(void **)obj = c->freelist;   /* inside the object */
    c->freelist = obj;
}

/* position of the object: "+k" for the first page, "n:+k" for later ones */
static const char *position(kmem_cache *c, void *p)
{
    static char b[4][16];
    static int idx = 0;
    char *s = b[idx++ % 4];
    for (int i = 0; i < c->npages; i++) {
        ptrdiff_t k = (char *)p - c->page[i];
        if (k >= 0 && k < PAGE) {
            if (i == 0)
                snprintf(s, 16, "+%td", k);
            else
                snprintf(s, 16, "%d:+%td", i + 1, k);
            return s;
        }
    }
    return "?";
}

int main(void)
{
    kmem_cache c = { NULL, { NULL }, 0 };
    c.freelist = new_slab(&c);        /* start: one empty slab */

    /* a,b = alloc; free(a); c,d,e,f = alloc */
    void *a = slab_alloc(&c);
    void *b = slab_alloc(&c);
    printf("a = %s   b = %s", position(&c, a), position(&c, b));
    slab_free(&c, a);
    void *cc = slab_alloc(&c);
    printf("   c = %s\n", position(&c, cc));
    void *d = slab_alloc(&c);
    void *e = slab_alloc(&c);
    int before = c.npages;
    void *f = slab_alloc(&c);
    printf("d = %s   e = %s   f = %s%s\n", position(&c, d), position(&c, e), position(&c, f),
           c.npages > before ? " (new slab)" : "");

    for (int i = 0; i < c.npages; i++)
        free(c.page[i]);
    return 0;
}

/* Expected output:
 * a = +0   b = +1024   c = +0
 * d = +2048   e = +3072   f = 2:+0 (new slab)
 */
