/* Hiding the hardware: hardware-dependent code is separated by conditional
 * compilation. In the slides CPU and WORD_LENGTH come from "config.h"; since each
 * file here is self-contained, the contents of config.h are written out below and
 * chosen from the macros the compiler defines. Example: can be changed with
 * gcc -DCPU=ARM64. */
#include <stdio.h>
#include <stdint.h>

/* ---- config.h ---- */
#define X86_64  1
#define ARM64   2
#define RISCV64 3
#ifndef CPU
#  if defined(__x86_64__)
#    define CPU X86_64
#  elif defined(__aarch64__)
#    define CPU ARM64
#  elif defined(__riscv)
#    define CPU RISCV64
#  else
#    define CPU 0
#  endif
#endif
#ifndef WORD_LENGTH
#  if UINTPTR_MAX == 0xFFFFFFFFu
#    define WORD_LENGTH 32
#  else
#    define WORD_LENGTH 64
#  endif
#endif
/* ---- end of config.h ---- */

void init(void)
{
#if (CPU == X86_64)
    /* x86-64 init code */
    printf("init: x86-64 init code\n");
#endif
#if (CPU == ARM64)
    /* ARM64 init code */
    printf("init: ARM64 init code\n");
#endif
#if (CPU == RISCV64)
    /* RISC-V init code */
    printf("init: RISC-V init code\n");
#endif
#if (CPU == 0)
    printf("init: unknown CPU\n");
#endif
}

#if (WORD_LENGTH == 32)
typedef int Register;
#elif (WORD_LENGTH == 64)
typedef long Register;
#else  /* today: portable */
typedef uintptr_t Register;
#endif
Register R0, R1, R2, R3;

int main(void)
{
    init();
    R0 = 1;
    R1 = R0 << 20;
    printf("WORD_LENGTH = %d, sizeof(Register) = %zu, R1 = %ld\n",
           WORD_LENGTH, sizeof(Register), (long)R1);
    return 0;
}

/* Expected output (x86-64 Linux):
 * init: x86-64 init code
 * WORD_LENGTH = 64, sizeof(Register) = 8, R1 = 1048576
 */
