/* Without wait() the output order is undetermined: after fork, the scheduler
 * decides which process runs first. The parent still waits for the child
 * after printing (so no zombie is left); the order is not affected by this. */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid = fork();
    if (pid == 0) printf("child\n");
    else          printf("parent\n");

    if (pid > 0)
        wait(NULL);         /* AFTER printing: does not affect the order */
    return 0;
}

/* Expected output (the order of the two lines may change from run to run):
 * parent
 * child
 */
