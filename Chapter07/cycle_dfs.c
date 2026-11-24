/* Cycle search in the wait-for graph: colored DFS (white/gray/black).
 * An edge to a gray node is a back edge: a cycle, i.e. a deadlock exists.
 * Graph: P1 -> P2, P2 -> P3 and P4, P4 -> P5, P5 -> P2. */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define N 5

enum { WHITE, GRAY, BLACK };
int color[N];         /* all WHITE */
bool waits[N][N];     /* waits[u][v]: Pu is waiting for the resource Pv holds */

bool cycle(int u)
{
    color[u] = GRAY;    /* on the path */
    for (int v = 0; v < N; v++) {
        if (!waits[u][v]) continue;
        if (color[v] == GRAY)
            return true;    /* back edge */
        if (color[v] == WHITE && cycle(v))
            return true;
    }
    color[u] = BLACK;   /* done */
    return false;
}

static void edge(int u, int v)           /* P numbers start at 1 */
{
    waits[u - 1][v - 1] = true;
}

static void print_colors(void)
{
    static const char letter[] = "WGB";

    for (int i = 0; i < N; i++)
        printf("P%d=%c%s", i + 1, letter[color[i]], i + 1 < N ? " " : "\n");
}

static void try_graph(const char *title)
{
    bool found;

    memset(color, 0, sizeof color);          /* WHITE = 0 */
    found = cycle(0);
    printf("%s\ncycle(P1) = %s\n", title, found ? "true" : "false");
    print_colors();
}

int main(void)
{
    edge(1, 2);
    edge(2, 3);
    edge(2, 4);
    edge(4, 5);
    edge(5, 2);
    try_graph("Graph 1: P1->P2, P2->P3, P2->P4, P4->P5, P5->P2");
    printf("back edge P5 -> P2: the cycle P2 -> P4 -> P5 -> P2 is deadlocked;\n"
           "P1 is not on the cycle but is stuck because it waits for P2.\n\n");

    waits[4][1] = false;                     /* remove the P5 -> P2 edge */
    try_graph("Graph 2: no P5->P2 edge");
    printf("no cycle: no deadlock\n");
    return 0;
}

/* Expected output:
 * Graph 1: P1->P2, P2->P3, P2->P4, P4->P5, P5->P2
 * cycle(P1) = true
 * P1=G P2=G P3=B P4=G P5=G
 * back edge P5 -> P2: the cycle P2 -> P4 -> P5 -> P2 is deadlocked;
 * P1 is not on the cycle but is stuck because it waits for P2.
 *
 * Graph 2: no P5->P2 edge
 * cycle(P1) = false
 * P1=B P2=B P3=B P4=B P5=B
 * no cycle: no deadlock
 */
