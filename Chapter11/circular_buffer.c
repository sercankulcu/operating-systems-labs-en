/* Circular buffer: the producer writes with put() at the tail (write) index, the
 * consumer reads with get() at the head (read) index. A buffer with N slots holds
 * at most N - 1 bytes; one slot is left empty to tell "full" from "empty". */
#include <stdio.h>

#define N 5

char buf[N];
int head = 0, tail = 0;

int put(char c)            /* producer */
{
    int s = (tail + 1) % N;
    if (s == head) return -1; /* full */
    buf[tail] = c;
    tail = s;
    return 0;
}

int get(char *c)           /* consumer */
{
    if (head == tail) return -1; /* empty */
    *c = buf[head];
    head = (head + 1) % N;
    return 0;
}

static void put_print(char c)
{
    if (put(c) < 0)
        printf("put('%c'): full\n", c);
}

static void get_print(void)
{
    char c;
    if (get(&c) == 0)
        printf("get: %c\n", c);
    else
        printf("get: empty\n");
}

int main(void)
{
    put_print('a');
    put_print('b');
    put_print('c');
    get_print();
    put_print('d');
    put_print('e');      /* tail wraps around: tail = 0 */
    put_print('f');      /* (0 + 1) % 5 == head: full */
    get_print();
    printf("head = %d, tail = %d\n", head, tail);
    return 0;
}

/* Expected output:
 * get: a
 * put('f'): full
 * get: b
 * head = 2, tail = 0
 */
