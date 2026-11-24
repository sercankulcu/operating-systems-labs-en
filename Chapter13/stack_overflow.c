/* Stack buffer overflow: since strcpy does not check the length, the extra bytes
 * are written into neighbouring cells of the frame and finally into the return address.
 * A real overflow is undefined behaviour, so the frame is simulated with a byte
 * array: [0..7] buffer, [8..15] saved rbp, [16..23] return address.
 * Run: ./prog [input]   (if no input is given, two examples are used) */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define FRAME 24
static unsigned char frame[FRAME + 8];      /* + 8: the simulation's own margin */
static const uint64_t RET = 0x401196;       /* the caller's return address */
static const uint64_t RBP = 0x7ffd1000;

static void print_frame(const char *title)
{
    uint64_t rbp, ret;
    memcpy(&rbp, frame + 8, 8);
    memcpy(&ret, frame + 16, 8);
    printf("  %-8s buffer=\"", title);
    for (int i = 0; i < 8; i++)
        putchar(frame[i] >= 32 && frame[i] < 127 ? frame[i] : '.');
    printf("\" rbp=0x%llx ret=0x%llx\n",
           (unsigned long long)rbp, (unsigned long long)ret);
}

/* small, deliberately buggy example */
void A(char *in)
{
    char *buffer = (char *)frame;        /* char buffer[8]; */
    memset(frame, 0, sizeof frame);
    memcpy(frame + 8, &RBP, 8);          /* call: rbp and return address */
    memcpy(frame + 16, &RET, 8);
    print_frame("before:");

    strcpy(buffer, in);   /* NO bounds check */

    print_frame("after:");
    uint64_t ret;
    memcpy(&ret, frame + 16, 8);
    if (ret == RET)
        printf("  return: returns to address 0x%llx\n", (unsigned long long)ret);
    else
        printf("  return: CPU jumps to corrupted address 0x%llx, control hijacked\n",
               (unsigned long long)ret);
    return;
}

int main(int argc, char *argv[])
{
    char short_in[] = "welcome";
    char long_in[] = "AAAAAAAABBBBBBBBCCCCCC";

    if (argc > 1) {
        if (strlen(argv[1]) >= FRAME + 8) {
            printf("for the simulation the input must be shorter than %d bytes\n", FRAME + 8);
            return 0;
        }
        A(argv[1]);           /* in: user input */
        return 0;
    }
    printf("input \"%s\" (%zu bytes):\n", short_in, strlen(short_in));
    A(short_in);
    printf("input \"%s\" (%zu bytes):\n", long_in, strlen(long_in));
    A(long_in);
    return 0;
}

/* Expected output (x86-64, little-endian):
 * input "welcome" (7 bytes):
 *   before:  buffer="........" rbp=0x7ffd1000 ret=0x401196
 *   after:   buffer="welcome." rbp=0x7ffd1000 ret=0x401196
 *   return: returns to address 0x401196
 * input "AAAAAAAABBBBBBBBCCCCCC" (22 bytes):
 *   before:  buffer="........" rbp=0x7ffd1000 ret=0x401196
 *   after:   buffer="AAAAAAAA" rbp=0x4242424242424242 ret=0x434343434343
 *   return: CPU jumps to corrupted address 0x434343434343, control hijacked
 */
