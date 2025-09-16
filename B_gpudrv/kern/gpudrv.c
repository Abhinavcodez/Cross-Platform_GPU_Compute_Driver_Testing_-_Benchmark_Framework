// gpudrv.c
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include "../include/gpudrv_ioctl.h"

#define DEVICE_NAME "gpudrv"
#define CLASS_NAME "gpudrvcls"

static dev_t dev_number;
static struct class *gpudrv_class;
static struct device *gpudrv_device;
static struct cdev gpudrv_cdev;

/* current mode (0=CPU,1=GPU,2=HYBRID) */
static int current_mode = GPUDRV_MODE_CPU;

static int gpudrv_open(struct inode *inode, struct file *file)
{
    pr_info("device opened\n");
    return 0;
}

static int gpudrv_release(struct inode *inode, struct file *file)
{
    pr_info("device closed\n");
    return 0;
}

static long gpudrv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int user_mode;
    switch (cmd) {
    case GPUDRV_IOC_SET_MODE:
        if (copy_from_user(&user_mode, (int __user *)arg, sizeof(int)))
            return -EFAULT;
        if (user_mode != GPUDRV_MODE_CPU && user_mode != GPUDRV_MODE_GPU && user_mode != GPUDRV_MODE_HYBRID)
            return -EINVAL;
        current_mode = user_mode;
        if (current_mode == GPUDRV_MODE_CPU)
            pr_info("IOCTL: set mode CPU\n");
        else if (current_mode == GPUDRV_MODE_GPU)
            pr_info("IOCTL: set mode GPU\n");
        else
            pr_info("IOCTL: set mode HYBRID\n");
        return 0;

    case GPUDRV_IOC_GET_MODE:
        if (copy_to_user((int __user *)arg, &current_mode, sizeof(int)))
            return -EFAULT;
        pr_info("IOCTL: get mode -> %d\n", current_mode);
        return 0;

    default:
        return -ENOTTY;
    }
}

static const struct file_operations gpudrv_fops = {
    .owner = THIS_MODULE,
    .open = gpudrv_open,
    .release = gpudrv_release,
    .unlocked_ioctl = gpudrv_ioctl,
};

static int __init gpudrv_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret) {
        pr_err("alloc_chrdev_region failed: %d\n", ret);
        return ret;
    }

    cdev_init(&gpudrv_cdev, &gpudrv_fops);
    gpudrv_cdev.owner = THIS_MODULE;

    ret = cdev_add(&gpudrv_cdev, dev_number, 1);
    if (ret) {
        pr_err("cdev_add failed: %d\n", ret);
        unregister_chrdev_region(dev_number, 1);
        return ret;
    }

    gpudrv_class = class_create(CLASS_NAME);
    if (IS_ERR(gpudrv_class)) {
        pr_err("class_create failed\n");
        cdev_del(&gpudrv_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(gpudrv_class);
    }

    gpudrv_device = device_create(gpudrv_class, NULL, dev_number, NULL, DEVICE_NAME);
    if (IS_ERR(gpudrv_device)) {
        pr_err("device_create failed\n");
        class_destroy(gpudrv_class);
        cdev_del(&gpudrv_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(gpudrv_device);
    }

    pr_info("module loaded, device /dev/%s ready\n", DEVICE_NAME);
    return 0;
}

static void __exit gpudrv_exit(void)
{
    device_destroy(gpudrv_class, dev_number);
    class_destroy(gpudrv_class);
    cdev_del(&gpudrv_cdev);
    unregister_chrdev_region(dev_number, 1);
    pr_info("module unloaded\n");
}

module_init(gpudrv_init);
module_exit(gpudrv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("gpudrv - prototype IOCTL mode control");