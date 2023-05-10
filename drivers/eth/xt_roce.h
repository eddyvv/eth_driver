
#ifndef XT_ROCE_H
#define XT_ROCE_H

#include <linux/pci.h>
#include <linux/netdevice.h>

#define XT_ROCE_ABI_VERSION	1


struct xib_dev_info {
    u64 *xib_regAddr;
    u32 *xib_regLen;
    u8 mac_addr[ETH_ALEN];
    struct pci_dev *pdev;
	struct net_device *netdev;
};


struct xib_driver {
    unsigned char name[32];
    u32 xt_abi_version;
    void (*add) (void);
    void (*remove) (void);
};




int xt_roce_register_driver(struct xib_driver *drv);
void xt_roce_unregister_driver(struct xib_driver *drv);
#endif

