/* link and unlink: a file gets a second name, then both names are removed.
 * Because the file is held by an open descriptor it can still be read; the
 * i-node is freed only when close closes the last descriptor.
 * Temporary directories under /tmp stand in for /usr/jim and /usr/ast. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

static char root[] = "/tmp/os02_linkXXXXXX";
static char jim[64], ast[64], memo[96], note[96];

static void print_links(int fd, const char *event)
{
    struct stat st;
    if (fstat(fd, &st) == 0)
        printf("%-22s link count=%lu size=%lld\n", event,
               (unsigned long)st.st_nlink, (long long)st.st_size);
}

/* Creates the 85-byte file /usr/jim/memo. */
static int prepare(void)
{
    char content[85];
    if (mkdtemp(root) == NULL)
        return -1;
    snprintf(jim, sizeof jim, "%s/jim", root);
    snprintf(ast, sizeof ast, "%s/ast", root);
    snprintf(memo, sizeof memo, "%s/memo", jim);
    snprintf(note, sizeof note, "%s/note", ast);
    if (mkdir(jim, 0755) == -1 || mkdir(ast, 0755) == -1)
        return -1;
    memset(content, 'm', sizeof content);
    int fd = creat(memo, 0644);
    if (fd == -1)
        return -1;
    if (write(fd, content, sizeof content) != (ssize_t)sizeof content) {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

int main(void)
{
    char buf[100];
    ssize_t n;

    if (prepare() == -1) {
        perror("prepare");
        return 1;
    }

    int fd = open(memo, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    printf("fd=%d\n", fd);
    print_links(fd, "open(memo)");
    if (link(memo, note) == -1)
        perror("link");
    print_links(fd, "link(memo, note)");
    unlink(memo);
    print_links(fd, "unlink(memo)");
    unlink(note);
    print_links(fd, "unlink(note)");
    n = read(fd, buf, 100);    /* still readable */
    printf("n=%zd\n", n);
    close(fd);

    rmdir(jim);
    rmdir(ast);
    rmdir(root);
    return 0;
}

/* Expected output:
 * fd=3
 * open(memo)             link count=1 size=85
 * link(memo, note)       link count=2 size=85
 * unlink(memo)           link count=1 size=85
 * unlink(note)           link count=0 size=85
 * n=85
 */
