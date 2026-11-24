/* POSIX semaphores. Unnamed (sem_init): between threads of the same process, a buffer with
 * N slots using empty/full. Named (sem_open): between unrelated processes; here the forked
 * child reopens the semaphore by name and the two processes increment a counter in shared
 * memory 10000 times each. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define N 4
#define COUNT 20
#define ROUNDS 10000

/* unnamed: threads of the same process */
sem_t empty, full;
static int buffer[N], in = 0, out = 0;
static long total = 0;

static void *producer(void *arg)
{
    (void)arg;
    for (int x = 1; x <= COUNT; x++) {
        sem_wait(&empty);   buffer[in] = x; in = (in + 1) % N;   sem_post(&full);
    }
    return NULL;
}

static void *consumer(void *arg)
{
    (void)arg;
    for (int k = 0; k < COUNT; k++) {
        sem_wait(&full);  total += buffer[out]; out = (out + 1) % N;  sem_post(&empty);
    }
    return NULL;
}

static void increment(sem_t *s, volatile long *counter)
{
    for (int k = 0; k < ROUNDS; k++) {
        sem_wait(s);  /* CS */
        *counter = *counter + 1;
        sem_post(s);
    }
}

int main(void)
{
    /* 1) unnamed semaphores */
    pthread_t p, c;
    sem_init(&empty, 0, N);  /* 0: in-process */
    sem_init(&full,  0, 0);
    pthread_create(&p, NULL, producer, NULL);
    pthread_create(&c, NULL, consumer, NULL);
    pthread_join(p, NULL);
    pthread_join(c, NULL);
    sem_destroy(&empty);
    sem_destroy(&full);
    printf("unnamed: consumed total = %ld (1 + ... + %d)\n", total, COUNT);

    /* 2) named semaphore: between unrelated processes */
    sem_unlink("/os06");                 /* in case one is left from a previous run */
    sem_t *s = sem_open("/os06", O_CREAT, 0644, 1);
    if (s == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }
    void *mem = mmap(NULL, sizeof(long), PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        perror("mmap");
        sem_close(s);
        sem_unlink("/os06");
        return 1;
    }
    volatile long *counter = mem;      /* counter shared by the two processes */
    *counter = 0;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        sem_close(s);
        sem_unlink("/os06");
        munmap(mem, sizeof(long));
        return 1;
    }
    if (pid == 0) {                      /* child: opens the semaphore by name */
        sem_t *cs = sem_open("/os06", 0);
        if (cs == SEM_FAILED)
            _exit(1);
        increment(cs, counter);
        sem_close(cs);
        _exit(0);
    }
    increment(s, counter);
    int status = 0;
    waitpid(pid, &status, 0);

    printf("named: counter = %ld (2 processes x %d)\n", *counter, ROUNDS);
    printf("child exit code = %d\n", WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    sem_close(s);
    sem_unlink("/os06");
    munmap(mem, sizeof(long));
    return 0;
}

/* Expected output (same on every run):
 * unnamed: consumed total = 210 (1 + ... + 20)
 * named: counter = 20000 (2 processes x 10000)
 * child exit code = 0
 */
