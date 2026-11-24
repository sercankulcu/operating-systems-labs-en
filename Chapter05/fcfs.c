/* FCFS (first come, first served) waiting time: processes run in arrival
 * order; waiting = the moment the CPU becomes free - arrival. */
#include <stdio.h>

typedef struct {
    const char *name;
    int arrival, burst;
} Proc;

/* p[] sorted by arrival time */
void fcfs(Proc p[], int n)
{
    int t = 0, sum = 0;
    for (int i = 0; i < n; i++) {
        if (t < p[i].arrival) t = p[i].arrival;
        int wait = t - p[i].arrival;
        t += p[i].burst;         /* completion */
        sum += wait;
        printf("%s: %d\n", p[i].name, wait);
    }
    printf("avg = %.2f\n", (double) sum / n);
}

int main(void)
{
    Proc p[] = {
        {"P1", 0, 8},
        {"P2", 1, 4},
        {"P3", 2, 9},
        {"P4", 3, 5},
    };
    fcfs(p, 4);
    return 0;
}

/* Expected output:
 * P1: 0
 * P2: 7
 * P3: 10
 * P4: 18
 * avg = 8.75
 */
