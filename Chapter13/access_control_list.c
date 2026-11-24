/* Access control list (ACL): the ACL of file A is column A of the protection
 * matrix. The decision is made at the first matching entry; for a group not in
 * the list the safe default is deny. Rights R=4 W=2 X=1. */
#include <stdio.h>

enum { G1, G2, G3, G4, ADM };
#define R 4
#define W 2
#define X 1

/* ACL of file A: R=4 W=2 X=1 */
struct entry { int group, rights; } acl[] =
    {{G1, 4}, {G2, 2}, {ADM, 7}};

int access(int group, int req)
{
    for (int i = 0; i < 3; i++)
        if (acl[i].group == group) {
            int h = acl[i].rights;
            return (h & req) == req;
        }
    return 0;   /* not listed: denied */
}

static void try_access(const char *gname, int group, const char *rname, int req)
{
    printf("%s %s: %s\n", gname, rname, access(group, req) ? "allow" : "deny");
}

int main(void)
{
    try_access("G2", G2, "W", W);
    try_access("G1", G1, "W", W);
    try_access("G4", G4, "R", R);
    try_access("ADM", ADM, "RX", R | X);
    return 0;
}

/* Expected output:
 * G2 W: allow
 * G1 W: deny
 * G4 R: deny
 * ADM RX: allow
 */
