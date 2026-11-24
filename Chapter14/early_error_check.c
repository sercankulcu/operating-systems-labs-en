/* Check errors first: validating the parameters before taking locks and memory
 * removes the cleanup code from the error path. Kernel functions
 * (kmalloc, kfree, access_ok, lock/unlock) are imitated in user space
 * and the number of resources acquired and released is counted. */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>

#define MAX 4096
#define GFP_KERNEL 0

static pthread_mutex_t tbl_lock = PTHREAD_MUTEX_INITIALIZER;
static int lock_calls = 0, alloc_calls = 0;

static void lock(pthread_mutex_t *m)   { pthread_mutex_lock(m);   lock_calls++; }
static void unlock(pthread_mutex_t *m) { pthread_mutex_unlock(m); }

static void *kmalloc(size_t n, int flags)
{
    (void)flags;
    alloc_calls++;
    return malloc(n);
}

static void kfree(void *p) { free(p); }

/* simulation: a user address is valid if it is not NULL and the range does not wrap */
static int access_ok(const void *u, size_t n)
{
    return u != NULL && UINTPTR_MAX - (uintptr_t)u >= n;
}

/* bad: check after the resources are acquired */
static long bad(const void *u, size_t n)
{
    void *buf;
    lock(&tbl_lock);
    buf = kmalloc(n, GFP_KERNEL);
    if (n > MAX || !access_ok(u, n)) {
        kfree(buf);
        unlock(&tbl_lock);
        return -EINVAL;
    }
    /* ... work on the table ... */
    kfree(buf);
    unlock(&tbl_lock);
    return 0;
}

/* good: check first, then the resources */
static long good(const void *u, size_t n)
{
    void *buf;
    if (n > MAX || !access_ok(u, n))
        return -EINVAL; /* check first */
    buf = kmalloc(n, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;
    lock(&tbl_lock);
    /* ... work on the table ... */
    unlock(&tbl_lock);
    kfree(buf);
    return 0;
}

static void try_call(const char *name, long (*f)(const void *, size_t),
                     const void *u, size_t n)
{
    lock_calls = alloc_calls = 0;
    long r = f(u, n);
    printf("%-4s n = %5zu -> %3ld  (lock %d, kmalloc %d)\n",
           name, n, r, lock_calls, alloc_calls);
}

int main(void)
{
    char user[64] = { 0 };
    try_call("bad", bad, user, 64);
    try_call("bad", bad, user, 100000);      /* invalid: resources wasted */
    try_call("good", good, user, 64);
    try_call("good", good, user, 100000);    /* invalid: no resources taken */
    return 0;
}

/* Expected output (EINVAL = 22):
 * bad  n =    64 ->   0  (lock 1, kmalloc 1)
 * bad  n = 100000 -> -22  (lock 1, kmalloc 1)
 * good n =    64 ->   0  (lock 1, kmalloc 1)
 * good n = 100000 -> -22  (lock 0, kmalloc 0)
 */
