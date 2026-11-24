/* Why are fork and exec separate? Between fork and exec the child closes
 * stdout and opens a file; open returns the lowest free number (1), and the
 * exec'd wc writes into the file without knowing. This is how the shell runs
 * "wc -l notes.txt > count.txt". The files are created under /tmp instead of
 * the working directory and removed at the end. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NOTES "/tmp/os03_notes.txt"
#define COUNT "/tmp/os03_count.txt"

/* 3-line sample notes.txt */
static int prepare_notes(void)
{
    const char *text = "line 1\nline 2\nline 3\n";
    int fd = open(NOTES, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
        return -1;
    ssize_t n = write(fd, text, strlen(text));
    close(fd);
    return n == (ssize_t)strlen(text) ? 0 : -1;
}

int main(void)
{
    if (prepare_notes() == -1) {
        perror("notes");
        return 1;
    }

    pid_t pid = fork();
    if (pid == 0) {    /* child */
        close(1);        /* stdout freed */
        int fd = open(COUNT,
                 O_WRONLY | O_CREAT | O_TRUNC, 0644);
        /* lowest free number: fd == 1 */
        if (fd != 1)
            _exit(2);
        execlp("wc", "wc", "-l", NOTES, NULL);
        perror("execlp");
        exit(1);
    }
    waitpid(pid, NULL, 0);
    printf("done\n");

    /* Shows what was written to the file. */
    char buf[128];
    int fd = open(COUNT, O_RDONLY);
    if (fd != -1) {
        ssize_t n = read(fd, buf, sizeof buf - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("count.txt: %s", buf);
        }
        close(fd);
    }
    unlink(COUNT);
    unlink(NOTES);
    return 0;
}

/* Expected output (only "done" would appear on screen; the second line is the file's content):
 * done
 * count.txt: 3 /tmp/os03_notes.txt
 */
