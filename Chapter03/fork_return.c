/* fork(): one call, two returns. It returns 0 in the child and the child's
 * PID in the parent; -1 on error. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid = fork();
    if (pid < 0)        { perror("fork"); exit(1); }
    else if (pid == 0)  { /* child: fork returned 0 */
        printf("child: fork returned 0, my PID is %d\n", (int)getpid());
        return 0;
    }
    else                { /* parent: pid = child's PID */
        waitpid(pid, NULL, 0);
        printf("parent: fork returned %d (the child's PID)\n", (int)pid);
    }
    return 0;
}

/* Expected output (PIDs vary; the two numbers are the same):
 * child: fork returned 0, my PID is 1235
 * parent: fork returned 1235 (the child's PID)
 */
