/* Where variables live: prints the addresses of objects in the text, data,
 * BSS, heap and stack regions.
 * In the compiled program:  size a.out    (text/data/BSS sizes)
 * In a running process:     cat /proc/PID/maps */
#include <stdio.h>
#include <stdlib.h>

int counter = 5;    /* data */
int table[1000];  /* bss */

int main(int argc, char *argv[])
{
    int local = 3;
    static int count;
    int *p = malloc(4 * sizeof *p);
    if (p == NULL)
        return 1;
    p[0] = local + counter;

    printf("text  : main    = %p\n", (void *)main);
    printf("data  : counter = %p (value %d)\n", (void *)&counter, counter);
    printf("bss   : table   = %p (table[0] = %d)\n", (void *)table, table[0]);
    printf("bss   : count   = %p (value %d)\n", (void *)&count, count);
    printf("heap  : *p      = %p (p[0] = %d)\n", (void *)p, p[0]);
    printf("stack : local   = %p, p = %p, argc = %p\n",
           (void *)&local, (void *)&p, (void *)&argc);
    (void)argv;

    free(p);
    return 0;
}

/* Expected output (addresses vary on every run, ASLR):
 * text  : main    = 0x55...      lowest address
 * data  : counter = 0x55... (value 5)
 * bss   : table   = 0x55... (table[0] = 0)
 * bss   : count   = 0x55... (value 0)
 * heap  : *p      = 0x55... (p[0] = 8)   just above data/BSS
 * stack : local   = 0x7ff..., p = 0x7ff..., argc = 0x7ff...   highest addresses
 */
