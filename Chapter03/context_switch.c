/* Context switch: the running process's CPU registers are saved into its
 * PCB, and the values in the next process's PCB are loaded into the CPU.
 * The CPU registers are simulated with global variables and the process
 * table with an array of two PCBs. */
#include <stdio.h>

struct pcb { int pc, sp, r1; char state; };
struct pcb P[2];   /* process table */
int pc, sp, r1;    /* CPU registers */

void context_switch(int from, int to)
{
    P[from].pc = pc;  P[from].sp = sp;  /* save */
    P[from].r1 = r1;  P[from].state = 'R';
    pc = P[to].pc;  sp = P[to].sp;  /* load */
    r1 = P[to].r1;  P[to].state = 'X';
}

static void print_state(const char *event)
{
    printf("%-22s CPU: pc=%d sp=%d r1=%d | PCB0: %d %d %d %c | PCB1: %d %d %d %c\n",
           event, pc, sp, r1,
           P[0].pc, P[0].sp, P[0].r1, P[0].state,
           P[1].pc, P[1].sp, P[1].r1, P[1].state);
}

int main(void)
{
    /* P0 is running; P1's last state is kept in PCB1. */
    pc = 104; sp = 896; r1 = 8;
    P[0].state = 'X';
    P[1].pc = 340; P[1].sp = 500; P[1].r1 = 2; P[1].state = 'R';
    print_state("P0 running");

    /* interrupt */  context_switch(0, 1);
    print_state("context_switch(0, 1)");

    /* ... P1 runs ... */
    pc = 352; sp = 492; r1 = 5;
    print_state("P1 a few instructions");

    /* interrupt */  context_switch(1, 0);
    print_state("context_switch(1, 0)");
    return 0;
}

/* Expected output:
 * P0 running             CPU: pc=104 sp=896 r1=8 | PCB0: 0 0 0 X | PCB1: 340 500 2 R
 * context_switch(0, 1)   CPU: pc=340 sp=500 r1=2 | PCB0: 104 896 8 R | PCB1: 340 500 2 X
 * P1 a few instructions  CPU: pc=352 sp=492 r1=5 | PCB0: 104 896 8 R | PCB1: 340 500 2 X
 * context_switch(1, 0)   CPU: pc=104 sp=896 r1=8 | PCB0: 104 896 8 X | PCB1: 352 492 5 R
 */
