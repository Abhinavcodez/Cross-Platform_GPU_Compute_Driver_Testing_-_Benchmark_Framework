// gpudrv.c
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/sched.h>
#include "gpudrv_ioctl.h"

#define DEVICE_NAME "gpudrv"
#define CLASS_NAME "gpudrvcls"
#define MAX_SUBMIT_SIZE (16 * 1024 * 1024) // 16 MB cap for prototype

static dev_t dev_number;
static struct class *gpudrv_class;
static struct device *gpudrv_device;
static struct cdev gpudrv_cdev;

/* Queue entry: holds a copy of user buffer */
struct buf_entry {
    struct list_head list;
    size_t len;
    char *data;
};

static LIST_HEAD(submit_queue);
static LIST_HEAD(result_queue);
static DECLARE_WAIT_QUEUE_HEAD(submit_wq);
static DECLARE_WAIT_QUEUE_HEAD(result_wq);
static DEFINE_MUTEX(queue_lock);

/* helper to push an entry into submit queue */
static int enqueue_submit(const char *data, size_t len) {
    struct buf_entry *e;
    e = kmalloc(sizeof(*e), GFP_KERNEL);
    if (!e) return -ENOMEM;
    e->data = kmalloc(len, GFP_KERNEL);
    if (!e->data) { kfree(e); return -ENOMEM; }
    if (copy_from_user(e->data, data, len)) { kfree(e->data); kfree(e); return -EFAULT; }
    e->len = len;
    INIT_LIST_HEAD(&e->list);
    mutex_lock(&queue_lock);
    list_add_tail(&e->list, &submit_queue);
    mutex_unlock(&queue_lock);
    wake_up_interruptible(&submit_wq);
    return 0;
}

/* helper to push result into result queue (kernel side expects user-space daemon to call write to push result) */
static int enqueue_result(const char *data, size_t len) {
    struct buf_entry *e;
    e = kmalloc(sizeof(*e), GFP_KERNEL);
    if (!e) return -ENOMEM;
    e->data = kmalloc(len, GFP_KERNEL);
    if (!e->data) { kfree(e); return -ENOMEM; }
    if (copy_from_user(e->data, data, len)) { kfree(e->data); kfree(e); return -EFAULT; }
    e->len = len;
    INIT_LIST_HEAD(&e->list);
    mutex_lock(&queue_lock);
    list_add_tail(&e->list, &result_queue);
    mutex_unlock(&queue_lock);
    wake_up_interruptible(&result_wq);
    return 0;
}

/* write(): used by client to submit header+payload OR by daemon to push processed result.
   Protocol:
   - user writes: first 8 bytes = uint64_t payload_len (host order), followed by payload_len bytes
   - kernel will assume caller intent:
       * If caller is non-daemon (no special flag) -> push into submit_queue
       * If caller intends to push result, they also call write() (we treat writes as "submit"; daemon will also use write() to push the result back)
   This simplified protocol works for prototype; in production use IDs and ioctls for clarity.
*/
static ssize_t gpudrv_write(struct file *filp, const char __user *buf, size_t count, loff_t *off) {
    uint64_t len;
    if (count < sizeof(len)) return -EINVAL;
    if (copy_from_user(&len, buf, sizeof(len))) return -EFAULT;
    if (len == 0 || len > MAX_SUBMIT_SIZE) return -EINVAL;
    if (count < sizeof(len) + len) return -EINVAL;

    /* data pointer in user buffer: buf + sizeof(len) */
    return enqueue_submit(buf + sizeof(len), len) == 0 ? (ssize_t)(sizeof(len) + len) : -EFAULT;
}

/* read(): daemon or client reads from device.
   - If result_queue has entries, return oldest result (header+payload).
   - Else if submit_queue has entries, return oldest submission (header+payload) to the daemon.
   - If neither present, block until something is available (interruptible).
*/
static ssize_t gpudrv_read(struct file *filp, char __user *buf, size_t count, loff_t *off) {
    struct buf_entry *e = NULL;
    ssize_t ret = 0;

    /* First check result queue */
    mutex_lock(&queue_lock);
    if (!list_empty(&result_queue)) {
        e = list_first_entry(&result_queue, struct buf_entry, list);
        list_del(&e->list);
        mutex_unlock(&queue_lock);
        size_t total = sizeof(uint64_t) + e->len;
        if (count < total) { ret = -EINVAL; goto out_free; }
        /* copy header */
        uint64_t len64 = e->len;
        if (copy_to_user(buf, &len64, sizeof(len64))) { ret = -EFAULT; goto out_free; }
        if (copy_to_user(buf + sizeof(len64), e->data, e->len)) { ret = -EFAULT; goto out_free; }
        ret = (ssize_t)total;
        goto out_free;
    }
    mutex_unlock(&queue_lock);

    /* If no results, wait for submits and return the next submission (daemon consumes them) */
    for (;;) {
        mutex_lock(&queue_lock);
        if (!list_empty(&submit_queue)) {
            e = list_first_entry(&submit_queue, struct buf_entry, list);
            list_del(&e->list);
            mutex_unlock(&queue_lock);
            break;
        }
        mutex_unlock(&queue_lock);
        if (wait_event_interruptible(submit_wq, !list_empty(&submit_queue)))
            return -ERESTARTSYS;
    }

    {
        size_t total = sizeof(uint64_t) + e->len;
        if (count < total) { ret = -EINVAL; goto out_free; }
        uint64_t len64 = e->len;
        if (copy_to_user(buf, &len64, sizeof(len64))) { ret = -EFAULT; goto out_free; }
        if (copy_to_user(buf + sizeof(len64), e->data, e->len)) { ret = -EFAULT; goto out_free; }
        ret = (ssize_t)total;
    }

out_free:
    if (e) {
        kfree(e->data);
        kfree(e);
    }
    return ret;
}

static long gpudrv_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
    case GPUDRV_IOC_COMPLETE:
        /* Prototype: not used in this simplified flow */
        return 0;
    default:
        return -EINVAL;
    }
}

static int gpudrv_open(struct inode *inode, struct file *file) {
    return 0;
}
static int gpudrv_release(struct inode *inode, struct file *file) {
    return 0;
}

static const struct file_operations gpudrv_fops = {
    .owner = THIS_MODULE,
    .open = gpudrv_open,
    .release = gpudrv_release,
    .write = gpudrv_write,
    .read = gpudrv_read,
    .unlocked_ioctl = gpudrv_ioctl,
};

static int __init gpudrv_init(void) {
    int ret;
    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret) { pr_err("alloc_chrdev_region failed: %d\n", ret); return ret; }
    cdev_init(&gpudrv_cdev, &gpudrv_fops);
    gpudrv_cdev.owner = THIS_MODULE;
    ret = cdev_add(&gpudrv_cdev, dev_number, 1);
    if (ret) { unregister_chrdev_region(dev_number, 1); pr_err("cdev_add failed: %d\n", ret); return ret; }
    gpudrv_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(gpudrv_class)) { cdev_del(&gpudrv_cdev); unregister_chrdev_region(dev_number, 1); pr_err("class_create failed\n"); return PTR_ERR(gpudrv_class); }
    gpudrv_device = device_create(gpudrv_class, NULL, dev_number, NULL, DEVICE_NAME);
    if (IS_ERR(gpudrv_device)) { class_destroy(gpudrv_class); cdev_del(&gpudrv_cdev); unregister_chrdev_region(dev_number, 1); pr_err("device_create failed\n"); return PTR_ERR(gpudrv_device); }

    pr_info("gpudrv loaded (%d:%d)\n", MAJOR(dev_number), MINOR(dev_number));
    return 0;
}

static void __exit gpudrv_exit(void) {
    /* free leftover entries */
    mutex_lock(&queue_lock);
    while (!list_empty(&submit_queue)) {
        struct buf_entry *e = list_first_entry(&submit_queue, struct buf_entry, list);
        list_del(&e->list);
        kfree(e->data);
        kfree(e);
    }
    while (!list_empty(&result_queue)) {
        struct buf_entry *e = list_first_entry(&result_queue, struct buf_entry, list);
        list_del(&e->list);
        kfree(e->data);
        kfree(e);
    }
    mutex_unlock(&queue_lock);

    device_destroy(gpudrv_class, dev_number);
    class_destroy(gpudrv_class);
    cdev_del(&gpudrv_cdev);
    unregister_chrdev_region(dev_number, 1);
    pr_info("gpudrv unloaded\n");
}

module_init(gpudrv_init);
module_exit(gpudrv_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Prototype gpudrv - kernel queue for user-space GPU processing");