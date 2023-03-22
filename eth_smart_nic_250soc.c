#include "eth_smart_nic_250soc.h"
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/of.h>
//#include <linux/dev_printk.h>
#include <linux/etherdevice.h>

static void release_bar(struct pci_dev *pdev);

static const struct pci_device_id xtnet_pci_tbl[] = {
    {PCI_DEVICE(PCI_VENDOR_ID_XTIC, PCI_DEVICE_ID_XTIC)},
    {0,}
};

MODULE_DEVICE_TABLE(pci, xtnet_pci_tbl);

static void xtnet_pci_disable_device(struct xtnet_core_dev *dev)
{
	struct pci_dev *pdev = dev->pdev;

	if (dev->pci_status == XTNET_PCI_STATUS_ENABLED) {
		pci_disable_device(pdev);
		dev->pci_status = XTNET_PCI_STATUS_DISABLED;
	}
}

static void xtnet_pci_close(struct xtnet_core_dev *dev)
{
	// iounmap(dev->iseg);
	pci_clear_master(dev->pdev);
	release_bar(dev->pdev);
	xtnet_pci_disable_device(dev);
}

/*
 * 设置DMA能力
 */
static int set_dma_caps(struct pci_dev *pdev)
{
	int err;

    /* 设置PCI设备DMA位宽以及DMA内存是否连续 */
	err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (err) {
		dev_warn(&pdev->dev, "Warning: couldn't set 64-bit PCI DMA mask\n");
		err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
		if (err) {
			dev_err(&pdev->dev, "Can't set PCI DMA mask, aborting\n");
			return err;
		}
	}
    /* 设置DMA传输最大段 */
	dma_set_max_seg_size(&pdev->dev, 2u * 1024 * 1024 * 1024);
	return err;
}

/*
 * 请求并分配bar
 */
static int request_bar(struct pci_dev *pdev)
{
	int err = 0;

	if (!(pci_resource_flags(pdev, 0) & IORESOURCE_MEM)) {
		dev_err(&pdev->dev, "Missing registers BAR, aborting\n");
		return -ENODEV;
	}

	err = pci_request_regions(pdev, KBUILD_MODNAME);
	if (err)
		dev_err(&pdev->dev, "Couldn't get PCI resources, aborting\n");

	return err;
}

static void release_bar(struct pci_dev *pdev)
{
	pci_release_regions(pdev);
}

void skel_get_configs(struct pci_dev *pdev,struct xtnet_core_dev *dev)
{
	uint8_t val1;
	uint16_t val2;
	uint32_t val4;
    uint32_t val5;

	pci_read_config_word(pdev,PCI_VENDOR_ID, &val2);
	printk("vendorID:0x%x\n",val2);
	pci_read_config_word(pdev,PCI_DEVICE_ID, &val2);
	printk("deviceID:0x%x\n",val2);
	pci_read_config_byte(pdev, PCI_REVISION_ID, &val1);
	printk("revisionID:0x%x\n",val1);
	pci_read_config_dword(pdev,PCI_CLASS_REVISION, &val4);
	printk("class:0x%x\n",val4);
    pci_read_config_dword(pdev, PCI_COMMAND, &val5);
    printk("command:0x%x\n",val5);
    pci_read_config_dword(pdev,PCI_BASE_ADDRESS_0, &val5);
    printk("bar0:0x%x\n",val5);
    // pci_write_config_dword(pdev, PCI_BASE_ADDRESS_0, dev->bar_addr);
    // pci_read_config_dword(pdev,PCI_BASE_ADDRESS_0, &val5);
    // printk("bar0:0x%x\n",val5);
}

/*
 PCI初始化
*/
static int xtnet_pci_init(struct xtnet_core_dev *dev, struct pci_dev *pdev,
			 const struct pci_device_id *id)
{
    int err;
    int reg_0;
    int val;
    printk("%s start!\n",__func__);

    pci_select_bars(pdev, IORESOURCE_MEM);
    /* 打开pci设备，成功返回0 */
    err = pci_enable_device(pdev);
    if (err){
        xtnet_core_err(dev,"Cannot enable PCI device, aborting\n");
        return err;
    }

    pci_read_config_dword(pdev, PCI_COMMAND, &val);
    pci_write_config_dword(pdev, PCI_COMMAND, val | PCI_COMMAND_MEMORY);

        /* 获取bar0地址 */
    dev->bar_addr = pci_resource_start(pdev, 0);
    printk("bar0 = 0x%x\n", dev->bar_addr);

    dev->range = pci_resource_end(pdev, 0) - dev->bar_addr + 1;
    printk("bar0 range = 0x%x\n", dev->range);
    /* 请求PCI资源 */
    err = request_bar(pdev);
	if (err) {
		xtnet_core_err(dev, "error requesting BARs, aborting\n");
		 goto xt_err_disable;
	}

    /* 设置PCI主控制器模式，并启用DMA传输 */
    pci_set_master(pdev);
    /* 设置PCI DMA功能 */
    err = set_dma_caps(pdev);
	if (err) {
		xtnet_core_err(dev, "Failed setting DMA capabilities mask, aborting\n");
	}
    /* 映射bar0至虚拟地址空间 */
    dev->hw_addr = pci_ioremap_bar(pdev, BAR_0);
    printk("dev->hw_addr = 0x%x\n",dev->hw_addr);
    if (!dev->hw_addr){
        xtnet_core_err(dev, "Failed pci_ioremap_bar\n");
        goto xt_err_clr_master;
    }

    /* pci设备与xtnet网络设备绑定 */
    pci_set_drvdata(dev->pdev, dev);
    err = pci_save_state(pdev);
	if (err){
		xtnet_core_err(dev, "error pci_save_state\n");
        return err;
    }
    skel_get_configs(pdev,dev);
    xtnet_iow(dev, 0, 0x0);
    reg_0 = xtnet_ior(dev, 0x110c);
    printk("reg_0 = 0x%x\n",reg_0);
    return 0;

xt_err_ioremap:
    iounmap(dev->hw_addr);
/* 错误处理 */
xt_err_clr_master:
	pci_clear_master(dev->pdev);
	release_bar(dev->pdev);
xt_err_disable:
	xtnet_pci_disable_device(dev);
	return err;
}

int xtnet_init_one(struct xtnet_core_dev *dev)
{
    int err;
    dev->state = XTNET_DEVICE_STATE_UP;

// /* 错误处理 */
// xt_err_function:
// 	dev->state = XTNET_DEVICE_STATE_INTERNAL_ERROR;

    return err;
}

static int xtnet_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    int err;
    struct xtnet_core_dev *dev;
    struct net_device *netdev;

    printk("%s start!\n", __func__);
    /* 申请用于存放xtnet设备的空间 */
    netdev = alloc_etherdev(sizeof(struct xtnet_core_dev));
    if (!netdev) {
        xtnet_core_err(dev, "error alloc_etherdev for net_device\n");
        return -ENOMEM;
    }

    /* 将PCI设备与dev绑定 */
    SET_NETDEV_DEV(netdev, &pdev->dev);

    dev = netdev_priv(netdev);
    dev->netdev = netdev;
    dev->pdev = pdev;

    err = xtnet_pci_init(dev, pdev, id);
    if (err) {
		xtnet_core_err(dev, "xtnet_pci_init failed with error code %d\n", err);
		goto xt_pci_init_err;
	}

    err = xtnet_init_one(dev);
	if (err) {
		xtnet_core_err(dev, "xtnet_init_one failed with error code %d\n", err);
		goto xt_err_init_one;
	}
    return 0;
/* 错误处理 */
xt_err_init_one:
	xtnet_pci_close(dev);
xt_pci_init_err:

    return err;

}

static void xtnet_remove(struct pci_dev *pdev)
{
    struct xtnet_core_dev *dev  = pci_get_drvdata(pdev);
    printk("%s\n",__func__);
    iounmap(dev->hw_addr);
    xtnet_pci_close(dev);
    free_netdev(dev->netdev);
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
    printk("%s\n",__func__);
    ret = pci_register_driver(&xtnet_driver);
    printk("ret = 0x%x\n",ret);
    return ret;
}

static void __exit xtnet_exit_module(void)
{
    printk("%s\n",__func__);
	pci_unregister_driver(&xtnet_driver);
}

/* 驱动注册与卸载入口 */
module_init(xtnet_init_module);
module_exit(xtnet_exit_module);

MODULE_DESCRIPTION("XTIC 25Gbps Ethernet driver");
MODULE_AUTHOR("XTIC Corporation,<xtnetic@xtnetic.com>");
MODULE_ALIAS("platform:xtnet");
MODULE_LICENSE("GPL");
