/* System call dispatch: the kernel's single entry point looks the call
 * number up in a table of function pointers. The kernel is simulated in
 * user space; the numbers match Linux x86-64 (0 read, 1 write, 2 open). */
#include <stdio.h>
#include <errno.h>

/* "Kernel" functions: they do no real work, they just print what was asked. */
static long sys_read(long fd, long buf, long n)
{
    (void)buf;
    printf("  [kernel] sys_read(fd=%ld, n=%ld)\n", fd, n);
    return 0;
}

static long sys_write(long fd, long buf, long n)
{
    printf("  [kernel] sys_write(fd=%ld, \"%s\", %ld)\n", fd, (const char *)buf, n);
    return n;                       /* bytes written */
}

static long sys_open(long path, long flags, long mode)
{
    (void)mode;
    printf("  [kernel] sys_open(\"%s\", %ld)\n", (const char *)path, flags);
    return 3;                       /* first free descriptor */
}

typedef long (*syscall_t)(long, long, long);
syscall_t table[3] = {sys_read, sys_write,
                      sys_open};

long syscall_entry(long no, long a,
                   long b, long c)
{
    if (no < 0 || no >= 3)
        return -ENOSYS;     /* bad number */
    return table[no](a, b, c);
}

/* libc wrapper: turns a negative return into errno and returns -1. */
static long libc_syscall(long no, long a, long b, long c)
{
    long r = syscall_entry(no, a, b, c);
    if (r < 0) {
        errno = (int)-r;
        return -1;
    }
    return r;
}

int main(void)
{
    long r;

    printf("write(1, \"abc\", 3), no=1\n");
    r = libc_syscall(1, 1, (long)"abc", 3);
    printf("  return=%ld\n", r);

    printf("open(\"notes.txt\", 0), no=2\n");
    r = libc_syscall(2, (long)"notes.txt", 0, 0);
    printf("  return=%ld\n", r);

    printf("syscall(99)\n");
    r = syscall_entry(99, 0, 0, 0);
    printf("  kernel return=%ld\n", r);
    r = libc_syscall(99, 0, 0, 0);
    printf("  return=%ld, errno=%d (ENOSYS=%d)\n", r, errno, ENOSYS);
    return 0;
}

/* Expected output:
 * write(1, "abc", 3), no=1
 *   [kernel] sys_write(fd=1, "abc", 3)
 *   return=3
 * open("notes.txt", 0), no=2
 *   [kernel] sys_open("notes.txt", 0)
 *   return=3
 * syscall(99)
 *   kernel return=-38
 *   return=-1, errno=38 (ENOSYS=38)
 */
