/* Port-mapped and memory-mapped I/O: port-mapped registers are accessed with special
 * instructions (in/out -> inb/outb on Linux), memory-mapped registers with ordinary
 * loads/stores (ioremap + readl/writel). Real access needs root and hardware, so the
 * port space and the MMIO region are simulated with arrays in user space. */
#include <stdio.h>
#include <stdint.h>

/* --- port space (separate 64 KiB address space) --- */
static uint8_t port_space[0x10000];

static uint8_t inb(uint16_t port)            /* in  al, dx */
{
    return port_space[port];
}

static void outb(uint8_t value, uint16_t port) /* out dx, al */
{
    port_space[port] = value;
}

/* --- memory-mapped device: 4 KiB register region at physical 0xfebf0000 --- */
static uint32_t device_regs[1024];        /* 4096 bytes */
#define DEVICE_PHYS 0xfebf0000UL

static volatile void *ioremap(unsigned long phys, size_t size)
{
    (void)size;
    if (phys != DEVICE_PHYS)
        return NULL;
    return device_regs;               /* in reality mapped uncached into the page table */
}

static uint32_t readl(const volatile void *addr)
{
    return *(const volatile uint32_t *)addr;   /* mov eax, [rbx+0x14] */
}

static void writel(uint32_t value, volatile void *addr)
{
    *(volatile uint32_t *)addr = value;
}

int main(void)
{
    /* port: data port 0x60 of the keyboard controller */
    outb(0x1c, 0x60);                 /* simulation: the device put the scan code of the "A" key */
    uint8_t al = inb(0x60);           /* in al, 0x60 */
    printf("inb(0x60) = 0x%02x\n", al);

    /* MMIO: the register region is mapped, the register at 0x14 is read with an ordinary load */
    volatile uint8_t *rbx = ioremap(DEVICE_PHYS, 4096);
    if (!rbx)
        return 1;
    writel(0xcafe0001u, rbx + 0x14);
    uint32_t eax = readl(rbx + 0x14); /* mov eax, [rbx+0x14] */
    printf("readl(rbx + 0x14) = 0x%08x\n", (unsigned)eax);
    return 0;
}

/* Expected output:
 * inb(0x60) = 0x1c
 * readl(rbx + 0x14) = 0xcafe0001
 * On a real system the ports are listed in /proc/ioports and the MMIO regions in
 * /proc/iomem.
 */
