
#include "xtic_enet.h"
#include <linux/init.h>
#include <linux/module.h>

#define READREG(pBaseAddr, offset, pbuf) \
			(*((unsigned long *)(pbuf)) =  ioread32(pBaseAddr + offset))
#define WRITEREG(pBaseAddr, offset, val) \
			iowrite32(val, pBaseAddr + offset)

extern char xtenet_driver_name[];

static int xtic_cdev_open(struct inode *inode, struct file *file)
{
    struct xtic_cdev *xcdev = NULL;

    xt_printk("%s start!\n", __func__);
    xcdev = container_of(inode->i_cdev, struct xtic_cdev, cdev);
    if(!xcdev){
        pr_err("xcdev 0x%p inode 0x%lx\n", xcdev, inode->i_ino);
		return -EINVAL;
    }

    file->private_data = xcdev;

    xt_printk("%s end!\n", __func__);
    return 0;
}

static ssize_t xtic_cdev_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    xt_printk("%s start!\n", __func__);

    xt_printk("%s end!\n", __func__);
    return 0;
}

static ssize_t xtic_cdev_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    xt_printk("%s start!\n", __func__);

    xt_printk("%s end!\n", __func__);
    return 0;
}

static int  xtic_cdev_close(struct inode *inode, struct file *file)
{
    xt_printk("%s start!\n", __func__);

    xt_printk("%s end!\n", __func__);
    return 0;
}

static long xtic_ioctrl_read(unsigned long arg, void* p)
{
    unsigned long ulTemp = 0;
    struct xtic_degug_reg_wr debug_reg;

    xt_printk("%s start!\n", __func__);
    if(0 == arg){
        pr_err("%s err arg == 0!\n", __func__);
        return -EFAULT;
    }

    memset(&debug_reg, 0x0, sizeof(struct xtic_degug_reg_wr));

    if(copy_from_user(&debug_reg, (struct xtic_degug_reg_wr *)arg, sizeof(struct xtic_degug_reg_wr))){
        pr_err("%s err copy_from_user err!\n", __func__);
        return -EFAULT;
    }

    READREG(p, debug_reg.addr, &ulTemp);
    debug_reg.data = (unsigned int)ulTemp;

    if(copy_to_user((struct xtic_degug_reg_wr *)arg, &debug_reg, sizeof(struct xtic_degug_reg_wr))){
        pr_err("%s err copy_to_user err!\n", __func__);
        return -EFAULT;
    }
    xt_printk("read reg base addr= 0x%x, offset=0x%x, value=0x%x,ulTemp=0x%lx\n",
                (unsigned int)(long)p, debug_reg.addr, debug_reg.data, ulTemp);
    xt_printk("%s end!\n", __func__);
    return 0;
}

static long xtic_ioctrl_write(unsigned long arg, void* p)
{
    struct xtic_degug_reg_wr debug_reg;

    xt_printk("%s start!\n", __func__);
    if(0 == arg){
        pr_err("%s err arg == 0!\n", __func__);
        return -EFAULT;
    }
    memset(&debug_reg, 0x0, sizeof(struct xtic_degug_reg_wr));

    if(copy_from_user(&debug_reg, (struct xtic_degug_reg_wr *)arg, sizeof(struct xtic_degug_reg_wr))){
        pr_err("%s err copy_from_user err!\n", __func__);
        return -EFAULT;
    }

    WRITEREG(p, debug_reg.addr, debug_reg.data);
    xt_printk("%s end!\n", __func__);
    return 0;
}

static long xtic_cdev_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{
    long ret;
    struct xtic_cdev *xcdev = (struct xtic_cdev *)flip->private_data;
    struct axienet_local *lp = xcdev->axidev;

    xt_printk("%s start!\n", __func__);

    switch(cmd) {
        case XILINX_IOC_READ_REG:
                ret = xtic_ioctrl_read(arg, lp->regs);
            break;
        case XILINX_IOC_WRITE_REG:
                ret = xtic_ioctrl_write(arg, lp->regs);
            break;
        default:
        	break;
    }

    xt_printk("%s end!\n", __func__);
    return ret;
}

static struct file_operations cdev_fops = {
    .owner = THIS_MODULE,
    .open  = xtic_cdev_open,
    .write  = xtic_cdev_write,
    .read  = xtic_cdev_read,
    .release = xtic_cdev_close,
    .unlocked_ioctl = xtic_cdev_ioctl,
};

static int create_xcdev(struct xtic_cdev *xcdev)
{
    int ret;
    xt_printk("%s start!\n", __func__);

    cdev_init(&xcdev->cdev, &cdev_fops);
    xt_printk("cdev_init end!\n");
    ret = cdev_add(&xcdev->cdev, xcdev->devid, 1);
    if (ret < 0){
        printk("add cdev failed\n");
        goto fail_add_cdev;
    }
    xt_printk("cdev_add end!\n");
    xt_printk("xcdev 0x%p, %u:%u, %s.\n",
		xcdev, xcdev->major, xcdev->minor, xcdev->cdev.kobj.name);

    xcdev->class = class_create(THIS_MODULE, xtenet_driver_name);
    if (IS_ERR(xcdev->class)) {
        xt_printk("class_create failed\n");
		return PTR_ERR(xcdev->class);
	}

    xcdev->device = device_create(xcdev->class, NULL, xcdev->devid, NULL, xtenet_driver_name);
    if (IS_ERR(xcdev->device)){
        printk("device create failed\n");
        ret = PTR_ERR(xcdev->device);
        goto fail_create_device;
    }
    xt_printk("%s end!\n", __func__);
    return 0;

fail_create_device:
    cdev_del(&xcdev->cdev);
fail_add_cdev:
    unregister_chrdev_region(xcdev->devid, 1);
    return ret;
}

static int destroy_xcdev(struct xtic_cdev *xcdev)
{
    xt_printk("%s start!\n", __func__);
    if(xcdev->device)
    {
        xt_printk("device_destroy start!\n");
        device_destroy(xcdev->class, xcdev->devid);
        xt_printk("device_destroy end!\n");
    }

    cdev_del(&xcdev->cdev);
    xt_printk("cdev_del end!\n");
    if (xcdev->class)
		class_destroy(xcdev->class);

    xt_printk("%s end!\n", __func__);
    return 0;
}

int xtic_cdev_create_interfaces(struct xtic_cdev *xcdev)
{
    int ret;

    xt_printk("%s start!\n", __func__);

    spin_lock_init(&xcdev->lock);
    if(!xcdev->major)
    {
        xt_printk("alloc_chrdev_region start!\n");
        ret = alloc_chrdev_region(&xcdev->devid, 0, 1, xtenet_driver_name);
        xt_printk("alloc_chrdev_region end!\n");
        if(ret)
        {
            pr_err("unable to allocate xcdev region %d.\n", ret);
			goto fail_alloc_chrdev;
        }

        xcdev->major = MAJOR(xcdev->devid);
        xcdev->minor = MINOR(xcdev->devid);
    }
    xt_printk("create_xcdev start!\n");
    ret = create_xcdev(xcdev);
    if (ret < 0) {
		pr_err("create_char_dev failed\n");
		goto fail_create_xcdev;
	}
    xt_printk("%s end!\n", __func__);
    return 0;
fail_create_xcdev:
	destroy_xcdev(xcdev);
fail_alloc_chrdev:
	return ret;
}

void xtic_cdev_destroy_interfaces(struct xtic_cdev *xcdev)
{

    destroy_xcdev(xcdev);
    if(xcdev->major)
        unregister_chrdev_region(xcdev->devid, 1);
}

static struct class *g_xdma_class;

int xtic_cdev_init(void)
{
    xt_printk("%s start!\n", __func__);
    g_xdma_class = class_create(THIS_MODULE, xtenet_driver_name);
	if (IS_ERR(g_xdma_class)) {
		pr_err("class_create failed\n");
		return -EINVAL;
	}
    xt_printk("%s end!\n", __func__);
    return 0;
}
