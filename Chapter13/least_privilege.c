/* Least privilege and setuid: a setuid-root program such as passwd uses its
 * privilege only for the work that needs it, then drops all three of the real,
 * effective and saved IDs to the user with setresuid; there is no way back.
 * Since the program is not installed setuid root, the first file it opens is a
 * "shadow" file simulated under /tmp; the last open tries the real /etc/shadow. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

static char shadow_path[] = "/tmp/shadow_XXXXXX";   /* simulates /etc/shadow */

static void update_own_entry(int fd, uid_t me)
{
    char line[64];
    int n = snprintf(line, sizeof line, "uid %u: new password hash\n", (unsigned)me);
    if (write(fd, line, (size_t)n) != n)
        perror("write");
}

static void print_ids(const char *when)
{
    uid_t r, e, s;
    if (getresuid(&r, &e, &s) != 0) {
        perror("getresuid");
        return;
    }
    printf("%-22s ruid=%u euid=%u suid=%u\n", when, (unsigned)r, (unsigned)e, (unsigned)s);
}

int main(void)
{
    /* /usr/bin/passwd  -rwsr-xr-x root */
    int t = mkstemp(shadow_path);
    if (t < 0) { perror("mkstemp"); return 1; }
    close(t);

    uid_t me = getuid();
    print_ids("start:");
    int fd = open(shadow_path, O_RDWR);           /* really /etc/shadow */
    update_own_entry(fd, me);
    close(fd);
    if (setresuid(me, me, me) != 0)
        perror("setresuid");
    print_ids("after setresuid:");
    /* ... rest of the work ... */
    fd = open("/etc/shadow", O_RDWR);
    if (fd < 0) perror("open");
    else close(fd);                                /* nothing is written */

    unlink(shadow_path);
    return 0;
}

/* Expected output (non-root user, for example uid 1000):
 * start:                 ruid=1000 euid=1000 suid=1000
 * after setresuid:       ruid=1000 euid=1000 suid=1000
 * open: Permission denied
 * The last line is written to stderr by perror. In a real setuid-root install
 * (chown root, chmod u+s) the start line shows euid=0 and suid=0; after
 * setresuid all three drop to 1000. The uid value varies by machine; when run as
 * root the open succeeds and the last line is not printed. */
