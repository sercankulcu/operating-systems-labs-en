/* lockdep lock-order graph: the relation "L acquired while H is held" accumulates
 * as an edge H -> L. If a new edge closes a cycle (there is a path from L to H)
 * a possible-deadlock warning is issued, even if the tasks never run at the
 * same time. Tasks and lock classes are simulated in user space. */
#include <stdio.h>
#include <string.h>

#define NLOCKS 3
static const char *names[NLOCKS] = { "A", "B", "C" };
enum { A, B, C };

typedef struct {
    const char *name;
    int held[8];
    int n;
} task;

static int edge[NLOCKS][NLOCKS];      /* edge[H][L]: H before L */
static int prev[NLOCKS];              /* to print the path found */

/* is there a path from L to H? (depth-first search) */
static int path_search(int u, int H, int *seen)
{
    if (u == H)
        return 1;
    seen[u] = 1;
    for (int v = 0; v < NLOCKS; v++)
        if (edge[u][v] && !seen[v]) {
            prev[v] = u;
            if (path_search(v, H, seen))
                return 1;
        }
    return 0;
}

static int path_exists(int L, int H)
{
    int seen[NLOCKS] = { 0 };
    return path_search(L, H, seen);
}

static void add_edge(int H, int L)
{
    edge[H][L] = 1;
}

static void warn_cycle(const char *msg, int H, int L)
{
    int path[NLOCKS + 1], n = 0;
    for (int u = H; u != L; u = prev[u])
        path[n++] = u;
    path[n++] = L;
    printf("WARNING: possible circular locking dependency (%s)\n", msg);
    printf("  %s -> %s, but ", names[H], names[L]);
    for (int i = n - 1; i >= 0; i--)
        printf("%s%s", names[path[i]], i ? " -> " : "\n");
}

void lock_acquire(task *t, int L)
{
    printf("%s: acquires %s\n", t->name, names[L]);
    for (int i = 0; i < t->n; i++) {
        int H = t->held[i];
        if (path_exists(L, H))   /* L -> ... -> H */
            warn_cycle("possible deadlock", H, L);
        add_edge(H, L);          /* H before L */
    }
    t->held[t->n++] = L;
}

void lock_release(task *t) { t->n--; }

int main(void)
{
    /* T1: A,B   T2: B,C   T3: C,A */
    task T1 = { "T1", { 0 }, 0 }, T2 = { "T2", { 0 }, 0 }, T3 = { "T3", { 0 }, 0 };

    lock_acquire(&T1, A); lock_acquire(&T1, B);
    lock_release(&T1);    lock_release(&T1);

    lock_acquire(&T2, B); lock_acquire(&T2, C);
    lock_release(&T2);    lock_release(&T2);

    lock_acquire(&T3, C); lock_acquire(&T3, A);
    lock_release(&T3);    lock_release(&T3);
    return 0;
}

/* Expected output:
 * T1: acquires A
 * T1: acquires B
 * T2: acquires B
 * T2: acquires C
 * T3: acquires C
 * T3: acquires A
 * WARNING: possible circular locking dependency (possible deadlock)
 *   C -> A, but A -> B -> C
 */
