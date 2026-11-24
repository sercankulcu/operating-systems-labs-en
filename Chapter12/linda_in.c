/* Linda tuple space: in() finds the first tuple matching the template, fills in the
 * formal fields and removes the tuple from the space; if none matches the process
 * blocks (here NULL is returned). Example: in("abc", 2, ? int i). */
#include <stdio.h>
#include <string.h>

#define MAXFIELDS 4
enum { INT, STRING };           /* field types */

typedef struct Value { int i; const char *s; } Value;

typedef struct Tuple {
    const char *name;           /* for tracing: d1, d2, ... */
    int n;                      /* number of fields */
    int type[MAXFIELDS];
    Value v[MAXFIELDS];
    struct Tuple *next;         /* next tuple */
} Tuple;

typedef struct Template {
    int n;
    int type[MAXFIELDS];
    int fixed[MAXFIELDS];       /* 1: actual value, 0: formal (? int i) */
    Value v[MAXFIELDS];
} Template;

Tuple *space = NULL;

static int equal(Tuple *d, Template *s, int k)
{
    if (d->type[k] == INT)
        return d->v[k].i == s->v[k].i;
    return strcmp(d->v[k].s, s->v[k].s) == 0;
}

static void remove_tuple(Tuple *d)
{
    Tuple **p = &space;
    while (*p && *p != d)
        p = &(*p)->next;
    if (*p)
        *p = d->next;
}

Tuple *in_find(Template *s)
{
    for (Tuple *d = space; d; d = d->next) {
        if (d->n != s->n) continue;
        int ok = 1;
        for (int k = 0; ok && k < s->n; k++)
            if (d->type[k] != s->type[k] ||
                (s->fixed[k] && !equal(d, s, k)))
                ok = 0;          /* field mismatch */
        if (ok) { remove_tuple(d); return d; }
    }
    return NULL;           /* block */
}

int main(void)
{
    static Tuple d1 = { "d1", 2, { STRING, INT },      { { 0, "abc" }, { 2, NULL } },              NULL };
    static Tuple d2 = { "d2", 3, { STRING, INT, INT }, { { 0, "xyz" }, { 2, NULL }, { 7, NULL } }, NULL };
    static Tuple d3 = { "d3", 3, { STRING, INT, INT }, { { 0, "abc" }, { 1, NULL }, { 4, NULL } }, NULL };
    static Tuple d4 = { "d4", 3, { STRING, INT, INT }, { { 0, "abc" }, { 2, NULL }, { 5, NULL } }, NULL };
    static Tuple d5 = { "d5", 3, { STRING, INT, INT }, { { 0, "abc" }, { 2, NULL }, { 9, NULL } }, NULL };
    d1.next = &d2; d2.next = &d3; d3.next = &d4; d4.next = &d5;
    space = &d1;

    /* in("abc", 2, ? int i) */
    Template s = { 3, { STRING, INT, INT }, { 1, 1, 0 },
                   { { 0, "abc" }, { 2, NULL }, { 0, NULL } } };

    for (int c = 1; c <= 3; c++) {
        Tuple *d = in_find(&s);
        if (d) {
            int i = d->v[2].i;       /* the formal field is filled in */
            printf("%d. in: i = %d (%s)\n", c, i, d->name);
        } else {
            printf("%d. in: no matching tuple, the process would block\n", c);
        }
    }
    return 0;
}

/* Expected output:
 * 1. in: i = 5 (d4)
 * 2. in: i = 9 (d5)
 * 3. in: no matching tuple, the process would block
 */
