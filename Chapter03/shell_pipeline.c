/* The shell pipeline "ls | wc -l": two children share the same pipe; dup2
 * connects the first one's stdout to the write end and the second one's
 * stdin to the read end. The programs never notice the pipe. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }
    if (fork() == 0) { if (dup2(fd[1], 1) == -1) _exit(1);
                       close(fd[0]); close(fd[1]);
                       execlp("ls", "ls", NULL);
                       _exit(127); }
    if (fork() == 0) { if (dup2(fd[0], 0) == -1) _exit(1);
                       close(fd[0]); close(fd[1]);
                       execlp("wc", "wc", "-l", NULL);
                       _exit(127); }
    close(fd[0]); close(fd[1]); wait(NULL); wait(NULL);
    return 0;
}

/* Expected output: the number of entries in the working directory. In an empty directory:
 * 0
 */
