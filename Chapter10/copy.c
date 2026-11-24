/* File copy (copy abc xyz): a read/write loop with a 4096-byte buffer.
 * The 10000-byte abc is copied in 3 rounds (4096 + 4096 + 1808); 4 reads, 3 writes.
 * The files are created in /tmp and deleted at the end.
 * Usage in the slides: $ copy abc xyz */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF 4096

static int reads, writes;       /* call counters */

int copy(const char *src, const char *dst)
{
    char buf[BUF];
    int in  = open(src, O_RDONLY);
    int out = open(dst, O_WRONLY | O_CREAT
                        | O_TRUNC, 0644);
    ssize_t n;
    if (in < 0 || out < 0)
        return -1;
    while ((n = read(in, buf, BUF)) > 0) {
        reads++;
        if (write(out, buf, n) != n)        /* exactly n bytes (not BUF!) */
            break;
        writes++;
    }
    reads++;                                /* the last read, which returns 0 */
    close(in);
    close(out);
    return 0;
}

int main(void)
{
    const char *abc = "/tmp/abc", *xyz = "/tmp/xyz";
    char data[10000];
    struct stat st;

    for (size_t i = 0; i < sizeof data; i++)
        data[i] = (char)('a' + i % 26);
    int fd = open(abc, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0 || write(fd, data, sizeof data) != (ssize_t)sizeof data) {
        perror(abc);
        return 1;
    }
    close(fd);

    if (copy(abc, xyz) != 0) {
        perror("copy");
        unlink(abc);
        return 1;
    }
    stat(xyz, &st);
    printf("copied = %lld bytes; %d read, %d write calls\n",
           (long long)st.st_size, reads, writes);

    char back[10000];
    fd = open(xyz, O_RDONLY);
    ssize_t n = read(fd, back, sizeof back);
    close(fd);
    printf("contents %s\n", n == (ssize_t)sizeof back &&
           memcmp(data, back, sizeof data) == 0 ? "identical" : "DIFFERENT");

    unlink(abc);
    unlink(xyz);
    return 0;
}

/* Expected output:
 * copied = 10000 bytes; 4 read, 3 write calls
 * contents identical
 */
