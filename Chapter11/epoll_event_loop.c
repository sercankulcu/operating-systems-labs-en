/* Event loop: with epoll a single thread watches several non-blocking descriptors;
 * only the ready ones are handled. Local socket pairs (socketpair) are used
 * instead of real sockets; the infinite loop runs for a limited number of rounds. */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#define NSOCK 3
#define ROUNDS 3

static int s[NSOCK];        /* the ends the server watches */
static int peer[NSOCK];     /* client ends: data arrives from here */

static int index_of(int fd)
{
    for (int k = 0; k < NSOCK; k++)
        if (s[k] == fd)
            return k;
    return -1;
}

static void handle(int fd, char *buf, ssize_t r)
{
    printf("  s[%d]: read %zd bytes, first byte '%c'\n", index_of(fd), r, buf[0]);
}

int main(void)
{
    struct epoll_event ev, evs[64];
    char buf[4096];
    int k, nsock = NSOCK;

    for (k = 0; k < nsock; k++) {
        int pair[2];
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) < 0) {
            perror("socketpair");
            return 1;
        }
        s[k] = pair[0];
        peer[k] = pair[1];
        fcntl(s[k], F_SETFL, fcntl(s[k], F_GETFL) | O_NONBLOCK);
    }

    int ep = epoll_create1(0);
    for (k = 0; k < nsock; k++) {   /* O_NONBLOCK */
        memset(&ev, 0, sizeof ev);
        ev.events = EPOLLIN;
        ev.data.fd = s[k];
        epoll_ctl(ep, EPOLL_CTL_ADD, s[k], &ev);
    }

    for (int round = 0; round < ROUNDS; round++) {   /* ROUNDS rounds instead of for (;;) */
        /* simulation: clients send data */
        if (round == 0) {
            memset(buf, 'x', 512);
            if (write(peer[2], buf, 512) != 512)    /* 512 bytes to s[2] */
                perror("write");
        } else if (round == 1) {
            memset(buf, 'y', 100);
            if (write(peer[0], buf, 100) != 100)    /* 100 bytes to s[0] */
                perror("write");
        }

        int n = epoll_wait(ep, evs, 64, 200); /* 200 ms instead of -1 */
        printf("round %d: epoll_wait -> n = %d\n", round, n);
        for (int i = 0; i < n; i++) {
            int fd = evs[i].data.fd;
            ssize_t r = read(fd, buf, sizeof buf);
            if (r < 0 && errno == EAGAIN) continue;
            handle(fd, buf, r);
            /* second read: no data left, a non-blocking read returns EAGAIN */
            r = read(fd, buf, sizeof buf);
            if (r < 0 && errno == EAGAIN)
                printf("  s[%d]: EAGAIN, no more data to wait for\n", index_of(fd));
        }
    }

    close(ep);
    for (k = 0; k < nsock; k++) {
        close(s[k]);
        close(peer[k]);
    }
    return 0;
}

/* Expected output:
 * round 0: epoll_wait -> n = 1
 *   s[2]: read 512 bytes, first byte 'x'
 *   s[2]: EAGAIN, no more data to wait for
 * round 1: epoll_wait -> n = 1
 *   s[0]: read 100 bytes, first byte 'y'
 *   s[0]: EAGAIN, no more data to wait for
 * round 2: epoll_wait -> n = 0
 * (no ready socket in round 2: 200 ms timeout)
 */
