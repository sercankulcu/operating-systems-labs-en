/* Interrupt-driven I/O with a printer: the system call writes the first character and
 * blocks the process; the printer raises an interrupt after every character and the
 * interrupt service procedure writes the next one. The interrupt is simulated by a
 * function called from the printer thread. */
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

#define READY 0

/* status and data register are the same cell: writing data makes the printer busy */
static atomic_int reg = READY;
static atomic_int *printer_status_reg = &reg;
static atomic_int *printer_data_register = &reg;

static char buffer[64];       /* kernel buffer */
static const char *p;         /* state the interrupt procedure also sees */
static int i, count;
static int interrupt_count = 0;

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t irq_lock = PTHREAD_MUTEX_INITIALIZER; /* i, count */
static pthread_cond_t wake = PTHREAD_COND_INITIALIZER;
static int blocked = 0;
static atomic_int interrupts_enabled = 0;
static atomic_int finished = 0;
static pthread_t printer;

static void printer_interrupt(void);

/* --- printer (device): prints the character, then raises an interrupt --- */
static void *printer_run(void *arg)
{
    (void)arg;
    while (!atomic_load(&finished)) {
        int c = atomic_load(&reg);
        if (c == READY)
            continue;                         /* no character to print */
        struct timespec ts = { 0, 1000000 };  /* printing takes 1 ms */
        nanosleep(&ts, NULL);
        putchar(c);
        fflush(stdout);
        atomic_store(&reg, READY);
        if (atomic_load(&interrupts_enabled))
            printer_interrupt();              /* interrupt line */
    }
    return NULL;
}

static void copy_from_user(char *dest, const char *src, int n)
{
    memcpy(dest, src, (size_t)n);
}

static void enable_interrupts(void)
{
    atomic_store(&interrupts_enabled, 1);
}

static void scheduler(void)
{
    /* process is blocked: the CPU could do other work; here it waits until woken up */
    pthread_mutex_lock(&m);
    while (blocked)
        pthread_cond_wait(&wake, &m);
    pthread_mutex_unlock(&m);
}

static void unblock_user(void)
{
    pthread_mutex_lock(&m);
    blocked = 0;
    atomic_store(&finished, 1);
    pthread_cond_signal(&wake);
    pthread_mutex_unlock(&m);
}

static void acknowledge_interrupt(void)
{
    interrupt_count++;
}

static void return_from_interrupt(void)
{
    /* in reality iret: return to the interrupted process */
}

/* --- interrupt service procedure --- */
static void printer_interrupt(void)
{
    pthread_mutex_lock(&irq_lock);
    if (count == 0) {
        unblock_user();
    } else {
        *printer_data_register = p[i];
        count = count - 1;
        i = i + 1;
    }
    acknowledge_interrupt();
    pthread_mutex_unlock(&irq_lock);
    return_from_interrupt();
}

/* --- system call --- */
static void system_call(const char *user, int n)
{
    p = user;
    count = n;
    blocked = 1;
    pthread_create(&printer, NULL, printer_run, NULL);

    copy_from_user(buffer, p, count);
    enable_interrupts();
    while (*printer_status_reg != READY) ;
    pthread_mutex_lock(&irq_lock);         /* simulation: defer the interrupt briefly */
    *printer_data_register = p[0];
    i = 1; count = count - 1;
    pthread_mutex_unlock(&irq_lock);
    scheduler();      /* process is blocked */

    pthread_join(printer, NULL);
    printf("\n%d interrupts\n", interrupt_count);
}

int main(void)
{
    system_call("ABC", 3);
    return 0;
}

/* Expected output:
 * ABC
 * 3 interrupts
 */
