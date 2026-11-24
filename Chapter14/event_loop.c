/* Execution paradigms (b) event-driven code: external events determine the flow.
 * get_message returns the next event, switch dispatches it to a handler;
 * when QUIT arrives it returns 0 and the loop ends. The event queue is simulated
 * with an array (in reality get_message blocks the process on an empty queue). */
#include <stdio.h>

enum { QUIT = 0, KEY = 1, CLICK = 2, TIMER = 3 };   /* case 1, 2, 3 */

typedef struct { int type; int data; } msg_t;

static msg_t queue[8];
static int head = 0, tail = 0;

static void post_event(int type, int data)
{
    queue[tail].type = type;
    queue[tail].data = data;
    tail++;
}

static void init(void)
{
    printf("init: window ready, queue empty\n");
    /* the user clicks, presses a key, a timer expires, then the window is closed */
    post_event(CLICK, 0);
    post_event(KEY, 'q');
    post_event(TIMER, 500);
    post_event(QUIT, 0);
}

static int get_message(msg_t *m)
{
    if (head == tail)
        return 0;               /* simulation: finish instead of blocking on an empty queue */
    *m = queue[head++];
    return m->type != QUIT;     /* 0 for QUIT */
}

static void on_key(msg_t *m)   { printf("on_key: '%c'\n", m->data); }
static void on_click(msg_t *m) { (void)m; printf("on_click\n"); }
static void on_timer(msg_t *m) { printf("on_timer: %d ms, cursor blinks\n", m->data); }

int main(void)
{
    msg_t m;
    init();
    while (get_message(&m)) {
        switch (m.type) {
        case KEY:   on_key(&m);   break;
        case CLICK: on_click(&m); break;
        case TIMER: on_timer(&m); break;
        }
    }
    printf("QUIT: loop finished\n");
    return 0;
}

/* Expected output:
 * init: window ready, queue empty
 * on_click
 * on_key: 'q'
 * on_timer: 500 ms, cursor blinks
 * QUIT: loop finished
 */
