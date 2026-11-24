/* How the shell sets up the "ls | wc -l" pipeline: pipe, two forks, dup2 to
 * redirect stdout/stdin, execlp. If the shell does not close both ends, wc never
 * sees EOF. The example works in a temporary directory with 7 files in it.
 * Shell equivalent:  $ ls | wc -l */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

static char dir[] = "/tmp/pipe_XXXXXX";
#define NFILES 7

int main(void)
{
    char name[64];
    if (!mkdtemp(dir)) { perror("mkdtemp"); return 1; }
    for (int i = 0; i < NFILES; i++) {
        snprintf(name, sizeof name, "%s/file%d", dir, i);
        FILE *f = fopen(name, "w");
        if (f) fclose(f);
    }
    if (chdir(dir) != 0) { perror("chdir"); return 1; }

    int fd[2];
    if (pipe(fd) != 0) { perror("pipe"); return 1; }   /* 3: read, 4: write */
    printf("pipe: fd[0] = %d, fd[1] = %d\n", fd[0], fd[1]);
    fflush(stdout);                    /* so the children do not copy the buffer */

    if (fork() == 0) {    /* child 1 */
        dup2(fd[1], 1);
        close(fd[0]); close(fd[1]);
        execlp("ls", "ls", (char *)NULL);
        _exit(127);
    }
    if (fork() == 0) {    /* child 2 */
        dup2(fd[0], 0);
        close(fd[0]); close(fd[1]);
        execlp("wc", "wc", "-l", (char *)NULL);
        _exit(127);
    }
    /* shell: close both ends */
    close(fd[0]); close(fd[1]);
    wait(NULL); wait(NULL);

    /* cleanup */
    for (int i = 0; i < NFILES; i++) {
        snprintf(name, sizeof name, "%s/file%d", dir, i);
        unlink(name);
    }
    if (chdir("/") != 0)
        perror("chdir");
    rmdir(dir);
    return 0;
}

/* Expected output:
 * pipe: fd[0] = 3, fd[1] = 4
 * 7
 * (the fd numbers are 3 and 4 if no other descriptors are open through inheritance)
 */
