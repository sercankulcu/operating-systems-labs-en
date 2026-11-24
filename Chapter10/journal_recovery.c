/* Journal recovery: at boot the journal is scanned; the blocks of transactions
 * committed with TxB ... TxE are rewritten to their final locations, and a
 * transaction without TxE is discarded. The system crashed while writing
 * transaction 3: 10 records. */
#include <stdio.h>
#include <string.h>

enum { TXB, BLOCK, TXE };

typedef struct {
    int type;
    const char *dest;           /* BLOCK: final location (I7, B, I9, D40) */
    const char *data;           /* BLOCK: version */
} Rec;

/* Final locations on disk and their contents */
static struct { const char *name; const char *version; } disk[] = {
    {"I7", "v0"}, {"B", "v0"}, {"I9", "v0"}, {"D40", "v0"},
};
#define DISK_SIZE (int)(sizeof disk / sizeof disk[0])

static void disk_write(const char *dest, const char *data)
{
    for (int i = 0; i < DISK_SIZE; i++)
        if (strcmp(disk[i].name, dest) == 0) {
            disk[i].version = data;
            printf("  write(%s, %s)\n", dest, data);
        }
}

void recover(Rec g[], int n)
{
    int i = 0;
    while (i < n && g[i].type == TXB) {
        int j = i + 1;
        while (j < n && g[j].type == BLOCK)
            j++;
        if (j == n || g[j].type != TXE)
            break;              /* uncommitted: drop */
        for (int k = i + 1; k < j; k++)
            disk_write(g[k].dest, g[k].data);
        i = j + 1;          /* next */
    }
    if (i < n)
        printf("  transaction starting at g[%d] is uncommitted: discarded\n", i);
}

int main(void)
{
    Rec g[] = {
        {TXB, NULL, NULL}, {BLOCK, "I7", "v1"}, {BLOCK, "B", "v1"}, {TXE, NULL, NULL},
        {TXB, NULL, NULL}, {BLOCK, "I9", "v2"}, {BLOCK, "D40", "v2"}, {TXE, NULL, NULL},
        {TXB, NULL, NULL}, {BLOCK, "I7", "v3"},
    };
    int n = (int)(sizeof g / sizeof g[0]);

    printf("recover(g, %d):\n", n);
    recover(g, n);
    printf("final locations:");
    for (int i = 0; i < DISK_SIZE; i++)
        printf(" %s=%s", disk[i].name, disk[i].version);
    printf("\n");
    return 0;
}

/* Expected output:
 * recover(g, 10):
 *   write(I7, v1)
 *   write(B, v1)
 *   write(I9, v2)
 *   write(D40, v2)
 *   transaction starting at g[8] is uncommitted: discarded
 * final locations: I7=v1 B=v1 I9=v2 D40=v2
 */
