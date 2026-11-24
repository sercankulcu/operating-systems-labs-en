/* POSIX shared memory: the producer creates the "/os03" object and writes
 * into it, the consumer (another process created with fork) maps the same
 * object and reads it. Instead of synchronization, the consumer is started
 * after the producer has written. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>

static int producer(void)
{
    int fd = shm_open("/os03", O_CREAT|O_RDWR, 0600);
    if (fd == -1) { perror("shm_open"); return -1; }
    if (ftruncate(fd, 4096) == -1) { perror("ftruncate"); close(fd); return -1; }
    char *p = mmap(NULL, 4096, PROT_READ|PROT_WRITE,
                   MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) { perror("mmap"); close(fd); return -1; }
    strcpy(p, "hello");     /* direct write */
    printf("producer: wrote\n");
    munmap(p, 4096);
    close(fd);
    return 0;
}

static int consumer(void)
{
    int fd = shm_open("/os03", O_RDONLY, 0);
    if (fd == -1) { perror("shm_open"); return -1; }
    char *p = mmap(NULL, 4096, PROT_READ,
                   MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) { perror("mmap"); close(fd); return -1; }
    printf("%s\n", p);
    munmap(p, 4096);
    close(fd);
    shm_unlink("/os03");    /* remove */
    return 0;
}

int main(void)
{
    if (producer() == -1) {
        shm_unlink("/os03");
        return 1;
    }
    fflush(stdout);

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        shm_unlink("/os03");
        return 1;
    }
    if (pid == 0)               /* consumer (another process) */
        exit(consumer() == 0 ? 0 : 1);

    int st;
    waitpid(pid, &st, 0);
    return WIFEXITED(st) ? WEXITSTATUS(st) : 1;
}

/* Expected output:
 * producer: wrote
 * hello
 */
