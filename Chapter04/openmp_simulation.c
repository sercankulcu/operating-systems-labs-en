/* Two OpenMP directives written by hand with pthreads:
 *   #pragma omp parallel                       -> every thread runs the region
 *   #pragma omp parallel for reduction(+:s)    -> loop is split, partial sums are combined
 * The real OpenMP version needs "gcc -fopenmp"; since CI does not use that
 * flag, omp_get_thread_num() is imitated here with a _Thread_local variable.
 *
 * Original code:
 *   #include <omp.h>
 *   #pragma omp parallel
 *   printf("hello from %d\n", omp_get_thread_num());
 *   #pragma omp parallel for reduction(+:s)
 *   for (int i = 0; i < 1000; i++) { c[i] = a[i] + b[i]; s += c[i]; }
 */
#include <stdio.h>
#include <pthread.h>

#define OMP_NUM_THREADS 4

static _Thread_local int my_no;
static int omp_get_thread_num(void) { return my_no; }

double a[1000], b[1000], c[1000], s = 0;
static double partial_s[OMP_NUM_THREADS];        /* private copies for the reduction */
static char hello[OMP_NUM_THREADS][32];

/* the #pragma omp parallel region */
static void *parallel_region(void *arg)
{
    my_no = (int)(long)arg;
    snprintf(hello[my_no], sizeof hello[my_no],
             "hello from %d", omp_get_thread_num());
    return NULL;
}

/* #pragma omp parallel for reduction(+:s): static partitioning */
static void *parallel_for(void *arg)
{
    my_no = (int)(long)arg;
    int chunk = 1000 / OMP_NUM_THREADS;
    int begin = my_no * chunk, end = begin + chunk;
    double s_private = 0;                         /* s private to each thread */
    for (int i = begin; i < end; i++) {
        c[i] = a[i] + b[i];
        s_private += c[i];
    }
    partial_s[omp_get_thread_num()] = s_private;
    return NULL;
}

static void run_parallel(void *(*f)(void *))
{
    pthread_t t[OMP_NUM_THREADS];
    for (long k = 0; k < OMP_NUM_THREADS; k++)
        pthread_create(&t[k], NULL, f, (void *)k);
    for (int k = 0; k < OMP_NUM_THREADS; k++)
        pthread_join(t[k], NULL);
}

int main(void)
{
    run_parallel(parallel_region);
    for (int k = 0; k < OMP_NUM_THREADS; k++)     /* print in order */
        printf("%s\n", hello[k]);

    /* ... fill in a and b ... */
    for (int i = 0; i < 1000; i++) {
        a[i] = i;
        b[i] = 2 * i;
    }
    run_parallel(parallel_for);
    for (int k = 0; k < OMP_NUM_THREADS; k++)     /* reduction: combine */
        s += partial_s[k];
    printf("s = %.0f\n", s);
    return 0;
}

/* Expected output (in real OpenMP the order of the "hello" lines varies;
 * here they are printed in order after the join):
 * hello from 0
 * hello from 1
 * hello from 2
 * hello from 3
 * s = 1498500
 */
