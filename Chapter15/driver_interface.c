/* Driver interface: a character driver collects its entry points in a single
 * file_operations structure; cdev_init ties the structure to a cdev, and the device
 * number (major:minor) is tied to the cdev. The VFS routes every read()/write() to
 * the matching driver's .read/.write. Kernel types are simulated. */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

/* --- stand-ins for kernel types --- */
struct file_operations;
struct module { const char *name; };
static struct module this_module = { "my" };
#define THIS_MODULE (&this_module)

struct inode { unsigned major, minor; };
struct file { const struct file_operations *f_op; loff_t f_pos; };
struct vm_area_struct { void *vm_start; size_t size; };
#define POLLIN 0x001
#define MY_IOCTL_CLEAR 1

struct file_operations {
    struct module *owner;
    int     (*open)(struct inode *, struct file *);
    int     (*release)(struct inode *, struct file *);
    ssize_t (*read)(struct file *, char *, size_t, loff_t *);
    ssize_t (*write)(struct file *, const char *, size_t, loff_t *);
    long    (*unlocked_ioctl)(struct file *, unsigned int, unsigned long);
    unsigned int (*poll)(struct file *, void *);
    int     (*mmap)(struct file *, struct vm_area_struct *);
};

struct cdev { const struct file_operations *ops; unsigned major, minor; };

/* --- driver: a 64-byte memory device --- */
static char device[64];
static size_t used = 0;

static int my_open(struct inode *i, struct file *f)
{
    (void)f;
    printf("my_open %u:%u\n", i->major, i->minor);
    return 0;
}

static int my_release(struct inode *i, struct file *f)
{
    (void)i;
    (void)f;
    printf("my_release\n");
    return 0;
}

static ssize_t my_read(struct file *f, char *buf, size_t n, loff_t *pos)
{
    (void)f;
    if ((size_t)*pos >= used)
        return 0;
    if (n > used - (size_t)*pos)
        n = used - (size_t)*pos;
    memcpy(buf, device + *pos, n);      /* in reality copy_to_user */
    *pos += (loff_t)n;
    return (ssize_t)n;
}

static ssize_t my_write(struct file *f, const char *buf, size_t n, loff_t *pos)
{
    (void)f;
    if ((size_t)*pos + n > sizeof device)
        n = sizeof device - (size_t)*pos;
    memcpy(device + *pos, buf, n);      /* in reality copy_from_user */
    *pos += (loff_t)n;
    if ((size_t)*pos > used)
        used = (size_t)*pos;
    return (ssize_t)n;
}

static long my_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    (void)f;
    (void)arg;
    if (cmd == MY_IOCTL_CLEAR) {
        used = 0;
        return 0;
    }
    return -25;                         /* -ENOTTY */
}

static unsigned int my_poll(struct file *f, void *wait)
{
    (void)f;
    (void)wait;
    return used ? POLLIN : 0;
}

static int my_mmap(struct file *f, struct vm_area_struct *vma)
{
    (void)f;
    vma->vm_start = device;             /* the device memory is mapped into the process */
    vma->size = sizeof device;
    return 0;
}

static const struct file_operations my_fops = {
    .owner          = THIS_MODULE,
    .open           = my_open,
    .release        = my_release,     /* close */
    .read           = my_read,
    .write          = my_write,
    .unlocked_ioctl = my_ioctl,
    .poll           = my_poll,
    .mmap           = my_mmap,
};

/* --- character device table and VFS --- */
static struct cdev cdev;
static struct cdev *chrdevs[1];

static void cdev_init(struct cdev *cd, const struct file_operations *fops)
{
    cd->ops = fops;
}

static int cdev_add(struct cdev *cd, unsigned major, unsigned minor)
{
    cd->major = major;
    cd->minor = minor;
    chrdevs[0] = cd;
    return 0;
}

static int vfs_open(struct inode *i, struct file *f)
{
    if (!chrdevs[0] || chrdevs[0]->major != i->major || chrdevs[0]->minor != i->minor)
        return -19;                     /* -ENODEV */
    f->f_op = chrdevs[0]->ops;          /* major:minor -> fops */
    f->f_pos = 0;
    return f->f_op->open(i, f);
}

int main(void)
{
    struct inode ino = { 240, 0 };      /* /dev/my: c 240,0 */
    struct file f;
    char buf[32];

    cdev_init(&cdev, &my_fops);  /* major:minor -> fops */
    cdev_add(&cdev, 240, 0);
    printf("module: %s\n", my_fops.owner->name);

    vfs_open(&ino, &f);
    ssize_t w = f.f_op->write(&f, "welcome", 7, &f.f_pos);
    printf("write: %zd bytes\n", w);
    printf("poll: %s\n", (f.f_op->poll(&f, NULL) & POLLIN) ? "POLLIN" : "-");
    f.f_pos = 0;
    ssize_t r = f.f_op->read(&f, buf, sizeof buf, &f.f_pos);
    printf("read: %zd bytes \"%.*s\"\n", r, (int)r, buf);
    struct vm_area_struct vma;
    f.f_op->mmap(&f, &vma);
    printf("mmap: %zu bytes, first character '%c'\n", vma.size, ((char *)vma.vm_start)[0]);
    printf("ioctl(CLEAR) = %ld\n", f.f_op->unlocked_ioctl(&f, MY_IOCTL_CLEAR, 0));
    printf("poll: %s\n", (f.f_op->poll(&f, NULL) & POLLIN) ? "POLLIN" : "-");
    f.f_op->release(&ino, &f);
    return 0;
}

/* Expected output:
 * module: my
 * my_open 240:0
 * write: 7 bytes
 * poll: POLLIN
 * read: 7 bytes "welcome"
 * mmap: 64 bytes, first character 'w'
 * ioctl(CLEAR) = 0
 * poll: -
 * my_release
 */
