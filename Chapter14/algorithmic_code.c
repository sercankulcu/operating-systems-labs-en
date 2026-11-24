/* Execution paradigms (a) algorithmic code: the program determines the flow and
 * makes blocking read/write calls to the operating system.
 * fd is one end of a local socket pair; the other end plays the "outside world". */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

static int fd;          /* descriptor the program talks to */
static int outside;     /* the other end */

static void init(void)
{
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
        perror("socketpair");
        exit(1);
    }
    fd = sv[0];
    outside = sv[1];
    int incoming = 21;
    if (write(outside, &incoming, sizeof incoming) != (ssize_t)sizeof incoming)  /* data ready to read */
        perror("write");
    printf("init\n");
}

static void do_something(void)
{
    printf("do_something\n");
}

static void do_something_else(void)
{
    printf("do_something_else\n");
}

static void keep_going(void)
{
    int v;
    if (read(outside, &v, sizeof v) == (ssize_t)sizeof v)
        printf("keep_going: the other side received y = %d\n", v);
    close(fd);
    close(outside);
}

int main(void)
{
    int x, y;
    init();
    do_something();
    if (read(fd, &x, sizeof x) != (ssize_t)sizeof x)   /* blocks until data arrives */
        return 1;
    printf("read: x = %d\n", x);
    do_something_else();
    y = 2 * x;
    if (write(fd, &y, sizeof y) != (ssize_t)sizeof y)
        return 1;
    keep_going();
    exit(0);
}

/* Expected output:
 * init
 * do_something
 * read: x = 21
 * do_something_else
 * keep_going: the other side received y = 42
 */
