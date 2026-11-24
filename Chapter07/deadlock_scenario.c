/* How a deadlock arises / how it can be avoided (Tanenbaum Figure 6-4):
 * processes A, B, C request and release resources R, S, T in the given order.
 * The same programs are run step by step under two different schedules. */
#include <stdio.h>

#define NP 3                    /* processes: A, B, C */
#define NR 3                    /* resources: R, S, T */

enum { REQUEST, RELEASE };

struct op { int type; int resource; };

/*  A          B          C
 *  request R  request S  request T
 *  request S  request T  request R
 *  release R  release S  release T
 *  release S  release T  release R */
static const struct op program[NP][4] = {
    { {REQUEST, 0}, {REQUEST, 1}, {RELEASE, 0}, {RELEASE, 1} },
    { {REQUEST, 1}, {REQUEST, 2}, {RELEASE, 1}, {RELEASE, 2} },
    { {REQUEST, 2}, {REQUEST, 0}, {RELEASE, 2}, {RELEASE, 0} },
};

static const char pname[] = "ABC";
static const char rname[] = "RST";

static int pc[NP];              /* next operation */
static int waiting_for[NP];     /* -1: not blocked */
static int owner[NR];           /* -1: free */
static int step_no;

static void reset(void)
{
    step_no = 0;
    for (int p = 0; p < NP; p++) {
        pc[p] = 0;
        waiting_for[p] = -1;
    }
    for (int r = 0; r < NR; r++)
        owner[r] = -1;
}

/* perform process p's next operation */
static void step(int p)
{
    printf("%2d. ", ++step_no);
    if (pc[p] == 4 || waiting_for[p] != -1) {
        printf("%c cannot run\n", pname[p]);
        return;
    }

    const struct op *o = &program[p][pc[p]];
    int r = o->resource;

    if (o->type == REQUEST) {
        if (owner[r] == -1) {
            owner[r] = p;
            pc[p]++;
            printf("%c requests %c and gets it.\n", pname[p], rname[r]);
        } else {
            waiting_for[p] = r;
            printf("%c requests %c: %c is held by %c -> %c blocked.\n",
                   pname[p], rname[r], rname[r], pname[owner[r]], pname[p]);
        }
        return;
    }
    /* RELEASE: if someone is waiting for the resource, hand it over */
    owner[r] = -1;
    pc[p]++;
    printf("%c releases %c", pname[p], rname[r]);
    for (int q = 0; q < NP; q++) {
        if (waiting_for[q] == r) {
            owner[r] = q;
            waiting_for[q] = -1;
            pc[q]++;
            printf(" -> %c is given to %c", rname[r], pname[q]);
            break;
        }
    }
    printf("%s\n", pc[p] == 4 ? "; finishes." : ".");
}

static void result(void)
{
    int finished = 0, blocked = 0;

    for (int p = 0; p < NP; p++) {
        if (pc[p] == 4)
            finished++;
        else if (waiting_for[p] != -1)
            blocked++;
    }
    if (blocked == NP)
        printf("All blocked, cycle closed: DEADLOCK\n\n");
    else
        printf("Finished: %d, blocked: %d -> no deadlock\n\n", finished, blocked);
}

static void run(const char *title, const char *order)
{
    printf("%s\n", title);
    reset();
    for (const char *c = order; *c; c++)
        step(*c - 'A');
    result();
}

int main(void)
{
    run("Requests in the order they arrive (A B C A B C):", "ABCABC");
    run("The OS keeps B waiting for a while:", "ACACAABBCCBB");
    return 0;
}

/* Expected output:
 * Requests in the order they arrive (A B C A B C):
 *  1. A requests R and gets it.
 *  2. B requests S and gets it.
 *  3. C requests T and gets it.
 *  4. A requests S: S is held by B -> A blocked.
 *  5. B requests T: T is held by C -> B blocked.
 *  6. C requests R: R is held by A -> C blocked.
 * All blocked, cycle closed: DEADLOCK
 *
 * The OS keeps B waiting for a while:
 *  1. A requests R and gets it.
 *  2. C requests T and gets it.
 *  3. A requests S and gets it.
 *  4. C requests R: R is held by A -> C blocked.
 *  5. A releases R -> R is given to C.
 *  6. A releases S; finishes.
 *  7. B requests S and gets it.
 *  8. B requests T: T is held by C -> B blocked.
 *  9. C releases T -> T is given to B.
 * 10. C releases R; finishes.
 * 11. B releases S.
 * 12. B releases T; finishes.
 * Finished: 3, blocked: 0 -> no deadlock
 */
