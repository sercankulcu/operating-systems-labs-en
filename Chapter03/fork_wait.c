/* fork() and wait(): the child changes its own copy of x, the parent's stays
 * 10. Thanks to wait(), the output order is the same on every run. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    int x = 10;
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }
    if (pid == 0) {
        x = x + 5;
        printf("child: x=%d\n", x);
        return 0;
    }
    wait(NULL);
    printf("parent: x=%d\n", x);
    return 0;
}

/* Expected output:
 * child: x=15
 * parent: x=10
 */
