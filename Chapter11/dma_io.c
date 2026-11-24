/* I/O with DMA: the system call programs the DMA controller and blocks the
 * process; DMA moves the whole buffer to the printer without the CPU and raises a
 * single interrupt at the end. The DMA controller is simulated as a separate thread. */
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

static char buffer[64];          /* kernel buffer: DMA reads from here */
static int dma_count;            /* number of bytes DMA will transfer */
static int interrupt_count = 0;

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t wake = PTHREAD_COND_INITIALIZER;
static int blocked = 0;
static pthread_t dma;

static void dma_interrupt(void);

/* --- DMA controller: copies byte by byte to the printer, never touches the CPU --- */
static void *dma_run(void *arg)
{
    (void)arg;
    for (int k = 0; k < dma_count; k++) {
        struct timespec ts = { 0, 1000000 };   /* 1 ms per character */
        nanosleep(&ts, NULL);
        putchar(buffer[k]);                     /* write to the printer port */
    }
    putchar('\n');
    fflush(stdout);
    dma_interrupt();                            /* job done: a single interrupt */
    return NULL;
}

static void copy_from_user(char *dest, const char *src, int n)
{
    memcpy(dest, src, (size_t)n);
}

static void set_up_DMA_controller(void)
{
    /* buffer address, count and target port are programmed, the transfer starts */
    pthread_create(&dma, NULL, dma_run, NULL);
}

static void scheduler(void)
{
    pthread_mutex_lock(&m);
    while (blocked)
        pthread_cond_wait(&wake, &m);     /* process is blocked */
    pthread_mutex_unlock(&m);
}

static void acknowledge_interrupt(void)
{
    interrupt_count++;
}

static void unblock_user(void)
{
    pthread_mutex_lock(&m);
    blocked = 0;
    pthread_cond_signal(&wake);
    pthread_mutex_unlock(&m);
}

static void return_from_interrupt(void)
{
    /* in reality: iret */
}

/* --- interrupt service procedure --- */
static void dma_interrupt(void)
{
    acknowledge_interrupt();
    unblock_user();
    return_from_interrupt();
}

/* --- system call --- */
static void system_call(const char *p, int count)
{
    blocked = 1;
    dma_count = count;
    copy_from_user(buffer, p, count);
    set_up_DMA_controller();
    scheduler();     /* process is blocked */

    pthread_join(dma, NULL);
    printf("%d characters, %d interrupt\n", count, interrupt_count);
}

int main(void)
{
    system_call("ABC", 3);
    return 0;
}

/* Expected output:
 * ABC
 * 3 characters, 1 interrupt
 */
