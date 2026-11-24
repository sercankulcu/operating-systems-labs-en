/* Stack canary: the compiler places a random value whose first byte is 0x00
 * between buf and the saved rbp and checks it before returning. The frame is
 * simulated with a byte array: [0..7] buf, [8..15] canary, [16..23] rbp,
 * [24..31] return address. The overflowing call receives SIGABRT in a child process. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>

/* The real __stack_chk_guard/__stack_chk_fail belong to glibc; in the simulation
 * the same names are redirected to our own variable and function. */
#define __stack_chk_guard sim_stack_chk_guard
#define __stack_chk_fail  sim_stack_chk_fail

static long __stack_chk_guard;              /* random at process start */
static unsigned char frame[32];

static void __stack_chk_fail(void)
{
    printf("*** stack smashing detected ***: terminated\n");
    fflush(stdout);
    abort();                                 /* SIGABRT */
}

/* the compiler adds the canary lines */
void copy_in(const char *input)
{
    long canary = __stack_chk_guard;
    char *buf = (char *)frame;               /* char buf[8]; */
    memcpy(frame + 8, &canary, 8);           /* prologue: place the canary */

    strcpy(buf, input);

    memcpy(&canary, frame + 8, 8);           /* epilogue: read it from the stack */
    printf("copy_in(\"%s\"): canary = 0x%016lx\n", input, (unsigned long)canary);
    fflush(stdout);
    if (canary != __stack_chk_guard)
        __stack_chk_fail();
}

int main(void)
{
    srand(42);
    __stack_chk_guard = ((long)rand() << 32 | (long)rand()) & ~0xFFL;   /* first byte 0x00 */

    copy_in("AAAA");                          /* fits: canary intact */
    printf("returned\n");
    fflush(stdout);

    pid_t pid = fork();
    if (pid == 0) {
        struct rlimit none = { 0, 0 };
        setrlimit(RLIMIT_CORE, &none);        /* do not write a core dump */
        /* input: "AAAAAAAABBBBBBBB" */
        copy_in("AAAAAAAABBBBBBBB");
        printf("returned\n");                 /* never reached */
        _exit(0);
    }
    int st;
    waitpid(pid, &st, 0);
    if (WIFSIGNALED(st))
        printf("child terminated by signal %d (%s)\n", WTERMSIG(st),
               WTERMSIG(st) == SIGABRT ? "SIGABRT" : "?");
    return 0;
}

/* Expected output (the first canary value depends on srand(42) and libc's rand;
 * its last byte is always 00):
 * copy_in("AAAA"): canary = 0x........xxxxxx00
 * returned
 * copy_in("AAAAAAAABBBBBBBB"): canary = 0x4242424242424242
 * *** stack smashing detected ***: terminated
 * child terminated by signal 6 (SIGABRT)
 */
