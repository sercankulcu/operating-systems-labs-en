/* pipe() + fork(): the parent writes "welcome" to the write end of the pipe,
 * the child reads from the read end. Unused ends are closed so that the
 * reader can see EOF. */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    int fd[2];
    char buf[32];
    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }
    pid_t pid = fork();
    if (pid == 0) {   /* child: reads */
        close(fd[1]);
        ssize_t n = read(fd[0], buf, sizeof buf - 1);
        if (n < 0)
            n = 0;
        buf[n] = '\0';
        printf("child got: %s\n", buf);
        close(fd[0]);
        return 0;
    }
    close(fd[0]);     /* parent: writes */
    if (write(fd[1], "welcome", 7) != 7)
        perror("write");
    close(fd[1]);
    wait(NULL);
    return 0;
}

/* Expected output:
 * child got: welcome
 */
