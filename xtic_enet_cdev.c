
#include "xtic_enet.h"
#include <linux/init.h>
#include <linux/module.h>

static int xtic_cdev_open(struct inode *inode, struct file *file)
{

    return 0;
}

static ssize_t xtic_cdev_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{

    return 0;
}

static ssize_t xtic_cdev_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{

    return 0;
}

static int  xtic_cdev_close(struct inode *inode, struct file *file)
{

    return 0;
}

static long xtic_cdev_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{

    return 0;
}

static struct file_operations cdev_fops = {
    .owner = THIS_MODULE,
    .open  = xtic_cdev_open,
    .write  = xtic_cdev_write,
    .read  = xtic_cdev_read,
    .release = xtic_cdev_close,
    .unlocked_ioctl = xtic_cdev_ioctl,
};

static int create_xcdev(struct pci_dev *pdev)
{

    return 0;
}

int xtic_cdev_create_interfaces(struct axienet_local *dev)
{

    create_xcdev(dev->pdev);

    return 0;
}


