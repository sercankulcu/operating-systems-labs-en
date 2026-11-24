/* Timer list: pending events are kept not as absolute times but as differences
 * from the previous one. add() inserts and updates only one difference; tick()
 * decrements only the first difference (O(1) per tick). */
#include <stdio.h>

typedef struct Timer {
    const char *name;
    int delta;               /* tick difference from the previous timer */
    struct Timer *next;
} Timer;

Timer *head = NULL;
int now = 4200;              /* current time (ticks) */

Timer *run(Timer *z)
{
    printf("%d: %s ran\n", now, z->name);
    return z->next;          /* remove from the list */
}

void add(Timer *z, int t)    /* t: ticks */
{
    Timer **p = &head;
    while (*p && (*p)->delta <= t) {
        t -= (*p)->delta;
        p = &(*p)->next;
    }
    z->delta = t; z->next = *p;
    if (*p) (*p)->delta -= t;
    *p = z;
}

void tick(void)
{
    now++;
    if (head) head->delta--;
    while (head && head->delta == 0)
        head = run(head);   /* remove */
}

static void list_print(void)
{
    printf("list:");
    for (Timer *z = head; z; z = z->next)
        printf(" %s(%d)", z->name, z->delta);
    printf("\n");
}

int main(void)
{
    static Timer A = { "A", 0, NULL }, B = { "B", 0, NULL }, C = { "C", 0, NULL },
                 D = { "D", 0, NULL }, E = { "E", 0, NULL }, X = { "X", 0, NULL };

    /* events at 4203, 4207, 4213, 4215, 4216 */
    add(&A, 3);
    add(&B, 7);
    add(&C, 13);
    add(&D, 15);
    add(&E, 16);
    list_print();

    add(&X, 10);             /* 4210: goes between B and C */
    list_print();

    for (int k = 0; k < 16; k++)
        tick();
    list_print();
    return 0;
}

/* Expected output:
 * list: A(3) B(4) C(6) D(2) E(1)
 * list: A(3) B(4) X(3) C(3) D(2) E(1)
 * 4203: A ran
 * 4207: B ran
 * 4210: X ran
 * 4213: C ran
 * 4215: D ran
 * 4216: E ran
 * list:
 */
