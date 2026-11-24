/* send/receive in a microkernel: the client, the file-system server (FS) and
 * the disk driver (DISK) are separate "processes" (threads here) that talk
 * only through messages carried by the kernel. Kernel mailboxes are simulated. */
#include <stdio.h>
#include <string.h>
#include <pthread.h>

/* Renamed with macros so they do not clash with send() in libc. */
#define send(dest, msg)       kernel_send((dest), (msg), sizeof *(msg))
#define receive(source, msg)  kernel_receive((source), (msg), sizeof *(msg))

enum { CLIENT, FS, DISK, PROC_COUNT, ANY = -1 };
enum { READ = 1, STOP = 2 };
static const char *name[] = {"client", "FS", "DISK"};

struct block_data { char content[24]; };
struct message {
    int type, file, offset;
    int source;                     /* filled in by the kernel */
    struct block_data result;
};

/* ---- kernel: one mailbox per process ---- */
#define BOX 8
struct mail { int source; size_t size; unsigned char data[64]; };
static struct mail box[PROC_COUNT][BOX];
static int box_count[PROC_COUNT];
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER;
static _Thread_local int me;       /* ID of the calling process */

static void kernel_send(int dest, const void *msg, size_t size)
{
    pthread_mutex_lock(&lock);
    struct mail *p = &box[dest][box_count[dest]++];
    p->source = me;
    p->size = size;
    memcpy(p->data, msg, size);
    if (size == sizeof(struct message))        /* the kernel writes m.source */
        ((struct message *)(void *)p->data)->source = me;
    printf("  send: %s -> %s\n", name[me], name[dest]);
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&lock);
}

/* If source == ANY, receives from any sender, otherwise only from that one. */
static void kernel_receive(int source, void *msg, size_t size)
{
    pthread_mutex_lock(&lock);
    for (;;) {
        for (int i = 0; i < box_count[me]; i++) {
            struct mail *p = &box[me][i];
            if (source == ANY || p->source == source) {
                memcpy(msg, p->data, size < p->size ? size : p->size);
                for (int j = i + 1; j < box_count[me]; j++)
                    box[me][j - 1] = box[me][j];
                box_count[me]--;
                pthread_mutex_unlock(&lock);
                return;
            }
        }
        pthread_cond_wait(&changed, &lock);    /* block */
    }
}

/* ---- file-system server ---- */
static int find_block(int file, int offset)
{
    static const int first_block[8] = {10, 20, 30, 40, 50, 60, 70, 120};
    return first_block[file] + offset / 4096;
}

static void *fs_server(void *arg)
{
    (void)arg;
    me = FS;
    struct message m;
    int block;
    struct block_data data;
    while (1) {
        receive(ANY, &m);
        if (m.type == STOP) {                  /* end the simulation */
            block = -1;
            send(DISK, &block);
            break;
        }
        block = find_block(m.file, m.offset);
        printf("  FS: file %d, offset %d -> block %d\n", m.file, m.offset, block);
        send(DISK, &block);
        receive(DISK, &data);
        m.result = data;
        send(m.source, &m);
    }
    return NULL;
}

/* ---- disk driver ---- */
static void *disk_driver(void *arg)
{
    (void)arg;
    me = DISK;
    int block;
    struct block_data data;
    while (1) {
        receive(FS, &block);
        if (block < 0)
            break;
        snprintf(data.content, sizeof data.content, "block-%d-data", block);
        printf("  DISK: block %d read\n", block);
        send(FS, &data);
    }
    return NULL;
}

int main(void)
{
    pthread_t fs_t, disk_t;
    pthread_create(&fs_t, NULL, fs_server, NULL);
    pthread_create(&disk_t, NULL, disk_driver, NULL);

    /* client */
    me = CLIENT;
    struct message m;
    memset(&m, 0, sizeof m);
    m.type = READ; m.file = 7; m.offset = 0;
    printf("client: READ file=7 offset=0\n");
    send(FS, &m);
    receive(FS, &m);       /* wait for the reply */
    printf("client: reply \"%s\"\n", m.result.content);

    m.type = STOP;
    send(FS, &m);
    pthread_join(fs_t, NULL);
    pthread_join(disk_t, NULL);
    return 0;
}

/* Expected output (the order is fixed because each step waits for the previous message):
 * client: READ file=7 offset=0
 *   send: client -> FS
 *   FS: file 7, offset 0 -> block 120
 *   send: FS -> DISK
 *   DISK: block 120 read
 *   send: DISK -> FS
 *   send: FS -> client
 * client: reply "block-120-data"
 *   send: client -> FS
 *   send: FS -> DISK
 */
