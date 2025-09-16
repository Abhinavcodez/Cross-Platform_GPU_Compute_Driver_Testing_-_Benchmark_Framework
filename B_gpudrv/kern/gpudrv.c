#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include "../include/gpudrv_ioctl.h"

#define DEVICE_NAME "gpudrv"

static dev_t dev_num;
static struct cdev gpudrv_cdev;
static struct class *gpudrv_class;

static long gpudrv_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
    case GPUDRV_IOC_RUN_CPU:
        pr_info("gpudrv: running CPU mode\n");
        break;
    case GPUDRV_IOC_RUN_GPU:
        pr_info("gpudrv: running GPU mode\n");
        break;
    case GPUDRV_IOC_RUN_HYBRID:
        pr_info("gpudrv: running Hybrid mode\n");
        break;
    default:
        return -ENOTTY;
    }
    return 0;
}

static int gpudrv_open(struct inode *inode, struct file *file) {
    pr_info("gpudrv: opened\n");
    return 0;
}

static int gpudrv_release(struct inode *inode, struct file *file) {
    pr_info("gpudrv: closed\n");
    return 0;
}

static struct file_operations fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = gpudrv_ioctl,
    .open           = gpudrv_open,
    .release        = gpudrv_release,
};

static int __init gpudrv_init(void) {
    alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    cdev_init(&gpudrv_cdev, &fops);
    cdev_add(&gpudrv_cdev, dev_num, 1);

    gpudrv_class = class_create(THIS_MODULE, DEVICE_NAME);
    device_create(gpudrv_class, NULL, dev_num, NULL, DEVICE_NAME);

    pr_info("gpudrv loaded, major=%d\n", MAJOR(dev_num));
    return 0;
}

static void __exit gpudrv_exit(void) {
    device_destroy(gpudrv_class, dev_num);
    class_destroy(gpudrv_class);
    cdev_del(&gpudrv_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("gpudrv unloaded\n");
}

module_init(gpudrv_init);
module_exit(gpudrv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("GPU Driver Benchmark Example");