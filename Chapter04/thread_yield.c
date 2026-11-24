/* User-level thread_yield: the thread library saves the running thread's PC
 * and SP in its table, picks the next READY thread and loads its values.
 * There is no entry into the kernel. The CPU registers are imitated with the
 * global variables PC and SP. */
#include <stdio.h>

enum { READY, RUNNING, WAITING };
static const char *state_name[] = {"READY", "RUNNING", "WAITING"};

struct tcb { int pc, sp, state; } T[3];
int cur = 0;               /* running */
int PC, SP;                /* "CPU registers" */

void thread_yield(void)    /* in library */
{
    T[cur].pc = PC;  T[cur].sp = SP;
    T[cur].state = READY;
    do cur = (cur + 1) % 3;
    while (T[cur].state != READY);
    T[cur].state = RUNNING;
    PC = T[cur].pc;  SP = T[cur].sp;
}

static void print_state(const char *event)
{
    printf("%-18s cur=%d PC=0x%X SP=0x%X  [T0 %s, T1 %s, T2 %s]\n",
           event, cur, (unsigned)PC, (unsigned)SP,
           state_name[T[0].state], state_name[T[1].state], state_name[T[2].state]);
}

int main(void)
{
    /* T0 is running, T1 is waiting for a lock, T2 is ready. */
    T[0].state = RUNNING;
    T[1].pc = 0x300; T[1].sp = 0xA000; T[1].state = WAITING;
    T[2].pc = 0x480; T[2].sp = 0xB000; T[2].state = READY;
    cur = 0; PC = 0x124; SP = 0x8FF0;
    print_state("start");

    thread_yield();            /* T0 gives up the CPU voluntarily */
    print_state("T0 yield");

    PC = 0x48C; SP = 0xAFF8;   /* T2 ran for a while */
    thread_yield();
    print_state("T2 yield");
    return 0;
}

/* Expected output:
 * start              cur=0 PC=0x124 SP=0x8FF0  [T0 RUNNING, T1 WAITING, T2 READY]
 * T0 yield           cur=2 PC=0x480 SP=0xB000  [T0 READY, T1 WAITING, T2 RUNNING]
 * T2 yield           cur=0 PC=0x124 SP=0x8FF0  [T0 RUNNING, T1 WAITING, T2 READY]
 */
