/* Buffer cache: on a hit bread moves the buffer to the tail of the LRU list;
 * on a miss it reuses the oldest buffer, writing it to disk first if it is dirty.
 * 3 buffers: blocks 5, 9 (dirty), 2. Requests: 9, 7, 2, 4. */
#include <stdio.h>

#define NBUF 3

typedef struct Buffer {
    int blk;
    int dirty;
    struct Buffer *prev, *next;         /* LRU list: head is oldest */
} Buffer;

static Buffer pool[NBUF];
static Buffer *head, *tail;
static int reads, writes, last_written = -1;

/* Small example: linear search in the pool instead of a hash table */
static Buffer *hash_find(int blk)
{
    for (int i = 0; i < NBUF; i++)
        if (pool[i].blk == blk)
            return &pool[i];
    return NULL;
}

static void lru_remove(Buffer *t)
{
    if (t->prev) t->prev->next = t->next; else head = t->next;
    if (t->next) t->next->prev = t->prev; else tail = t->prev;
    t->prev = t->next = NULL;
}

static void lru_to_tail(Buffer *t)          /* make it the newest */
{
    if (t->prev || t->next || head == t)
        lru_remove(t);
    t->prev = tail;
    t->next = NULL;
    if (tail) tail->next = t; else head = t;
    tail = t;
}

static Buffer *lru_head(void)               /* the oldest */
{
    return head;
}

static void disk_write(Buffer *t)
{
    writes++;
    last_written = t->blk;
    t->dirty = 0;
}

static void disk_read(Buffer *t)
{
    (void)t;
    reads++;
}

Buffer *bread(int blk)
{
    Buffer *t = hash_find(blk);
    if (t != NULL) {         /* hit */
        lru_to_tail(t);
        return t;
    }
    t = lru_head();          /* oldest */
    if (t->dirty)
        disk_write(t);       /* write back */
    t->blk = blk; t->dirty = 0;
    disk_read(t);            /* miss: I/O */
    lru_to_tail(t);
    return t;
}

static void print_list(void)
{
    printf("   LRU (old -> new):");
    for (Buffer *t = head; t != NULL; t = t->next)
        printf(" %d%s", t->blk, t->dirty ? "*" : "");
    printf("\n");
}

int main(void)
{
    const int blk[3] = {5, 9, 2}, dirty[3] = {0, 1, 0};
    const int req[4] = {9, 7, 2, 4};

    for (int i = 0; i < NBUF; i++) {
        pool[i].blk = blk[i];
        pool[i].dirty = dirty[i];
        lru_to_tail(&pool[i]);
    }
    print_list();
    for (int i = 0; i < 4; i++) {
        int r = reads, w = writes;
        bread(req[i]);
        if (reads == r)
            printf("%d: hit\n", req[i]);
        else if (writes > w)
            printf("%d: miss (%d written back)\n", req[i], last_written);
        else
            printf("%d: miss\n", req[i]);
        print_list();
    }
    printf("total: reads = %d, writes = %d\n", reads, writes);
    return 0;
}

/* Expected output (* dirty):
 *    LRU (old -> new): 5 9* 2
 * 9: hit
 *    LRU (old -> new): 5 2 9*
 * 7: miss
 *    LRU (old -> new): 2 9* 7
 * 2: hit
 *    LRU (old -> new): 9* 7 2
 * 4: miss (9 written back)
 *    LRU (old -> new): 7 2 4
 * total: reads = 2, writes = 1
 */
