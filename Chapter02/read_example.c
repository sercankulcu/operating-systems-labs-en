/* The read() system call: reads from descriptor 0 (stdin) and writes the n
 * bytes it got back to stdout unchanged. A pipe connected to stdin replaces
 * the keyboard; the user typing "hello" and pressing Enter is simulated. */
#include <unistd.h>
#include <stdio.h>

/* Writes the "user" line into a pipe and makes the pipe descriptor 0. */
static int simulate_keyboard(const char *line, size_t length)
{
    int p[2];
    if (pipe(p) == -1)
        return -1;
    if (write(p[1], line, length) != (ssize_t)length)
        return -1;
    close(p[1]);
    if (dup2(p[0], 0) == -1)
        return -1;
    close(p[0]);
    return 0;
}

int main(void)
{
    if (simulate_keyboard("hello\n", 6) == -1) {
        perror("pipe");
        return 1;
    }

    char buf[100];
    ssize_t n = read(0, buf, sizeof(buf));
    if (n == -1) {
        perror("read error");
        return 1;
    }
    if (write(1, buf, n) != n)    // exactly n bytes
        return 1;
    printf("n=%zd\n", n);
    return 0;
}

/* Expected output:
 * hello
 * n=6
 */
