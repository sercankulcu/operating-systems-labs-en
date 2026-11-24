/* EDF (earliest deadline first): every millisecond, the ready job with the
 * nearest deadline runs. A(P=3,C=1) B(P=4,C=1) C(P=6,C=2),
 * U = 1/3 + 1/4 + 2/6 = 0.917; the 12 ms common period is traced. */
#include <stdio.h>

int main(void)
{
    const char name[] = {'A', 'B', 'C'};
    int P[] = {3, 4, 6};
    int C[] = {1, 1, 2};
    int n = 3;
    int left[3] = {0, 0, 0};
    int due[3] = {0, 0, 0};
    int missed = 0;

    /* A(P=3,C=1) B(P=4,C=1) C(P=6,C=2) */
    for (int t = 0; t < 12; t++) {
        for (int i = 0; i < n; i++)
            if (t % P[i] == 0) {      /* new job */
                if (left[i] > 0)
                    missed++;         /* the previous job missed its deadline */
                left[i] = C[i];
                due[i] = t + P[i];
            }
        int s = -1;
        for (int i = 0; i < n; i++)
            if (left[i] > 0 &&
                (s < 0 || due[i] < due[s]))
                s = i;
        if (s >= 0) left[s]--;     /* 1 ms */
        printf("t=%2d: %c\n", t, s >= 0 ? name[s] : '-');
    }
    printf("missed deadlines: %d\n", missed);
    return 0;
}

/* Expected output:
 * t= 0: A
 * t= 1: B
 * t= 2: C
 * t= 3: A
 * t= 4: C
 * t= 5: B
 * t= 6: A
 * t= 7: C
 * t= 8: B
 * t= 9: A
 * t=10: C
 * t=11: -
 * missed deadlines: 0
 */
