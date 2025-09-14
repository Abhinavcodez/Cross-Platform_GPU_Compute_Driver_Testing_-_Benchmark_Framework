// gpudrv.c (simplified prototype)
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/list.h>

#define DEVICE_NAME "gpudrv"
#define CLASS_NAME "gpudrvcls"
#define GPUDRV_IOC_MAGIC 'G'
#define GPUDRV_IOC_COMPLETE _IOW(GPUDRV_IOC_MAGIC, 1, unsigned long)

static int major;
static struct class *gpudrv_class;
static struct device *gpudrv_device;

struct buf_entry {
    struct list_head list;
    size_t len;
    char data[]; // flexible
};

static LIST_HEAD(submit_queue);
static LIST_HEAD(result_queue);
static DECLARE_WAIT_QUEUE_HEAD(submit_wq);
static DECLARE_WAIT_QUEUE_HEAD(result_wq);
static DEFINE_MUTEX(queue_lock);

static ssize_t gpudrv_write(struct file *f, const char __user *buf, size_t count, loff_t *off) {
    // expecting header: first 8 bytes = length (uint64)
    if (count < sizeof(uint64_t)) return -EINVAL;
    uint64_t len;
    if (copy_from_user(&len, buf, sizeof(len))) return -EFAULT;
    if (len > 16*1024*1024) return -EINVAL; // safety 16MB cap
    if (count < sizeof(len) + len) return -EINVAL;
    // allocate entry
    struct buf_entry *e = kmalloc(sizeof(*e) + len, GFP_KERNEL);
    if (!e) return -ENOMEM;
    e->len = len;
    if (copy_from_user(e->data, buf + sizeof(len), len)) { kfree(e); return -EFAULT; }
    mutex_lock(&queue_lock);
    list_add_tail(&e->list, &submit_queue);
    mutex_unlock(&queue_lock);
    wake_up(&submit_wq);
    return count;
}

static ssize_t gpudrv_read(struct file *f, char __user *buf, size_t count, loff_t *off) {
    // daemon calls read to get next submission; or client calls read to get processed result
    struct buf_entry *e = NULL;
    // first check result queue (if user called read as client)
    mutex_lock(&queue_lock);
    if (!list_empty(&result_queue)) {
        e = list_first_entry(&result_queue, struct buf_entry, list);
        list_del(&e->list);
        mutex_unlock(&queue_lock);
        // return header+payload: header len (u64) + data
        size_t total = sizeof(uint64_t) + e->len;
        if (count < total) { kfree(e); return -EINVAL; }
        if (copy_to_user(buf, &e->len, sizeof(uint64_t))) { kfree(e); return -EFAULT; }
        if (copy_to_user(buf + sizeof(uint64_t), e->data, e->len)) { kfree(e); return -EFAULT; }
        kfree(e);
        return total;
    }
    // else wait for submit queue (for daemon)
    while (list_empty(&submit_queue)) {
        mutex_unlock(&queue_lock);
        if (wait_event_interruptible(submit_wq, !list_empty(&submit_queue))) return -ERESTARTSYS;
        mutex_lock(&queue_lock);
    }
    e = list_first_entry(&submit_queue, struct buf_entry, list);
    list_del(&e->list);
    mutex_unlock(&queue_lock);
    // return header+payload
    size_t total = sizeof(uint64_t) + e->len;
    if (count < total) { kfree(e); return -EINVAL; }
    if (copy_to_user(buf, &e->len, sizeof(uint64_t))) { kfree(e); return -EFAULT; }
    if (copy_to_user(buf + sizeof(uint64_t), e->data, e->len)) { kfree(e); return -EFAULT; }
    kfree(e);
    return total;
}

static long gpudrv_ioctl(struct file *f, unsigned int cmd, unsigned long arg) {
    if (cmd == GPUDRV_IOC_COMPLETE) {
        // user passes pointer to buffer in user-space containing header+payload to be queued as result
        // For simplicity, do nothing here — daemon will call write() to push results into result_queue
        return 0;
    }
    return -EINVAL;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = gpudrv_write,
    .read = gpudrv_read,
    .unlocked_ioctl = gpudrv_ioctl,
};

static struct cdev gpudrv_cdev;

static int __init gpudrv_init(void) {
    dev_t dev;
    alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    major = MAJOR(dev);
    cdev_init(&gpudrv_cdev, &fops);
    cdev_add(&gpudrv_cdev, dev, 1);
    gpudrv_class = class_create(THIS_MODULE, CLASS_NAME);
    gpudrv_device = device_create(gpudrv_class, NULL, dev, NULL, DEVICE_NAME);
    pr_info("gpudrv loaded, major %d\n", major);
    return 0;
}

static void __exit gpudrv_exit(void) {
    device_destroy(gpudrv_class, MKDEV(major, 0));
    class_destroy(gpudrv_class);
    cdev_del(&gpudrv_cdev);
    unregister_chrdev_region(MKDEV(major, 0), 1);
    pr_info("gpudrv unloaded\n");
}

module_init(gpudrv_init);
module_exit(gpudrv_exit);
MODULE_LICENSE("GPL");