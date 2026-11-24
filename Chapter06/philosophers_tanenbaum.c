/* Dining philosophers, Tanenbaum's solution: each philosopher has a state (THINKING,
 * HUNGRY, EATING) and its own semaphore s[i]; a philosopher eats only when both
 * neighbours are not eating. First the trace from the slide (1 and 3 eat, 2 waits,
 * when 1 and 3 put the forks down 2 eats) is played, then five philosophers eat
 * 1000 times each and the rule is checked. */
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <time.h>

#define N        5
#define LEFT     ((i + N - 1) % N)   /* (i+4) % 5 */
#define RIGHT    ((i + 1) % N)       /* (i+1) % 5 */
#define THINKING 0
#define HUNGRY   1
#define EATING   2
#define MEALS    1000

typedef sem_t semaphore;
static void down(semaphore *x) { sem_wait(x); }
static void up(semaphore *x)   { sem_post(x); }

int state[N];
semaphore mutex;           /* = 1 */
semaphore s[N];            /* all 0 */

void test(int i)
{
    if (state[i] == HUNGRY && state[LEFT] != EATING
                           && state[RIGHT] != EATING) {
        state[i] = EATING;
        up(&s[i]);
    }
}

void take_forks(int i)
{
    down(&mutex);
    state[i] = HUNGRY;
    test(i);
    up(&mutex);
    down(&s[i]);    /* block if no forks */
}

void put_forks(int i)
{
    down(&mutex);
    state[i] = THINKING;
    test(LEFT);     /* (i+4) % 5 */
    test(RIGHT);    /* (i+1) % 5 */
    up(&mutex);
}

static void print_state(const char *event)
{
    char d[N + 1];
    down(&mutex);
    for (int k = 0; k < N; k++)
        d[k] = state[k] == THINKING ? 'T' : state[k] == HUNGRY ? 'H' : 'E';
    up(&mutex);
    d[N] = '\0';
    printf("state = %s%s%s\n", d, event[0] != '\0' ? "   " : "", event);
}

static void *philosopher_two(void *arg)  /* philosopher 2 in the trace */
{
    (void)arg;
    take_forks(2);                       /* blocks because 1 is eating */
    return NULL;
}

/* ---- free run ---- */
static int ate[N];
static atomic_int violations = 0;

static void eat(int i)
{
    down(&mutex);
    if (state[LEFT] == EATING || state[RIGHT] == EATING || state[i] != EATING)
        atomic_fetch_add(&violations, 1);
    up(&mutex);
    ate[i]++;
}

static void *philosopher(void *arg)
{
    int i = *(int *)arg;
    for (int k = 0; k < MEALS; k++) {   /* MEALS rounds instead of while (true) */
        /* think */
        take_forks(i);
        eat(i);
        put_forks(i);
    }
    return NULL;
}

int main(void)
{
    sem_init(&mutex, 0, 1);
    for (int i = 0; i < N; i++)
        sem_init(&s[i], 0, 0);

    /* 1) The trace from the slide */
    print_state("");
    take_forks(1);
    print_state("1 took the forks, eating");
    pthread_t t2;
    pthread_create(&t2, NULL, philosopher_two, NULL);
    for (;;) {                                   /* until 2 is HUNGRY */
        down(&mutex);
        int h = state[2] == HUNGRY;
        up(&mutex);
        if (h)
            break;
        struct timespec ts = { 0, 1000000 };
        nanosleep(&ts, NULL);
    }
    print_state("2 hungry: neighbour 1 is eating, blocked on s[2]");
    take_forks(3);
    print_state("3 eating (neighbours 2 and 4 are not eating)");
    put_forks(1);
    print_state("1 put down: test(2) fails, 3 eating");
    put_forks(3);
    pthread_join(t2, NULL);                      /* test(2) succeeds: 2 woke up */
    print_state("3 put down: test(2) succeeds, 2 eating");
    put_forks(2);

    /* 2) Free run */
    pthread_t t[N];
    int id[N];
    for (int i = 0; i < N; i++) {
        id[i] = i;
        pthread_create(&t[i], NULL, philosopher, &id[i]);
    }
    for (int i = 0; i < N; i++)
        pthread_join(t[i], NULL);
    printf("meal counts:");
    for (int i = 0; i < N; i++)
        printf(" %d", ate[i]);
    printf("\nneighbours ate at the same time: %d times\n", atomic_load(&violations));

    sem_destroy(&mutex);
    for (int i = 0; i < N; i++)
        sem_destroy(&s[i]);
    return 0;
}

/* Expected output (same on every run):
 * state = TTTTT
 * state = TETTT   1 took the forks, eating
 * state = TEHTT   2 hungry: neighbour 1 is eating, blocked on s[2]
 * state = TEHET   3 eating (neighbours 2 and 4 are not eating)
 * state = TTHET   1 put down: test(2) fails, 3 eating
 * state = TTETT   3 put down: test(2) succeeds, 2 eating
 * meal counts: 1000 1000 1000 1000 1000
 * neighbours ate at the same time: 0 times
 */
