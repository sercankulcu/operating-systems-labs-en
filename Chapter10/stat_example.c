/* The stat structure and ls -l: a file's i-node information is read with stat()
 * and printed in ls -l format. struct stat is defined in <sys/stat.h>:
 *   dev_t st_dev; ino_t st_ino; mode_t st_mode; nlink_t st_nlink; uid_t st_uid;
 *   gid_t st_gid; dev_t st_rdev; off_t st_size; blksize_t st_blksize;
 *   blkcnt_t st_blocks; struct timespec st_atim, st_mtim, st_ctim; */
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define FILE_PATH "/tmp/stat_example.txt"

static char type_char(mode_t m)
{
    if (S_ISREG(m)) return '-';
    if (S_ISDIR(m)) return 'd';
    if (S_ISLNK(m)) return 'l';
    if (S_ISCHR(m)) return 'c';
    if (S_ISBLK(m)) return 'b';
    if (S_ISFIFO(m)) return 'p';
    if (S_ISSOCK(m)) return 's';
    return '?';
}

static void ls_l(const char *path)
{
    struct stat st;
    char perm[11];
    char when[32];
    const char *rwx = "rwxrwxrwx";

    if (stat(path, &st) != 0) {
        perror(path);
        return;
    }
    perm[0] = type_char(st.st_mode);
    for (int i = 0; i < 9; i++)
        perm[i + 1] = (st.st_mode & (0400 >> i)) ? rwx[i] : '-';
    if (st.st_mode & S_ISVTX)               /* sticky bit: /tmp */
        perm[9] = (st.st_mode & S_IXOTH) ? 't' : 'T';
    perm[10] = '\0';
    strftime(when, sizeof when, "%Y-%m-%d %H:%M", localtime(&st.st_mtim.tv_sec));

    printf("%s %lu %u %u %lld %s %s\n", perm, (unsigned long)st.st_nlink,
           (unsigned)st.st_uid, (unsigned)st.st_gid, (long long)st.st_size,
           when, path);
    printf("  st_ino = %lu, st_blksize = %ld, st_blocks = %lld (512 B units)\n",
           (unsigned long)st.st_ino, (long)st.st_blksize, (long long)st.st_blocks);
}

int main(void)
{
    const char text[] = "operating systems.\n";
    int fd = open(FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd < 0) { perror("open"); return 1; }
    if (write(fd, text, strlen(text)) < 0)
        perror("write");
    fchmod(fd, 0644);                       /* independent of umask */
    close(fd);

    ls_l(FILE_PATH);
    ls_l("/tmp");
    unlink(FILE_PATH);
    return 0;
}

/* Expected output (i-node, uid/gid, time and block counts vary by system):
 * -rw-r--r-- 1 1001 1001 19 2026-11-06 12:00 /tmp/stat_example.txt
 *   st_ino = 1234567, st_blksize = 4096, st_blocks = 8 (512 B units)
 * drwxrwxrwt 12 0 0 4096 2026-11-06 12:00 /tmp
 *   st_ino = 2, st_blksize = 4096, st_blocks = 8 (512 B units)
 */
