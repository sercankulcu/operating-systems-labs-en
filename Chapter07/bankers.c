/* Banker's algorithm: safety test and resource-request algorithm
 * (Silberschatz 10e 8.6.3 example: A = 10, B = 5, C = 7 instances, T0..T4).
 * Need = Max - Allocation; a request is granted only if the resulting state is safe. */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define N 5             /* threads */
#define M 3             /* resource types */

static int Available[M] = { 3, 3, 2 };
static int Max[N][M] = {
    { 7, 5, 3 }, { 3, 2, 2 }, { 9, 0, 2 }, { 2, 2, 2 }, { 4, 3, 3 },
};
static int Allocation[N][M] = {
    { 0, 1, 0 }, { 2, 0, 0 }, { 3, 0, 2 }, { 2, 1, 1 }, { 0, 0, 2 },
};
static int Need[N][M];

static bool leq(const int a[M], const int b[M])
{
    for (int j = 0; j < M; j++)
        if (a[j] > b[j])
            return false;
    return true;
}

static void print_vector(const int v[M])
{
    printf("(%d %d %d)", v[0], v[1], v[2]);
}

/* Work <- Available; Finish <- false
 * for i = T0 ... T4 (repeatedly):
 *   if !Finish[i] and Need[i] <= Work: Work <- Work + Allocation[i]; Finish[i] <- true
 * return all Finish[i] */
static bool is_safe(void)
{
    int Work[M];
    bool Finish[N] = { false };
    int order[N], k = 0;
    bool progressed = true;

    memcpy(Work, Available, sizeof Work);
    while (progressed) {                    /* repeatedly */
        progressed = false;
        for (int i = 0; i < N; i++) {
            if (!Finish[i] && leq(Need[i], Work)) {
                for (int j = 0; j < M; j++)
                    Work[j] += Allocation[i][j];
                Finish[i] = true;
                order[k++] = i;
                progressed = true;
            }
        }
    }
    printf("  order:");
    for (int i = 0; i < k; i++)
        printf(" T%d", order[i]);
    printf("  Work = ");
    print_vector(Work);
    printf(" -> %s\n", k == N ? "SAFE" : "UNSAFE");
    return k == N;
}

/* Ti makes Request. Returns: 1 granted, 0 waits, -1 error */
static int request(int i, const int Request[M])
{
    printf("T%d, Request = ", i);
    print_vector(Request);
    printf(" requested\n");
    if (!leq(Request, Need[i])) {
        printf("  error: Max exceeded\n");
        return -1;
    }
    if (!leq(Request, Available)) {
        printf("  T%d waits: resources not available\n", i);
        return 0;
    }
    for (int j = 0; j < M; j++) {           /* pretend to allocate */
        Available[j] -= Request[j];
        Allocation[i][j] += Request[j];
        Need[i][j] -= Request[j];
    }
    if (is_safe()) {
        printf("  request GRANTED\n");
        return 1;
    }
    for (int j = 0; j < M; j++) {           /* roll back */
        Available[j] += Request[j];
        Allocation[i][j] -= Request[j];
        Need[i][j] += Request[j];
    }
    printf("  rolled back, T%d waits\n", i);
    return 0;
}

int main(void)
{
    const int r1[M] = { 0, 1, 1 };
    const int r2[M] = { 3, 3, 0 };

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++)
            Need[i][j] = Max[i][j] - Allocation[i][j];
        printf("T%d Need = ", i);
        print_vector(Need[i]);
        printf("\n");
    }

    printf("Safety test, Available = (3 3 2):\n");
    is_safe();

    request(3, r1);
    request(3, r1);                         /* Need[3] is now (0 0 0) */
    request(4, r2);                         /* Available (3 2 1): B is not enough */
    return 0;
}

/* Expected output:
 * T0 Need = (7 4 3)
 * T1 Need = (1 2 2)
 * T2 Need = (6 0 0)
 * T3 Need = (0 1 1)
 * T4 Need = (4 3 1)
 * Safety test, Available = (3 3 2):
 *   order: T1 T3 T4 T0 T2  Work = (10 5 7) -> SAFE
 * T3, Request = (0 1 1) requested
 *   order: T3 T4 T1 T2 T0  Work = (10 5 7) -> SAFE
 *   request GRANTED
 * T3, Request = (0 1 1) requested
 *   error: Max exceeded
 * T4, Request = (3 3 0) requested
 *   T4 waits: resources not available
 */
