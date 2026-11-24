/* Handshake with polling: the driver busy-waits on the BUSY bit, fills in the command
 * and data registers and then sets CMDRDY. The controller is simulated as a
 * separate thread. */
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>

/* status register bits */
#define BUSY   0x1
#define ERR    0x2
/* command register bits */
#define WRITE  0x1
#define CMDRDY 0x80

/* the controller's registers (device addresses in reality) */
static atomic_int status = BUSY;      /* at first the previous job is still running */
static atomic_int command = 0;
static atomic_int data_out = 0;

static char device[8];                /* bytes going to the device */
static int device_n = 0;
static long wait_loops = 0;           /* number of idle spins of the driver */

/* --- controller side --- */
static void *controller(void *arg)
{
    (void)arg;
    /* the previous job is finishing: BUSY = 0 */
    for (volatile int k = 0; k < 100000; k++)
        ;
    atomic_store(&status, 0);

    while (!(atomic_load(&command) & CMDRDY))
        ;                                         /* wait for CMDRDY */
    atomic_fetch_or(&status, BUSY);               /* CMDRDY seen -> BUSY = 1 */
    if (atomic_load(&command) & WRITE)            /* read the command */
        device[device_n++] = (char)atomic_load(&data_out);  /* write the byte to the device */
    atomic_store(&command, 0);                    /* CMDRDY = 0 */
    atomic_fetch_and(&status, ~(ERR | BUSY));     /* ERR = BUSY = 0 */
    return NULL;
}

int main(void)
{
    pthread_t d;
    pthread_create(&d, NULL, controller, NULL);

    /* --- driver (host) side --- */
    while (atomic_load(&status) & BUSY)
        wait_loops++;            /* busy waiting */
    atomic_store(&command, WRITE);
    atomic_store(&data_out, 'A');
    atomic_fetch_or(&command, CMDRDY);

    pthread_join(d, NULL);
    printf("written to the device: %.*s\n", device_n, device);
    printf("status = %d (ERR = 0: success)\n", atomic_load(&status));
    printf("driver %s\n", wait_loops > 0 ? "busy-waited" : "did not wait");
    return 0;
}

/* Expected output:
 * written to the device: A
 * status = 0 (ERR = 0: success)
 * driver busy-waited
 * (the number of wait loops depends on timing; if the thread starts late
 *  the last line can also be "driver did not wait") */
