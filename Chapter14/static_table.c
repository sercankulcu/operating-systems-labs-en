/* Static and dynamic structures: linear search with a pointer in a fixed-size
 * process table. p++ advances to the next structure; empty entries (pid 0) are
 * scanned too. In the worst case the whole table: O(n). */
#include <stdio.h>

#define PROC_TABLE_SIZE 6

struct proc {
    int proc_pid;            /* 0: empty entry */
};

struct proc proc_table[PROC_TABLE_SIZE] = {
    { 1 }, { 314 }, { 0 }, { 42 }, { 977 }, { 0 }
};

static int search(int pid)
{
    struct proc *p;
    int found, scanned = 0;

    found = 0;
    for (p = &proc_table[0];
         p < &proc_table[PROC_TABLE_SIZE];
         p++) {
        scanned++;
        if (p->proc_pid == pid) {
            found = 1;
            break;
        }
    }
    printf("pid %d: found = %d, %d entries scanned", pid, found, scanned);
    if (found)
        printf(", proc_table[%td]", p - proc_table);
    printf("\n");
    return found;
}

int main(void)
{
    search(42);
    search(7);               /* not there: the whole table is scanned */
    return 0;
}

/* Expected output:
 * pid 42: found = 1, 4 entries scanned, proc_table[3]
 * pid 7: found = 0, 6 entries scanned
 */
