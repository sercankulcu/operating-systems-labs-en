/* Disk arm scheduling: SSTF picks the pending request closest to the head at every
 * step; LOOK sweeps up first, then down. The head is at cylinder 50, the queue
 * (arrival order) is 82, 170, 43, 140, 24, 16, 190. */
#include <stdio.h>
#include <stdlib.h>

#define NMAX 16

int done[NMAX];              /* SSTF: requests already served */

int sstf(int head, int q[], int n)
{
    int total = 0;
    for (int k = 0; k < n; k++) {
        int best = -1;
        for (int i = 0; i < n; i++)
            if (!done[i] && (best < 0 ||
                abs(q[i]-head) < abs(q[best]-head)))
                best = i;          /* closest */
        total += abs(q[best] - head);
        head = q[best];
        done[best] = 1;
        printf("%d ", head);
    }
    printf("\n");
    return total;
}

/* s[] sorted; go up first, then down */
int look(int head, int s[], int n)
{
    int i = 0, total = 0;
    while (i < n && s[i] < head) i++;
    for (int j = i; j < n; j++) {
        total += s[j] - head; head = s[j];
        printf("%d ", head);
    }
    for (int j = i - 1; j >= 0; j--) {
        total += head - s[j]; head = s[j];
        printf("%d ", head);
    }
    printf("\n");
    return total;
}

static int compare(const void *a, const void *b)
{
    return *(const int *)a - *(const int *)b;
}

int main(void)
{
    int q[] = { 82, 170, 43, 140, 24, 16, 190 };
    int n = (int)(sizeof q / sizeof q[0]);
    int s[NMAX];

    printf("SSTF: ");
    int t1 = sstf(50, q, n);
    printf("SSTF total = %d cylinders\n", t1);

    for (int k = 0; k < n; k++)
        s[k] = q[k];
    qsort(s, (size_t)n, sizeof s[0], compare);
    printf("LOOK: ");
    int t2 = look(50, s, n);
    printf("LOOK total = %d cylinders\n", t2);
    return 0;
}

/* Expected output:
 * SSTF: 43 24 16 82 140 170 190
 * SSTF total = 208 cylinders
 * LOOK: 82 140 170 190 43 24 16
 * LOOK total = 314 cylinders
 */
