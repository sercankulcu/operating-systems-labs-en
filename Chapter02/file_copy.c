/* File copy with read/write: a 10,000-byte "source" file is copied to
 * "destination" with a 4096-byte buffer (4096 + 4096 + 1808). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define SRC "/tmp/os02_source"
#define DST "/tmp/os02_destination"

/* Prepares the 10,000-byte sample source file. */
static int prepare_source(void)
{
    char block[1000];
    memset(block, 'x', sizeof block);
    int fd = creat(SRC, 0644);
    if (fd == -1)
        return -1;
    for (int i = 0; i < 10; i++) {
        if (write(fd, block, sizeof block) != (ssize_t)sizeof block) {
            close(fd);
            return -1;
        }
    }
    close(fd);
    return 0;
}

int main(void)
{
    if (prepare_source() == -1) {
        perror("source");
        return 1;
    }

    char buf[4096];
    int in = open(SRC, O_RDONLY);
    int out = creat(DST, 0644);
    if (in == -1 || out == -1) {
        perror("open");
        unlink(SRC);
        return 1;
    }
    printf("in=%d out=%d\n", in, out);
    ssize_t n;  long total = 0;
    while ((n = read(in, buf, 4096)) > 0) {
        if (write(out, buf, n) != n) {
            perror("write");
            break;
        }
        total += n;
        printf("n=%zd total=%ld\n", n, total);
    }
    close(in);  close(out);
    printf("%ld bytes\n", total);

    unlink(SRC);
    unlink(DST);
    return 0;
}

/* Expected output (stdin/stdout/stderr are open, so the first free fd is 3):
 * in=3 out=4
 * n=4096 total=4096
 * n=4096 total=8192
 * n=1808 total=10000
 * 10000 bytes
 */
