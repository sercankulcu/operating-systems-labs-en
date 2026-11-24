/* fork() and copy-on-write (COW): fork copies only the page tables.
 * When the child reads x nothing is copied; when it writes, the kernel copies page D.
 * The parent's x stays 1. The child also prints its minor-fault count for the write.
 * To see a program's fault counts from outside (commands from the slides):
 *   $ /usr/bin/time -v ./prog | grep -i fault
 *   $ ps -o min_flt,maj_flt -p PID
 *   $ perf stat -e page-faults,major-faults ./prog */
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int x = 1;                 /* data: D */

static long minor_faults(void)
{
    struct rusage r;
    getrusage(RUSAGE_SELF, &r);
    return r.ru_minflt;
}

int main(void)
{
    char *b = malloc(4096);  /* heap: H */
    b[0] = 'a';
    pid_t pid = fork();
    if (pid == 0) {          /* child */
        printf("%d\n", x);    /* read */
        long before = minor_faults();
        x = 2;                 /* write */
        long after = minor_faults();
        printf("child: x = %d, minor faults on write: %ld (COW copy)\n", x, after - before);
        fflush(stdout);        /* _exit does not flush the stdio buffer */
        _exit(0);
    }
    wait(NULL);
    b[0] = 'b';              /* parent */
    printf("parent: x = %d, b[0] = %c\n", x, b[0]);
    free(b);
    return 0;
}

/* Expected output (the fault count may vary with the kernel, usually 1):
 * 1
 * child: x = 2, minor faults on write: 1 (COW copy)
 * parent: x = 1, b[0] = b
 */
