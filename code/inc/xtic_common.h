#ifndef XTIC_COMMON_H_
#define XTIC_COMMON_H_

#define XTIC_DEBUG

#ifndef XTIC_DEBUG
#define xt_printk
#else
#define xt_printk printk
#endif /* XTIC_DEBUG */

#define xtenet_core_err(__dev, format, ...)         \
    dev_err((__dev)->dev, "%s:%d:(pid %d): " format, \
        __func__, __LINE__, current->pid,       \
           ##__VA_ARGS__)

#if defined(LINUX_5_15)

// #define PCI_VENDOR_ID_XTIC 0x1057
// #define PCI_DEVICE_ID_XTIC 0x0004
#define PCI_VENDOR_ID_XTIC 0x8086
#define PCI_DEVICE_ID_XTIC 0x100f
#elif defined(LINUX_5_4)
/* VENDOR_ID 0x10ee DEVICE_ID 0x903f */
#define PCI_VENDOR_ID_XTIC 0x10ee
#define PCI_DEVICE_ID_XTIC 0x9038
#define WRITE_REG
#define PRINT_REG_WR
#define DEBUG
#endif


#endif // !XTIC_COMMON_H_