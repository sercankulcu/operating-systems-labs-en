/* Multithreaded web server: the dispatcher thread takes requests and hands
 * them to a free worker; the worker looks for the page in the cache and, if
 * it is missing, reads it from disk (blocking). Network and disk are
 * simulated; the infinite loops end when the requests run out. */
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define TRUE 1
#define WORKER_COUNT 2

struct request { char page[32]; int client; };   /* buf */
struct web_page { char name[32]; int found; int client; };   /* page */

/* ---- network: incoming requests ---- */
static const char *incoming_pages[] = {"b.html", "a.html"};
static int incoming_next = 0;

/* ---- cache: initially only a.html ---- */
static char cache[4][32] = {"a.html"};
static int cache_count = 1;
static pthread_mutex_t cache_lock = PTHREAD_MUTEX_INITIALIZER;

/* ---- work handoff from dispatcher to worker ---- */
static struct request work_slot;
static int work_ready = 0;
static pthread_mutex_t work_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t work_arrived = PTHREAD_COND_INITIALIZER;
static pthread_cond_t slot_empty = PTHREAD_COND_INITIALIZER;

/* ---- replies: completion order ---- */
static char replies[8][64];
static int reply_count = 0;
static pthread_mutex_t reply_lock = PTHREAD_MUTEX_INITIALIZER;

static void sleep_ms(long ms)
{
    struct timespec t = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&t, NULL);
}

/* Reads the next request from the network; returns 0 when none are left. */
static int get_next_request(struct request *buf)
{
    if (incoming_next >= 2)
        return 0;
    sleep_ms(20);                                /* between requests */
    strcpy(buf->page, incoming_pages[incoming_next]);
    buf->client = incoming_next + 1;
    incoming_next++;
    return 1;
}

static void handoff_work(struct request *buf)
{
    pthread_mutex_lock(&work_lock);
    while (work_ready)
        pthread_cond_wait(&slot_empty, &work_lock);
    work_slot = *buf;
    work_ready = 1;
    pthread_cond_signal(&work_arrived);          /* wake an idle worker */
    pthread_mutex_unlock(&work_lock);
}

static void wait_for_work(struct request *buf)
{
    pthread_mutex_lock(&work_lock);
    while (!work_ready)
        pthread_cond_wait(&work_arrived, &work_lock);
    *buf = work_slot;
    work_ready = 0;
    pthread_cond_signal(&slot_empty);
    pthread_mutex_unlock(&work_lock);
}

static void look_for_page_in_cache(struct request *buf, struct web_page *page)
{
    strcpy(page->name, buf->page);
    page->client = buf->client;
    page->found = 0;
    pthread_mutex_lock(&cache_lock);
    for (int i = 0; i < cache_count; i++)
        if (strcmp(cache[i], buf->page) == 0)
            page->found = 1;
    pthread_mutex_unlock(&cache_lock);
}

static int page_not_in_cache(struct web_page *page)
{
    return !page->found;
}

static void read_page_from_disk(struct request *buf, struct web_page *page)
{
    sleep_ms(200);                               /* disk: only this worker blocks */
    pthread_mutex_lock(&cache_lock);
    if (cache_count < 4)
        strcpy(cache[cache_count++], buf->page);
    pthread_mutex_unlock(&cache_lock);
    page->found = 1;
}

static void return_page(struct web_page *page)
{
    pthread_mutex_lock(&reply_lock);
    snprintf(replies[reply_count++], sizeof replies[0], "%.31s -> client %d",
             page->name, page->client);
    pthread_mutex_unlock(&reply_lock);
}

/* dispatcher thread */
static void *dispatcher(void *arg)
{
    (void)arg;
    struct request buf = {"", 0};
    while (TRUE) {
        if (!get_next_request(&buf))
            break;                               /* simulation: requests ran out */
        handoff_work(&buf);
    }
    for (int i = 0; i < WORKER_COUNT; i++) {     /* "stop" request to the workers */
        buf.page[0] = '\0';
        handoff_work(&buf);
    }
    return NULL;
}

/* worker thread */
static void *worker(void *arg)
{
    (void)arg;
    struct request buf = {"", 0};
    struct web_page page = {"", 0, 0};
    while (TRUE) {
        wait_for_work(&buf);
        if (buf.page[0] == '\0')
            break;
        look_for_page_in_cache(&buf, &page);
        if (page_not_in_cache(&page))
            read_page_from_disk(&buf, &page);
        return_page(&page);
    }
    return NULL;
}

int main(void)
{
    pthread_t d, w[WORKER_COUNT];
    for (int i = 0; i < WORKER_COUNT; i++)
        pthread_create(&w[i], NULL, worker, NULL);
    pthread_create(&d, NULL, dispatcher, NULL);

    pthread_join(d, NULL);
    for (int i = 0; i < WORKER_COUNT; i++)
        pthread_join(w[i], NULL);

    for (int i = 0; i < reply_count; i++)
        printf("%s\n", replies[i]);
    return 0;
}

/* Expected output (request 2 is served from the cache, request 1 from disk; the
 * order comes from the disk delay (200 ms) and may change on a heavily loaded machine):
 * a.html -> client 2
 * b.html -> client 1
 */
