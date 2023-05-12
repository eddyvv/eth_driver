
#ifndef XT_ROCE_H
#define XT_ROCE_H

#include <linux/pci.h>
#include <linux/netdevice.h>

#define XT_ROCE_ABI_VERSION	1



struct xilinx_ib_dev;

struct xib_dev_info {
    u8 __iomem *xib_regAddr;
    u32 xib_regLen;
    int xib_irq;
    u8 mac_addr[ETH_ALEN];
    struct pci_dev *pdev;
    struct axienet_local *ethdev;
	struct net_device *netdev;
};


struct xib_driver {
    unsigned char name[32];
    u32 xt_abi_version;
    int (*add) (struct xib_dev_info *, struct xilinx_ib_dev *);
    void (*remove) (struct xilinx_ib_dev *);
    void (*state_change_handler) (struct xilinx_ib_dev *, u32 new_state);
};

enum xt_roce_event {
	XT_DEV_SHUTDOWN = 2
};


int xt_roce_register_driver(struct xib_driver *drv);
void xt_roce_unregister_driver(struct xib_driver *drv);
#endif

