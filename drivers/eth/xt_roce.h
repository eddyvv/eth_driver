
#ifndef XT_ROCE_H
#define XT_ROCE_H

#include <linux/pci.h>
#include <linux/netdevice.h>


struct xib_driver {
    unsigned char name[32];
    void (*probe) (void);
    void (*remove) (void);
};




int xt_roce_register_driver(void);
void xt_roce_unregister_driver(void);
#endif

