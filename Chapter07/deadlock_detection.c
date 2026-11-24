/* Deadlock detection with multiple instances per resource (Tanenbaum Figure 6-7).
 * A process whose request (Ri) can be met by the available resources (A) finishes
 * and returns what it holds (Ci); processes left unmarked at the end are deadlocked. */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define NP 3            /* processes */
#define M 4             /* resource types: tape drive, plotter, scanner, CD */

static void print_vector(const int v[M])
{
    printf("(%d %d %d %d)", v[0], v[1], v[2], v[3]);
}

static bool leq(const int r[M], const int a[M])
{
    for (int j = 0; j < M; j++)
        if (r[j] > a[j])
            return false;
    return true;
}

/* A <- available resource vector; all Pi unmarked; repeat: find an unmarked Pi
 * with Ri <= A, exit if none is found; A <- A + Ci; mark Pi.
 * Returns: the number of deadlocked processes. */
static int detect(const int avail[M], int C[NP][M], int R[NP][M])
{
    int A[M];
    bool marked[NP] = { false };
    int deadlocked = 0;

    memcpy(A, avail, sizeof A);
    for (;;) {
        int i;
        for (i = 0; i < NP; i++)
            if (!marked[i] && leq(R[i], A))
                break;
        if (i == NP)
            break;                          /* none found: exit */
        for (int j = 0; j < M; j++)
            A[j] += C[i][j];                /* Pi finishes, releases */
        marked[i] = true;
        printf("P%d: R%d = ", i + 1, i + 1);
        print_vector(R[i]);
        printf(" <= A -> finishes, A = ");
        print_vector(A);
        printf("\n");
    }
    for (int i = 0; i < NP; i++) {
        if (!marked[i]) {
            printf("P%d unmarked: deadlocked\n", i + 1);
            deadlocked++;
        }
    }
    if (deadlocked == 0)
        printf("All processes marked: no deadlock\n");
    return deadlocked;
}

int main(void)
{
    const int A[M] = { 2, 1, 0, 0 };        /* E = (4 2 3 1) */
    int C[NP][M] = {                        /* allocation matrix */
        { 0, 0, 1, 0 },
        { 2, 0, 0, 1 },
        { 0, 1, 2, 0 },
    };
    int R[NP][M] = {                        /* request matrix */
        { 2, 0, 0, 1 },
        { 1, 0, 1, 0 },
        { 2, 1, 0, 0 },
    };

    printf("Example: A = (2 1 0 0)\n");
    detect(A, C, R);

    printf("\nVariant: if P3 also wants a CD, R3 = (2 1 0 1)\n");
    R[2][3] = 1;
    detect(A, C, R);
    return 0;
}

/* Expected output:
 * Example: A = (2 1 0 0)
 * P3: R3 = (2 1 0 0) <= A -> finishes, A = (2 2 2 0)
 * P2: R2 = (1 0 1 0) <= A -> finishes, A = (4 2 2 1)
 * P1: R1 = (2 0 0 1) <= A -> finishes, A = (4 2 3 1)
 * All processes marked: no deadlock
 *
 * Variant: if P3 also wants a CD, R3 = (2 1 0 1)
 * P1 unmarked: deadlocked
 * P2 unmarked: deadlocked
 * P3 unmarked: deadlocked
 */
