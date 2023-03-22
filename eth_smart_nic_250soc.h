#ifndef ETH_SMART_NIC_250SOC_H
#define ETH_SMART_NIC_250SOC_H

#include <linux/types.h>
#include <linux/device.h>
#include <linux/netdevice.h>

char xtnet_driver_name[] = "xtnet_eth";
/* VENDOR_ID 0x10ee DEVICE_ID 0x903f */

#define xtnet_core_err(__dev, format, ...)			\
	dev_err((__dev)->device, "%s:%d:(pid %d): " format,	\
		__func__, __LINE__, current->pid,		\
	       ##__VA_ARGS__)

#define PCI_VENDOR_ID_XTIC 0x1057
#define PCI_DEVICE_ID_XTIC 0x0004

#define BAR_0 0

enum xtnet_pci_status {
	XTNET_PCI_STATUS_DISABLED,
	XTNET_PCI_STATUS_ENABLED,
};

enum xtnet_device_state {
	XTNET_DEVICE_STATE_UP = 1,
	XTNET_DEVICE_STATE_INTERNAL_ERROR,
};


struct xtnet_core_dev {
    /* bar地址 */
    phys_addr_t     bar_addr;
    /* 映射后的bar地址 */
    u8 __iomem      *hw_addr;
    long            range;
    /* xtnet设备状态 */
    enum xtnet_device_state     state;
    /* 绑定的PCI设备 */
    struct pci_dev      *pdev;
    /* PCI设备状态 */
    enum xtnet_pci_status       pci_status;
    /* 设备对象 */
    struct device       *device;
    /* 网络设备 */
    struct net_device   *netdev;
};

/*
 * register write
 */
static inline void xtnet_iow(struct xtnet_core_dev *lp, off_t offset,
			       u32 value)
{
	iowrite32(value, lp->hw_addr + offset);
}

/*
 * register read
 */
static inline u32 xtnet_ior(struct xtnet_core_dev *lp, off_t offset)
{
	return ioread32(lp->hw_addr + offset);
}
















#endif /* ETH_SMART_NIC_250SOC_H */