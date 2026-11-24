/* Memory-mapped file: a 12 KiB (3-page) file is mapped with mmap; pages are
 * read in by faults on first access. p[5000] = 'x' dirties page 1, msync writes it
 * to the file. The file is created in /tmp and deleted at the end. */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define FILE_PATH "/tmp/data.bin"

static int create_file(void)
{
    char block[4096];
    int fd = open(FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return -1;
    memset(block, 'a', sizeof block);
    for (int i = 0; i < 3; i++)
        if (write(fd, block, sizeof block) != (ssize_t)sizeof block) {
            close(fd);
            return -1;
        }
    close(fd);
    return 0;
}

int main(void)
{
    if (create_file() != 0) { perror("create_file"); return 1; }

    int fd = open(FILE_PATH, O_RDWR);
    struct stat st;
    fstat(fd, &st);          /* 12 KiB = 3 pages */
    char *p = mmap(NULL, st.st_size,
         PROT_READ | PROT_WRITE,
         MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) { perror("mmap"); close(fd); unlink(FILE_PATH); return 1; }
    close(fd);               /* mapping stays */
    char c = p[0];           /* page 0 */
    p[5000] = 'x';           /* page 1 */
    msync(p, st.st_size, MS_SYNC);
    munmap(p, st.st_size);

    printf("size = %lld bytes (%lld pages)\n", (long long)st.st_size,
           (long long)st.st_size / 4096);
    printf("c = p[0] = '%c'\n", c);

    /* Is the change in the file? Check with read */
    char b = 0;
    fd = open(FILE_PATH, O_RDONLY);
    if (pread(fd, &b, 1, 5000) == 1)
        printf("byte 5000 in the file = '%c' (page %d)\n", b, 5000 / 4096);
    close(fd);
    unlink(FILE_PATH);
    return 0;
}

/* Expected output:
 * size = 12288 bytes (3 pages)
 * c = p[0] = 'a'
 * byte 5000 in the file = 'x' (page 1)
 */
