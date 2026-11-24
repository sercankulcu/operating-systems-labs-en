/* Livelock: A wants resource_1 first, B wants resource_2 first (Tanenbaum 6-16).
 * 1) enter_region/leave_region are busy-waiting locks built on a C11 atomic_flag.
 * 2) When "polite" processes take-try-release in the same rhythm, neither makes
 *    progress; random back-off (srand(42)) breaks the loop. Step-by-step,
 *    single-threaded simulation. */
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* ---- 1) Real code: busy-waiting locks ---- */
static atomic_flag resource_1 = ATOMIC_FLAG_INIT;
static atomic_flag resource_2 = ATOMIC_FLAG_INIT;
static int uses = 0;

static void enter_region(atomic_flag *r)
{
    while (atomic_flag_test_and_set(r)) {
        /* busy wait: does not block, burns CPU */
    }
}

static void leave_region(atomic_flag *r)
{
    atomic_flag_clear(r);
}

static void use_both_resources(void)
{
    uses++;
}

static void process_A(void)
{
    enter_region(&resource_1);
    enter_region(&resource_2);
    use_both_resources();
    leave_region(&resource_2);
    leave_region(&resource_1);
}

static void process_B(void)            /* B: resource_2 first, then 1 */
{
    enter_region(&resource_2);
    enter_region(&resource_1);
    use_both_resources();
    leave_region(&resource_1);
    leave_region(&resource_2);
}

static void *run_A(void *arg)
{
    (void)arg;
    process_A();
    return NULL;
}

static void *run_B(void *arg)
{
    (void)arg;
    process_B();
    return NULL;
}

/* ---- 2) Step-by-step simulation: polite processes ---- */
#define MAX_ROUNDS 30

struct proc {
    char name;
    int first, second;          /* resource number: 0 = r1, 1 = r2 */
    int phase;                  /* 0: take the first, 1: try the second, 2: done */
    int wait;                   /* remaining back-off rounds */
    int finish;
};

static int simulate(bool random_backoff, bool verbose)
{
    struct proc s[2] = { { 'A', 0, 1, 0, 0, 0 }, { 'B', 1, 0, 0, 0, 0 } };
    int owner[2] = { -1, -1 };

    for (int t = 1; t <= MAX_ROUNDS; t++) {
        int before[2] = { owner[0], owner[1] }; /* state at the start of the round */
        int release[2] = { -1, -1 };            /* to release at the end of the round */

        if (verbose)
            printf("round %2d:", t);
        for (int i = 0; i < 2; i++) {
            struct proc *p = &s[i];
            if (p->phase == 2)
                continue;
            if (p->wait > 0) {
                p->wait--;
                if (verbose)
                    printf("  %c waiting", p->name);
                continue;
            }
            if (p->phase == 0) {
                if (before[p->first] == -1 && owner[p->first] == -1) {
                    owner[p->first] = i;
                    p->phase = 1;
                    if (verbose)
                        printf("  %c took r%d", p->name, p->first + 1);
                }
            } else if (before[p->second] == -1 && owner[p->second] == -1) {
                owner[p->second] = i;             /* holds both: run */
                release[i] = 1;
                p->phase = 2;
                p->finish = t;
                if (verbose)
                    printf("  %c took r%d, ran", p->name, p->second + 1);
            } else {
                release[i] = 0;                   /* politely release */
                p->phase = 0;
                p->wait = random_backoff ? rand() % 4 : 0;
                if (verbose)
                    printf("  %c could not take r%d, released r%d", p->name,
                           p->second + 1, p->first + 1);
            }
        }
        if (verbose)
            printf("\n");
        for (int i = 0; i < 2; i++) {
            if (release[i] >= 0)
                owner[s[i].first] = -1;
            if (release[i] == 1)
                owner[s[i].second] = -1;
        }
        if (s[0].phase == 2 && s[1].phase == 2) {
            printf("A finished in round %d, B in round %d\n", s[0].finish, s[1].finish);
            return t;
        }
        if (!random_backoff && t == 6) {
            printf("no progress in 6 rounds: livelock (goes on forever)\n");
            return -1;
        }
    }
    printf("did not finish in %d rounds\n", MAX_ROUNDS);
    return -1;
}

int main(void)
{
    pthread_t t;

    /* A and B run one after the other: run together they could spin forever */
    pthread_create(&t, NULL, run_A, NULL);
    pthread_join(t, NULL);
    pthread_create(&t, NULL, run_B, NULL);
    pthread_join(t, NULL);
    printf("one after the other: use_both_resources ran %d times\n\n", uses);

    printf("Polite processes, same rhythm:\n");
    simulate(false, true);

    printf("\nRandom back-off (0-3 rounds):\n");
    srand(42);
    simulate(true, true);
    return 0;
}

/* Expected output (the details of part 2 depend on the glibc rand() sequence):
 * one after the other: use_both_resources ran 2 times
 *
 * Polite processes, same rhythm:
 * round  1:  A took r1  B took r2
 * round  2:  A could not take r2, released r1  B could not take r1, released r2
 * round  3:  A took r1  B took r2
 * round  4:  A could not take r2, released r1  B could not take r1, released r2
 * round  5:  A took r1  B took r2
 * round  6:  A could not take r2, released r1  B could not take r1, released r2
 * no progress in 6 rounds: livelock (goes on forever)
 *
 * Random back-off (0-3 rounds):
 * round  1:  A took r1  B took r2
 * round  2:  A could not take r2, released r1  B could not take r1, released r2
 * round  3:  A waiting  B took r2
 * round  4:  A waiting  B took r1, ran
 * round  5:  A took r1
 * round  6:  A took r2, ran
 * A finished in round 6, B in round 4
 */
