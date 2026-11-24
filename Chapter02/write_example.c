/* The write() system call: writes 17 bytes to descriptor 1 (stdout).
 * The u-umlaut in "Turkiye" is two bytes in UTF-8 (\xc3\xbc), hence 17 bytes. */
#include <unistd.h>      // write()
#include <string.h>      // strlen()
#include <stdio.h>       // printf()

int main(void)
{
    const char *msg = "Hello, T\xc3\xbcrkiye!\n";
    size_t len = strlen(msg);        // 17 bytes
    ssize_t n = write(1, msg, len);  // 1: stdout
    if (n == -1)
        return 1;                    // error
    printf("len=%zu n=%zd\n", len, n);
    return 0;
}

/* Expected output:
 * Hello, Turkiye!      (u-umlaut in place of the u, as UTF-8)
 * len=17 n=17
 */
