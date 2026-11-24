/* Programmed I/O (PIO) with a printer: for every character the CPU polls the
 * printer by busy waiting until it is ready, then writes the character to the data
 * register. The printer is simulated as a separate thread. */
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

#define READY 0   /* data register empty: the printer can accept a new character */

/* In the simulation the status and data registers are the same cell: writing to
 * the data register makes the printer busy at once (as on real hardware). */
static atomic_int reg = READY;
static atomic_int *printer_status_reg = &reg;
static atomic_int *printer_data_register = &reg;

static char buffer[64];      /* kernel buffer */
static int to_print;         /* number of characters the printer will print */
static pthread_t printer;

/* --- printer (device) --- */
static void *printer_run(void *arg)
{
    (void)arg;
    for (int k = 0; k < to_print; k++) {
        int c;
        while ((c = atomic_load(&reg)) == READY)
            ;                                   /* wait for a character */
        struct timespec ts = { 0, 1000000 };    /* printing takes 1 ms */
        nanosleep(&ts, NULL);
        putchar(c);
        fflush(stdout);
        atomic_store(&reg, READY);              /* ready again */
    }
    return NULL;
}

static void copy_from_user(char *dest, const char *src, int n)
{
    memcpy(dest, src, (size_t)n);
}

static void return_to_user(void)
{
    pthread_join(printer, NULL);   /* simulation: wait for the printer to finish */
    putchar('\n');
}

/* PIO body of the write system call */
static void system_call(const char *p, int count)
{
    int i;
    copy_from_user(buffer, p, count);
    to_print = count;
    pthread_create(&printer, NULL, printer_run, NULL);
    for (i = 0; i < count; i++) {
        while (*printer_status_reg != READY)
            ;   /* busy waiting */
        *printer_data_register = p[i];
    }
    return_to_user();
}

int main(void)
{
    system_call("ABCD", 4);
    return 0;
}

/* Expected output:
 * ABCD
 */
