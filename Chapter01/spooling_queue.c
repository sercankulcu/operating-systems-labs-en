/* Spooling queue: programs add files to a 4-slot circular queue, and the
 * spooler sends them to the printer one by one. */
#include <stdio.h>
#include <stddef.h>

char *queue[4];
int in = 0, out = 0, count = 0;

/* Printer simulation: prints the file name on the screen. */
void send_to_printer(char *file)
{
    printf("%s\n", file);
}

void enqueue(char *file)     /* programs */
{
    queue[in] = file;
    in = (in + 1) % 4;
    count++;
}

void spooler(void)
{
    while (count > 0) {
        send_to_printer(queue[out]);
        queue[out] = NULL;   /* slot freed */
        out = (out + 1) % 4;
        count--;
    }
}

static void print_state(const char *event)
{
    printf("[%s] in=%d out=%d count=%d\n", event, in, out, count);
}

int main(void)
{
    print_state("start");
    enqueue("a.pdf");
    enqueue("b.txt");
    enqueue("c.doc");
    enqueue("d.png");
    print_state("4 enqueued");
    spooler();
    print_state("spooled");
    return 0;
}

/* Expected output:
 * [start] in=0 out=0 count=0
 * [4 enqueued] in=0 out=0 count=4
 * a.pdf
 * b.txt
 * c.doc
 * d.png
 * [spooled] in=0 out=0 count=0
 */
