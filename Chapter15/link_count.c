/* Link count and unlink: a hard link is a new directory entry and increases the
 * inode's link count; a symbolic link holds only a path and dangles when the target
 * is deleted. Even when the link count is 0 the data lives on while a descriptor is
 * open. The example works in a temporary directory; inode numbers depend on the machine. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

static char dir[] = "/tmp/link_XXXXXX";

static void links(const char *when, int fd)
{
    struct stat st;
    if (fstat(fd, &st) == 0)
        printf("  %-18s inode link count = %lu\n", when, (unsigned long)st.st_nlink);
}

int main(void)
{
    if (!mkdtemp(dir)) { perror("mkdtemp"); return 1; }
    if (chdir(dir) != 0) { perror("chdir"); return 1; }

    int fd = open("a.txt", O_CREAT|O_RDWR, 0644);
    printf("fd = %d\n", fd);
    if (write(fd, "data", 4) != 4)
        perror("write");
    links("open:", fd);
    if (link("a.txt", "b.txt") != 0)      /* hard link */
        perror("link");
    links("link:", fd);
    if (symlink("a.txt", "s.txt") != 0)   /* symbolic */
        perror("symlink");
    links("symlink:", fd);
    unlink("a.txt");
    links("unlink(a.txt):", fd);
    int r1 = open("b.txt", O_RDONLY);
    printf("r1 = %d\n", r1);
    int r2 = open("s.txt", O_RDONLY);
    printf("r2 = %d (%s)\n", r2, errno == ENOENT ? "ENOENT" : strerror(errno));
    unlink("b.txt");
    links("unlink(b.txt):", fd);

    char buf[8] = { 0 };
    ssize_t n = read(r1, buf, sizeof buf - 1);
    printf("  read from r1: \"%s\" (%zd bytes; no name left but the data lives on)\n", buf, n);
    close(r1);
    close(fd);                   /* last descriptor: inode and data are freed */

    unlink("s.txt");             /* cleanup: the dangling symbolic link */
    if (chdir("/") != 0)
        perror("chdir");
    rmdir(dir);
    return 0;
}

/* Expected output:
 * fd = 3
 *   open:              inode link count = 1
 *   link:              inode link count = 2
 *   symlink:           inode link count = 2
 *   unlink(a.txt):     inode link count = 1
 * r1 = 4
 * r2 = -1 (ENOENT)
 *   unlink(b.txt):     inode link count = 0
 *   read from r1: "data" (4 bytes; no name left but the data lives on)
 * (the fd numbers are 3 and 4 if no other descriptors are open through inheritance)
 */
