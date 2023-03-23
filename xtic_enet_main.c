#include "xtic_enet.h"
#include "xtic_enet_config.h"
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/of.h>
//#include <linux/dev_printk.h>
#include <linux/etherdevice.h>

static void release_bar(struct pci_dev *pdev);

static const struct pci_device_id xtenet_pci_tbl[] = {
    {PCI_DEVICE(PCI_VENDOR_ID_XTIC, PCI_DEVICE_ID_XTIC)},
    {0,}
};

MODULE_DEVICE_TABLE(pci, xtenet_pci_tbl);










static int xtenet_open(struct net_device *ndev)
{
    int ret = 0, i = 0;
    xt_printk("%s start\n",__func__);


    xt_printk("%s end\n",__func__);
    return 0;
}

static const struct net_device_ops xticenet_netdev_ops = {
// #ifdef CONFIG_XILINX_TSN
//  .ndo_open = xticenet_tsn_open,
// #else
    .ndo_open = xtenet_open,
// #endif
//  .ndo_stop = xticenet_stop,
// #ifdef CONFIG_XILINX_TSN
//  .ndo_start_xmit = xticenet_tsn_xmit,
// #else
//  .ndo_start_xmit = xticenet_start_xmit,
// #endif
//  .ndo_change_mtu = xticenet_change_mtu,
//  .ndo_set_mac_address = netdev_set_mac_address,
//  .ndo_validate_addr = eth_validate_addr,
//  .ndo_set_rx_mode = xticenet_set_multicast_list,
//  .ndo_do_ioctl = xticenet_ioctl,
// #ifdef CONFIG_NET_POLL_CONTROLLER
//  .ndo_poll_controller = xticenet_poll_controller,
// #endif
};


static void xtenet_pci_disable_device(struct xtenet_core_dev *dev)
{
    struct pci_dev *pdev = dev->pdev;

    if (dev->pci_status == XTNET_PCI_STATUS_ENABLED) {
        pci_disable_device(pdev);
        dev->pci_status = XTNET_PCI_STATUS_DISABLED;
    }
}

static void xtenet_pci_close(struct xtenet_core_dev *dev)
{
    // iounmap(dev->iseg);
    pci_clear_master(dev->pdev);
    release_bar(dev->pdev);
    xtenet_pci_disable_device(dev);
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

/*
 * 读取PCI核配置空间相应信息，配置PCI设备寄存器并读取
 */
static void skel_get_configs(struct pci_dev *pdev)
{
    uint8_t val1;
    uint16_t val2;
    uint32_t val4, val5,reg_0;
    struct xtenet_core_dev *dev  = pci_get_drvdata(pdev);

    pci_read_config_word(pdev,PCI_VENDOR_ID, &val2);
    xt_printk("vendorID:0x%x\n",val2);
    pci_read_config_word(pdev,PCI_DEVICE_ID, &val2);
    xt_printk("deviceID:0x%x\n",val2);
    pci_read_config_byte(pdev, PCI_REVISION_ID, &val1);
    xt_printk("revisionID:0x%x\n",val1);
    pci_read_config_dword(pdev,PCI_CLASS_REVISION, &val4);
    xt_printk("class:0x%x\n",val4);
    pci_read_config_dword(pdev, PCI_COMMAND, &val5);
    xt_printk("command:0x%x\n",val5);
    pci_read_config_dword(pdev,PCI_BASE_ADDRESS_0, &val5);
    xt_printk("bar0:0x%x\n",val5);

    xtenet_iow(dev, 0, 0x0);
    reg_0 = xtenet_ior(dev, 0x110c);
    xt_printk("reg_0 = 0x%x\n",reg_0);

}
/*
 PCI初始化
*/
static int xtenet_pci_init(struct xtenet_core_dev *dev, struct pci_dev *pdev,
             const struct pci_device_id *id)
{
    int err;
    int val;
    xt_printk("%s start!\n",__func__);

    pci_select_bars(pdev, IORESOURCE_MEM);
    /* 打开pci设备，成功返回0 */
    err = pci_enable_device(pdev);
    if (err){
        xtenet_core_err(dev,"Cannot enable PCI device, aborting\n");
        return err;
    }

    /* 获取bar0地址 */
    dev->bar_addr = pci_resource_start(pdev, 0);
    xt_printk("bar0 = 0x%llx\n", dev->bar_addr);

    dev->range = pci_resource_end(pdev, 0) - dev->bar_addr + 1;
    xt_printk("bar0 range = 0x%x\n", dev->range);
    /* 请求PCI资源 */
    err = request_bar(pdev);
    if (err) {
        xtenet_core_err(dev, "error requesting BARs, aborting\n");
         goto xt_err_disable;
    }

    /* 使能PCI_COMMAND_MEMORY */
    pci_read_config_dword(pdev, PCI_COMMAND, &val);
    pci_write_config_dword(pdev, PCI_COMMAND, val | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
    /* 设置PCI主控制器模式，并启用DMA传输 */
    pci_set_master(pdev);
    /* 设置PCI DMA功能 */
    err = set_dma_caps(pdev);
    if (err) {
        xtenet_core_err(dev, "Failed setting DMA capabilities mask, aborting\n");
    }
    /* 映射bar0至虚拟地址空间 */
    dev->hw_addr = pci_ioremap_bar(pdev, BAR_0);
    xt_printk("dev->hw_addr = 0x%x\n",dev->hw_addr);
    if (!dev->hw_addr){
        xtenet_core_err(dev, "Failed pci_ioremap_bar\n");
        goto xt_err_clr_master;
    }

    /* pci设备与xtenet网络设备绑定 */
    pci_set_drvdata(dev->pdev, dev);
    err = pci_save_state(pdev);
    if (err){
        xtenet_core_err(dev, "error pci_save_state\n");
        return err;
    }
    skel_get_configs(pdev);

    return 0;

xt_err_ioremap:
    iounmap(dev->hw_addr);
/* 错误处理 */
xt_err_clr_master:
    pci_clear_master(dev->pdev);
    release_bar(dev->pdev);
xt_err_disable:
    xtenet_pci_disable_device(dev);
    return err;
}

static int xtenet_init_one(struct xtenet_core_dev *dev)
{
    int err;
    dev->state = XTNET_DEVICE_STATE_UP;

    return err;
}

static int xtenet_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    int err;
    struct xtenet_core_dev *dev;
    struct net_device *ndev;
    int txcsum;
    int rxcsum;
    u16 num_queues = XTIC_MAX_QUEUES;

    xt_printk("%s start!\n", __func__);

    /* 申请用于存放xtenet设备的空间 */
    ndev = alloc_etherdev(sizeof(struct xtenet_core_dev));
    if (!ndev) {
        xtenet_core_err(dev, "error alloc_etherdev for net_device\n");
        return -ENOMEM;
    }

    /* 清除多播 */
    ndev->flags &= ~IFF_MULTICAST;
    ndev->features = NETIF_F_SG;
    //ndev->netdev_ops = &xticenet_netdev_ops;
    //ndev->ethtool_ops = &xticenet_ethtool_ops;
    ndev->min_mtu = 64;
    ndev->max_mtu = XTIC_NET_JUMBO_MTU;

    /* 将PCI设备与dev绑定 */
    SET_NETDEV_DEV(ndev, &pdev->dev);

    dev = netdev_priv(ndev);
    dev->ndev = ndev;
    dev->pdev = pdev;
    dev->options = XTIC_OPTION_DEFAULTS;
    
    dev->num_tx_queues = num_queues;
    dev->num_rx_queues = num_queues;
    dev->rx_bd_num = RX_BD_NUM_DEFAULT;
    dev->tx_bd_num = TX_BD_NUM_DEFAULT;

    err = xtenet_pci_init(dev, pdev, id);
    if (err) {
        xtenet_core_err(dev, "xtenet_pci_init failed with error code %d\n", err);
        goto xt_pci_init_err;
    }

    /* 设置校验和卸载，但如果未指定，则默认为关闭 */
    dev->features = 0;

    // txcsum = ;
    if(txcsum){
        switch(txcsum){
        case 1:
            dev->csum_offload_on_tx_path =
                XAE_FEATURE_PARTIAL_TX_CSUM;
            dev->features |= XAE_FEATURE_PARTIAL_TX_CSUM;
            /* Can checksum TCP/UDP over IPv4. */
            ndev->features |= NETIF_F_IP_CSUM | NETIF_F_SG;
        break;
        case 2:
            dev->csum_offload_on_tx_path =
                XAE_FEATURE_FULL_TX_CSUM;
            dev->features |= XAE_FEATURE_FULL_TX_CSUM;
            /* Can checksum TCP/UDP over IPv4. */
            ndev->features |= NETIF_F_IP_CSUM | NETIF_F_SG;
        break;
        default:
            dev->csum_offload_on_tx_path = XAE_NO_CSUM_OFFLOAD;
        break;
        }
    }
    // rxcsum = ;
    if(rxcsum){
	/* 为了支持巨型框架，Axi以太网硬件必须具有
	 * 更大的Rx/Tx内存。通常，尺寸必须很大，以便
	 * 我们可以启用巨型选项，并开始支持巨型框架。
	 * 在这里，我们从以下位置检查为硬件中的Rx/Tx分配的内存
	 * 设备树，并相应地设置标志。
	 */
        switch(rxcsum){
        case 1:
            dev->csum_offload_on_rx_path =
                XAE_FEATURE_PARTIAL_RX_CSUM;
            dev->features |= XAE_FEATURE_PARTIAL_RX_CSUM;
            /* Can checksum TCP/UDP over IPv4. */
            ndev->features |= NETIF_F_IP_CSUM | NETIF_F_SG;
        break;
        case 2:
            dev->csum_offload_on_rx_path =
                XAE_FEATURE_FULL_RX_CSUM;
            dev->features |= XAE_FEATURE_FULL_RX_CSUM;
            /* Can checksum TCP/UDP over IPv4. */
            ndev->features |= NETIF_F_IP_CSUM | NETIF_F_SG;
        break;
        default:
            dev->csum_offload_on_rx_path = XAE_NO_CSUM_OFFLOAD;
        break;
        }
    }

	dev->rxmem = RX_MEM;
	/* phy_mode是可选的，但未指定时不应
	 * 是一个可以改变驱动程序行为的值，因此将其设置为无效
	 * 默认值。
	 */
	dev->phy_mode = PHY_INTERFACE_MODE_NA;

	/* Set default USXGMII rate */
	dev->usxgmii_rate = SPEED_1000;
	
	/* Set default MRMAC rate */
	dev->mrmac_rate = SPEED_10000;
	
	
	
    // err = xtenet_init_one(dev);
    // if (err) {
    //  xtenet_core_err(dev, "xtenet_init_one failed with error code %d\n", err);
    //  goto xt_err_init_one;
    // }
    return 0;
/* 错误处理 */
xt_err_init_one:
    xtenet_pci_close(dev);
xt_pci_init_err:

    return err;
}

/* xtenet卸载函数 */
static void xtenet_remove(struct pci_dev *pdev)
{
    struct xtenet_core_dev *dev = pci_get_drvdata(pdev);

    xt_printk("%s\n",__func__);
    iounmap(dev->hw_addr);
    xtenet_pci_close(dev);
    free_netdev(dev->ndev);
}

static struct pci_driver xtenet_driver = {
    .name     = xtenet_driver_name,
    .id_table   = xtenet_pci_tbl,
    .probe      = xtenet_probe,
    .remove     = xtenet_remove,
};

static int __init xtenet_init_module(void)
{
    int ret;
    xt_printk("%s\n",__func__);
    ret = pci_register_driver(&xtenet_driver);
    xt_printk("ret = 0x%x\n",ret);
    return ret;
}

static void __exit xtenet_exit_module(void)
{
    xt_printk("%s\n",__func__);
    pci_unregister_driver(&xtenet_driver);
}

/* 驱动注册与卸载入口 */
module_init(xtenet_init_module);
module_exit(xtenet_exit_module);

MODULE_DESCRIPTION("XTIC 25Gbps Ethernet driver");
MODULE_AUTHOR("XTIC Corporation,<xtic@xtic.com>");
MODULE_ALIAS("platform:xtnet");
MODULE_LICENSE("GPL");
