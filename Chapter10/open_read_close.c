/* open, read, close step by step: a 6000-byte file is read with a 4096-byte buffer.
 * The first read returns 4096, the second a short read of 1904, the third 0 (end of file).
 * notes.txt is first created in /tmp and deleted at the end. */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define NOTES "/tmp/notes.txt"

static int prepare(void)
{
    char buffer[6000];
    int fd = open(NOTES, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return -1;
    memset(buffer, 'n', sizeof buffer);
    ssize_t w = write(fd, buffer, sizeof buffer);
    close(fd);
    return w == (ssize_t)sizeof buffer ? 0 : -1;
}

int main(void)
{
    if (prepare() != 0) { perror("prepare"); return 1; }

    char buf[4096];
    int fd = open(NOTES, O_RDONLY);
    printf("fd = %d\n", fd);
    ssize_t n = read(fd, buf, sizeof buf);
    printf("n = %zd, position = %lld\n", n, (long long)lseek(fd, 0, SEEK_CUR));
    n = read(fd, buf, sizeof buf);
    printf("n = %zd, position = %lld\n", n, (long long)lseek(fd, 0, SEEK_CUR));
    n = read(fd, buf, sizeof buf);
    printf("n = %zd (end of file)\n", n);
    close(fd);

    unlink(NOTES);
    return 0;
}

/* Expected output (if 0, 1, 2 are open, the lowest free number is 3):
 * fd = 3
 * n = 4096, position = 4096
 * n = 1904, position = 6000
 * n = 0 (end of file)
 */
