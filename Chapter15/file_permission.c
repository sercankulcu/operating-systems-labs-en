/* File permission check: for root (euid 0) the permission bits are not consulted
 * for read/write; for everyone else the first matching class (owner, group, other)
 * is selected and only that class's bits are checked. The cred and inode
 * structures are simulated. */
#include <stdio.h>

typedef struct {
    int euid;
    int gid[4];              /* groups the user belongs to */
    int ngid;
} cred;

typedef struct {
    int mode;                /* rwxrwxrwx bits */
    int uid, gid;
} inode;

enum { ALI = 1000, VELI = 1001, CAN = 1002 };
enum { TEAM = 100, OTHER_GROUP = 200 };

static int in_group(cred *c, int gid)
{
    for (int i = 0; i < c->ngid; i++)
        if (c->gid[i] == gid)
            return 1;
    return 0;
}

int has_perm(cred *c, inode *f, int req)
{
    int bit, m = f->mode;       /* r4 w2 x1 */
    if (c->euid == 0)           /* root */
        return !(req & 1) || (m & 0111);
    if (c->euid == f->uid)
        bit = (m >> 6) & 7;     /* owner */
    else if (in_group(c, f->gid))
        bit = (m >> 3) & 7;     /* group */
    else
        bit = m & 7;            /* other */
    return (bit & req) == req;
}

static void try_access(const char *prefix, const char *user, cred *c, inode *f,
                       const char *op, int req)
{
    printf("%s%s %s: %s\n", prefix, user, op, has_perm(c, f, req) ? "allow" : "deny");
}

int main(void)
{
    cred ali  = { ALI,  { TEAM }, 1 };
    cred veli = { VELI, { TEAM }, 1 };
    cred can  = { CAN,  { OTHER_GROUP }, 1 };
    cred root = { 0,    { 0 }, 1 };
    inode report = { 0640, ALI, TEAM };     /* report.txt: rw-r----- ali team */

    try_access("", "ali", &ali, &report, "w", 2);
    try_access("", "veli", &veli, &report, "w", 2);
    try_access("", "veli", &veli, &report, "r", 4);
    try_access("", "can", &can, &report, "r", 4);
    try_access("", "root", &root, &report, "r", 4);

    report.mode = 0066;                     /* ----rw-rw- */
    try_access("0066 ", "ali", &ali, &report, "r", 4);
    try_access("0066 ", "veli", &veli, &report, "r", 4);
    return 0;
}

/* Expected output:
 * ali w: allow
 * veli w: deny
 * veli r: allow
 * can r: deny
 * root r: allow
 * 0066 ali r: deny
 * 0066 veli r: allow
 */
