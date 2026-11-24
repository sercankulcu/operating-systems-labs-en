/* fork + exec + waitpid: the child replaces itself with the "ls -l" program
 * through execlp, the parent waits with waitpid and prints the exit status. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }
    if (pid == 0) {
        execlp("ls", "ls", "-l", NULL);
        perror("execlp");  /* error */
        exit(1);
    }
    int st;
    if (waitpid(pid, &st, 0) < 0) { perror("waitpid"); exit(1); }
    if (WIFEXITED(st)) printf("done: %d\n", WEXITSTATUS(st));
    return 0;
}

/* Expected output (the ls -l output depends on the working directory; in an empty directory):
 * total 0
 * done: 0
 */
