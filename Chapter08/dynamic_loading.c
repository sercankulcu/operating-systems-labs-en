/* Dynamic loading: libm is loaded at run time with dlopen and sqrt is found with dlsym.
 * Since glibc 2.34 dlopen lives in libc; -ldl is not needed.
 * Build: gcc dynamic_loading.c -o dynamic_loading */
#include <dlfcn.h>
#include <stdio.h>

int main(void)
{
    void *h = dlopen("libm.so.6", RTLD_LAZY);
    if (!h) { puts(dlerror()); return 1; }
    double (*root)(double) =
        (double (*)(double)) dlsym(h, "sqrt");
    printf("%.3f\n", root(2.0));
    dlclose(h);
    return 0;
}

/* Expected output:
 * 1.414
 */
