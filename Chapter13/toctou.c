/* TOCTOU race: the name checked with access() can be turned into a symbolic
 * link to another file before open() uses it. The attacker's interference is
 * simulated deterministically; instead of /etc/passwd the target is a "secret"
 * file under /tmp. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

static char dir[] = "/tmp/toctou_XXXXXX";
static char path[64], secret[64];

static void write_file(const char *name, const char *contents)
{
    int fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror(name); return; }
    if (write(fd, contents, strlen(contents)) < 0)
        perror("write");
    close(fd);
}

static void show_secret(void)
{
    char b[64];
    int fd = open(secret, O_RDONLY);
    ssize_t n = fd < 0 ? -1 : read(fd, b, sizeof b - 1);
    if (fd >= 0)
        close(fd);
    if (n < 0) n = 0;
    b[n] = '\0';
    printf("  contents of the secret file: \"%s\"\n", b);
}

static void attacker(void)
{
    /* the name is swapped in the window between check and use */
    unlink(path);
    if (symlink(secret, path) != 0)
        perror("symlink");
}

int main(void)
{
    const char *data = "ATTACKERS";
    size_t n = strlen(data);
    int fd;

    if (!mkdtemp(dir)) { perror("mkdtemp"); return 1; }
    snprintf(path, sizeof path, "%s/f", dir);
    snprintf(secret, sizeof secret, "%s/secret", dir);

    /* --- vulnerable: check and use are separate --- */
    write_file(secret, "root:x:0:0");
    write_file(path, "");
    printf("vulnerable version:\n");
    /* TOCTOU: a setuid-root helper */
    if (access(path, W_OK) == 0) {   /* CHECK */
        printf("  access: writable\n");
        attacker();
        fd = open(path, O_WRONLY);     /* USE */
        if (write(fd, data, n) < 0)
            perror("write");
        close(fd);
    }
    show_secret();

    /* --- fix: the name is resolved once, symbolic links are not followed --- */
    write_file(secret, "root:x:0:0");
    unlink(path);
    write_file(path, "");
    printf("fixed version:\n");
    if (access(path, W_OK) == 0) {
        printf("  access: writable\n");
        attacker();
        fd = open(path, O_WRONLY | O_NOFOLLOW);
        if (fd < 0) {
            printf("  open(O_NOFOLLOW): %s\n", errno == ELOOP ? "ELOOP, rejected" : strerror(errno));
        } else {
            if (write(fd, data, n) < 0)
                perror("write");
            close(fd);
        }
    }
    show_secret();

    unlink(path);
    unlink(secret);
    rmdir(dir);
    return 0;
}

/* Expected output:
 * vulnerable version:
 *   access: writable
 *   contents of the secret file: "ATTACKERS0"
 * fixed version:
 *   access: writable
 *   open(O_NOFOLLOW): ELOOP, rejected
 *   contents of the secret file: "root:x:0:0"
 * ("root:x:0:0" is 10 bytes; writing the 9-byte "ATTACKERS" over it leaves the last "0")
 */
