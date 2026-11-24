/* A simple shell: print a prompt, read the command, create a child with fork,
 * run the program in the child with execve, wait in the parent with waitpid.
 * Commands come from a fixed script instead of the keyboard; the loop ends
 * when the script runs out. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define TRUE 1

static char dir[] = "/tmp/os02_shellXXXXXX";
static char path1[128], path2[128];

/* Script standing in for the commands a user would type. */
static const char *script[][3] = {
    {"/bin/ls", "ls", dir},
    {"/bin/echo", "echo", "hello"},
};
static int next_cmd = 0;

static void type_prompt(void)
{
    printf("$ ");
}

/* Reads the next command; returns 0 when no commands are left. */
static int read_command(char *command, char *parameters[])
{
    if (next_cmd >= (int)(sizeof script / sizeof script[0])) {
        printf("(end of input)\n");
        return 0;
    }
    strcpy(command, script[next_cmd][0]);
    parameters[0] = (char *)script[next_cmd][1];
    parameters[1] = (char *)script[next_cmd][2];
    parameters[2] = NULL;
    printf("%s %s\n", parameters[0], parameters[1]);
    next_cmd++;
    return 1;
}

/* Creates the two empty files that ls will list. */
static int prepare_sample_dir(void)
{
    if (mkdtemp(dir) == NULL)
        return -1;
    snprintf(path1, sizeof path1, "%s/notes.txt", dir);
    snprintf(path2, sizeof path2, "%s/homework.c", dir);
    int a = open(path1, O_WRONLY | O_CREAT, 0644);
    int b = open(path2, O_WRONLY | O_CREAT, 0644);
    if (a == -1 || b == -1)
        return -1;
    close(a);
    close(b);
    return 0;
}

int main(void)
{
    char command[256];
    char *parameters[3];
    int status;

    if (prepare_sample_dir() == -1) {
        perror("dir");
        return 1;
    }

    while (TRUE) {              /* forever (until the input ends) */
        type_prompt();          /* print the prompt */
        if (!read_command(command, parameters))
            break;
        fflush(stdout);         /* keep child and parent output apart */

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            break;
        }
        if (pid != 0) {         /* parent */
            waitpid(-1, &status, 0);
            printf("[child %s finished, exit status %d]\n",
                   parameters[0], WEXITSTATUS(status));
        } else {                /* child process */
            execve(command, parameters, 0);
            perror("execve");   /* only if execve failed */
            _exit(127);
        }
    }

    unlink(path1);
    unlink(path2);
    rmdir(dir);
    return 0;
}

/* Expected output (when ls output does not go to a terminal, each name is on its own line):
 * $ ls /tmp/os02_shellXXXXXX     (XXXXXX differs on every run)
 * homework.c
 * notes.txt
 * [child ls finished, exit status 0]
 * $ echo hello
 * hello
 * [child echo finished, exit status 0]
 * $ (end of input)
 */
