/* The PCB in Linux: task_struct. A small user-space simulation of the kernel
 * structure: the same field names, the kernel's doubly linked list_head
 * lists, and container_of to walk the children of a parent. */
#include <stdio.h>
#include <stddef.h>
#include <sys/types.h>

/* ---- imitation of <linux/list.h> ---- */
struct list_head { struct list_head *next, *prev; };

#define container_of(ptr, type, member) \
    ((type *)(void *)((char *)(ptr) - offsetof(type, member)))

static void INIT_LIST_HEAD(struct list_head *h)
{
    h->next = h;
    h->prev = h;
}

static void list_add_tail(struct list_head *entry, struct list_head *head)
{
    entry->prev = head->prev;
    entry->next = head;
    head->prev->next = entry;
    head->prev = entry;
}

/* ---- imitations of kernel types ---- */
struct sched_entity { unsigned long long vruntime; };
struct files_struct { int open_files; };
struct mm_struct    { unsigned long total_pages; };

#define TASK_RUNNING         0x0000
#define TASK_INTERRUPTIBLE   0x0001

struct task_struct {          /* <linux/sched.h> */
    pid_t pid;                     /* process ID */
    unsigned int __state;          /* process state */
    int prio;                      /* priority */
    struct sched_entity se;        /* CFS/EEVDF info */
    struct task_struct *parent;    /* parent process */
    struct list_head children;     /* children */
    struct list_head sibling;      /* link in the parent's children list */
    struct files_struct *files;    /* open files */
    struct mm_struct *mm;          /* address space */
    const char *comm;              /* program name */
    /* ... hundreds more fields ... */
};

static void task_create(struct task_struct *t, pid_t pid, const char *comm,
                        struct task_struct *parent, struct files_struct *f,
                        struct mm_struct *mm)
{
    t->pid = pid;
    t->__state = TASK_INTERRUPTIBLE;
    t->prio = 120;                 /* nice 0 */
    t->se.vruntime = 0;
    t->parent = parent;
    INIT_LIST_HEAD(&t->children);
    INIT_LIST_HEAD(&t->sibling);
    t->files = f;
    t->mm = mm;
    t->comm = comm;
    if (parent)
        list_add_tail(&t->sibling, &parent->children);
}

int main(void)
{
    static struct files_struct f1 = {3}, f2 = {5}, f3 = {4};
    static struct mm_struct m1 = {120}, m2 = {800}, m3 = {300};
    struct task_struct init_t, bash_t, vim_t, gcc_t;

    task_create(&init_t, 1, "systemd", NULL, &f1, &m1);
    task_create(&bash_t, 2105, "bash", &init_t, &f2, &m2);
    task_create(&vim_t, 2241, "vim", &bash_t, &f3, &m3);
    task_create(&gcc_t, 2242, "gcc", &bash_t, &f3, &m3);
    gcc_t.__state = TASK_RUNNING;

    printf("sizeof(struct task_struct) = %zu bytes (about 10 KB in the real kernel)\n",
           sizeof(struct task_struct));
    printf("bash (pid %d), parent %s (pid %d)\n",
           (int)bash_t.pid, bash_t.parent->comm, (int)bash_t.parent->pid);

    /* equivalent of list_for_each_entry(child, &bash_t.children, sibling) */
    for (struct list_head *p = bash_t.children.next; p != &bash_t.children; p = p->next) {
        struct task_struct *child = container_of(p, struct task_struct, sibling);
        printf("  child pid=%d comm=%s state=%s prio=%d open files=%d\n",
               (int)child->pid, child->comm,
               child->__state == TASK_RUNNING ? "R" : "S",
               child->prio, child->files->open_files);
    }
    return 0;
}

/* Expected output (the structure size may vary by compiler; 88 on x86-64):
 * sizeof(struct task_struct) = 88 bytes (about 10 KB in the real kernel)
 * bash (pid 2105), parent systemd (pid 1)
 *   child pid=2241 comm=vim state=S prio=120 open files=4
 *   child pid=2242 comm=gcc state=R prio=120 open files=4
 */
