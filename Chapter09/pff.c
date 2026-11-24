/* Page-fault frequency (PFF): on every fault, look at the time since the last fault.
 * If the interval is greater than C (rare faults), pages not used since the last
 * fault (R = 0) are evicted; otherwise the set grows. C = 2, 14 references. */
#include <stdio.h>

#define C 2
#define MAXN 16

typedef struct { int page, R; } Entry;

typedef struct {
    Entry set[MAXN];
    int n;                      /* number of pages in the set */
    int last;                   /* time of the last fault */
} Proc;

static void evict(Proc *s, int i)
{
    printf(" p%d evicted;", s->set[i].page);
    for (int j = i; j < s->n - 1; j++)
        s->set[j] = s->set[j + 1];
    s->n--;
}

static void add(Proc *s, int p)
{
    s->set[s->n].page = p;
    s->set[s->n].R = 1;
    s->n++;
}

void pff(Proc *s, int p, int t)
{
    if (t - s->last > C) {  /* rare faults */
        for (int i = s->n - 1; i >= 0; i--)
            if (s->set[i].R == 0)
                evict(s, i);    /* unused */
    }
    for (int i = 0; i < s->n; i++)
        s->set[i].R = 0;      /* clear */
    add(s, p);              /* with R = 1 */
    s->last = t;
}

static void print_set(const Proc *s)
{
    printf(" set = {");
    for (int i = 0; i < s->n; i++)
        printf("%sp%d", i ? " " : "", s->set[i].page);
    printf("}\n");
}

int main(void)
{
    const int ref[15] = {0, 1, 2, 1, 3, 1, 3, 1, 3, 4, 4, 5, 4, 5, 2};  /* ref[t], t = 1..14 */
    Proc s = { .n = 0, .last = 0 };
    int faults = 0;

    for (int t = 1; t <= 14; t++) {
        int p = ref[t], found = 0;
        for (int i = 0; i < s.n; i++)
            if (s.set[i].page == p) {
                s.set[i].R = 1;
                found = 1;
            }
        if (found)
            continue;                       /* hit */
        faults++;
        printf("t=%2d p%d FAULT, interval %d:", t, p, t - s.last);
        pff(&s, p, t);
        print_set(&s);
    }
    printf("%d faults in 14 references\n", faults);
    return 0;
}

/* Expected output:
 * t= 1 p1 FAULT, interval 1: set = {p1}
 * t= 2 p2 FAULT, interval 1: set = {p1 p2}
 * t= 4 p3 FAULT, interval 2: set = {p1 p2 p3}
 * t= 9 p4 FAULT, interval 5: p2 evicted; set = {p1 p3 p4}
 * t=11 p5 FAULT, interval 2: set = {p1 p3 p4 p5}
 * t=14 p2 FAULT, interval 3: p3 evicted; p1 evicted; set = {p4 p5 p2}
 * 6 faults in 14 references
 */
