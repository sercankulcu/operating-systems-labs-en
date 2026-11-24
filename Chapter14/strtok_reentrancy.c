/* Reuse and reentrancy: strtok keeps its state in a static pointer, so an
 * interleaved call (an interrupt handler) corrupts main's place. strtok_r leaves
 * the state to the caller and fixes the problem. The "interrupt" is simulated
 * deterministically between two calls. */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

char *my_strtok(char *s, char sep)
{
    static char *rest;      /* shared by all! */
    if (s) rest = s;
    if (!rest || !*rest) return NULL;
    char *tok = rest;
    while (*rest && *rest != sep) rest++;
    if (*rest) *rest++ = '\0';
    return tok;
}

int main(void)
{
    char a[] = "ali,veli", b[] = "x;y";

    printf("my_strtok:\n");
    printf("  main:  %s\n", my_strtok(a, ','));     /* a = "ali,veli" */
    printf("  irq:   %s\n", my_strtok(b, ';'));     /* b = "x;y" */
    printf("  main:  %s  (wrong, expected veli)\n", my_strtok(NULL, ','));

    /* libc strtok uses the same kind of static state */
    char a2[] = "ali,veli", b2[] = "x;y";
    printf("strtok:\n");
    printf("  main:  %s\n", strtok(a2, ","));
    printf("  irq:   %s\n", strtok(b2, ";"));
    printf("  main:  %s  (wrong)\n", strtok(NULL, ","));

    /* reentrant version: state in the caller */
    char a3[] = "ali,veli", b3[] = "x;y";
    char *main_rest, *irq_rest;
    printf("strtok_r:\n");
    printf("  main:  %s\n", strtok_r(a3, ",", &main_rest));
    printf("  irq:   %s\n", strtok_r(b3, ";", &irq_rest));
    printf("  main:  %s  (correct)\n", strtok_r(NULL, ",", &main_rest));
    return 0;
}

/* Expected output:
 * my_strtok:
 *   main:  ali
 *   irq:   x
 *   main:  y  (wrong, expected veli)
 * strtok:
 *   main:  ali
 *   irq:   x
 *   main:  y  (wrong)
 * strtok_r:
 *   main:  ali
 *   irq:   x
 *   main:  veli  (correct)
 */
