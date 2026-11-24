/* Name resolution with a path cache: first the longest prefix in the cache is
 * looked up, the remaining components are resolved from disk (i-node + directory
 * block = 2 reads) and added to the cache. The Linux counterpart is the dcache. */
#include <stdio.h>
#include <string.h>

#define ROOT 1                /* i-node of the root directory */
#define CACHE_MAX 16

/* path -> i-node cache */
static struct { char path[64]; int ino; } cache[CACHE_MAX] = {
    { "/usr", 6 }, { "/usr/ast", 26 }, { "/usr/ast/mbox", 60 },
    { "/usr/bal", 45 }, { "/usr/bal/paper.ps", 85 },
};
static int cache_n = 5;

/* directories on disk: (directory i-node, name) -> i-node */
static const struct { int dir; const char *name; int ino; } disk[] = {
    { ROOT, "usr", 6 }, { ROOT, "etc", 14 }, { 6, "ast", 26 }, { 6, "bal", 45 },
    { 26, "mbox", 60 }, { 45, "paper.ps", 85 }, { 45, "thesis.tex", 131 },
    { 14, "passwd", 212 },
};

static int last_reads;        /* disk reads of the last resolution */

static void build_path(char *c[], int k, char *path, size_t size)
{
    size_t u = 0;
    path[0] = '\0';
    for (int i = 0; i < k && u < size; i++)
        u += (size_t)snprintf(path + u, size - u, "/%s", c[i]);
}

static int in_cache(char *c[], int k, int *ino)
{
    char path[64];
    build_path(c, k, path, sizeof path);
    for (int i = 0; i < cache_n; i++)
        if (strcmp(cache[i].path, path) == 0) {
            *ino = cache[i].ino;
            return 1;
        }
    return 0;
}

static void cache_add(char *c[], int k, int ino)
{
    if (cache_n == CACHE_MAX)
        return;
    build_path(c, k, cache[cache_n].path, sizeof cache[cache_n].path);
    cache[cache_n].ino = ino;
    cache_n++;
}

static int dir_lookup(int dir, const char *name)
{
    for (size_t i = 0; i < sizeof disk / sizeof disk[0]; i++)
        if (disk[i].dir == dir && strcmp(disk[i].name, name) == 0)
            return disk[i].ino;
    return -1;
}

/* c[]: "usr","bal","thesis.tex"; n = 3 */
int resolve(char *c[], int n)
{
    int k = n, ino = ROOT, reads = 0;
    while (k > 0 && !in_cache(c, k, &ino))
        k--;              /* longest prefix */
    for (int j = k; j < n; j++) {
        ino = dir_lookup(ino, c[j]);
        reads += 2;       /* i-node + block */
        cache_add(c, j + 1, ino);
    }
    last_reads = reads;
    return ino;
}

static void resolve_and_print(char *c[], int n)
{
    char path[64];
    build_path(c, n, path, sizeof path);
    int ino = resolve(c, n);
    printf("%s -> %d (%d reads)\n", path, ino, last_reads);
}

int main(void)
{
    char *thesis[] = { "usr", "bal", "thesis.tex" };
    char *passwd[] = { "etc", "passwd" };
    resolve_and_print(thesis, 3);
    resolve_and_print(thesis, 3);   /* same path again: the full path is cached */
    resolve_and_print(passwd, 2);
    return 0;
}

/* Expected output:
 * /usr/bal/thesis.tex -> 131 (2 reads)
 * /usr/bal/thesis.tex -> 131 (0 reads)
 * /etc/passwd -> 212 (4 reads)
 */
