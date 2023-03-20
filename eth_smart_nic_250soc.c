#include <linux/netdevice.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/of.h>
#include "eth_smart_nic_250soc.h"

char xtnet_driver_name[] = "xtnet_eth";

#define PCI_VENDOR_ID_XTIC 0x8086
#define PCI_DEVICE_ID_XTIC 0x100f
static const struct pci_device_id xtnet_pci_tbl[] = {
    {PCI_DEVICE(PCI_VENDOR_ID_XTIC, PCI_DEVICE_ID_XTIC)},
};

static int xtnet_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
    printk("xtnet_probe\n");
    return 0;
}

static void xtnet_remove(struct pci_dev *pdev)
{
    printk("xtnet_remove\n");
}

static struct pci_driver xtnet_driver = {
    .name     = xtnet_driver_name,
    .id_table = xtnet_pci_tbl,
    .probe		= xtnet_probe,
    .remove		= xtnet_remove,
};

static int __init xtnet_init_module(void)
{
    int ret;
    printk("xtnet_init_module\n");
    ret = pci_register_driver(&xtnet_driver);
    printk("ret = 0x%x\n",ret);
    return ret;
}

static void __exit xtnet_exit_module(void)
{
	pci_unregister_driver(&xtnet_driver);
}

/* 驱动注册与卸载入口 */
module_init(xtnet_init_module);
module_exit(xtnet_exit_module);

MODULE_DESCRIPTION("XTIC 25Gbps Ethernet driver");
MODULE_AUTHOR("XTIC Corporation,<xtnetic@xtnetic.com>");
MODULE_ALIAS("platform:xtnet");
MODULE_LICENSE("GPL v2");
