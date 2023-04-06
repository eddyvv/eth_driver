#include "xtic_enet.h"
#include <linux/module.h>
#include <linux/ethtool.h>
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



static const struct xtnet_config axienet_10g25g_config = {
	.mactype = XAXIENET_10G_25G,
	// .setoptions = xxvenet_setoptions,
	// .clk_init = xxvenet_clk_init,
	// .tx_ptplen = XXV_TX_PTP_LEN,
	// .ts_header_len = XXVENET_TS_HEADER_LEN,
};

/**
 * axienet_dma_bd_init - Setup buffer descriptor rings for Axi DMA
 * @ndev:	Pointer to the net_device structure
 *
 * Return: 0, on success -ENOMEM, on failure -EINVAL, on default return
 *
 * This function is called to initialize the Rx and Tx DMA descriptor
 * rings. This initializes the descriptors with required default values
 * and is called when Axi Ethernet driver reset is called.
 */
static int axienet_dma_bd_init(struct net_device *ndev)
{
	int i, ret = -EINVAL;
	struct xtenet_core_dev *lp = netdev_priv(ndev);

#ifdef CONFIG_AXIENET_HAS_MCDMA
	for_each_tx_dma_queue(lp, i) {
		ret = axienet_mcdma_tx_q_init(ndev, lp->dq[i]);
		if (ret != 0)
			break;
	}
#endif
	for_each_rx_dma_queue(lp, i) {
#ifdef CONFIG_AXIENET_HAS_MCDMA
		ret = axienet_mcdma_rx_q_init(ndev, lp->dq[i]);
#else
		ret = axienet_dma_q_init(ndev, lp->dq[i]);
#endif
		if (ret != 0) {
			netdev_err(ndev, "%s: Failed to init DMA buf\n", __func__);
			break;
		}
	}

	return ret;
}


void __axienet_device_reset(struct axienet_dma_q *q)
{
	u32 timeout;
	/* Reset Axi DMA. This would reset Axi Ethernet core as well. The reset
	 * process of Axi DMA takes a while to complete as all pending
	 * commands/transfers will be flushed or completed during this
	 * reset process.
	 * Note that even though both TX and RX have their own reset register,
	 * they both reset the entire DMA core, so only one needs to be used.
	 */
	axienet_dma_out32(q, XAXIDMA_TX_CR_OFFSET, XAXIDMA_CR_RESET_MASK);
	timeout = DELAY_OF_ONE_MILLISEC;
	while (axienet_dma_in32(q, XAXIDMA_TX_CR_OFFSET) &
				XAXIDMA_CR_RESET_MASK) {
		udelay(1);
		if (--timeout == 0) {
			netdev_err(q->lp->ndev, "%s: DMA reset timeout!\n",
				   __func__);
			break;
		}
	}
}

/**
 * xtnet_device_reset - Reset and initialize the Axi Ethernet hardware.
 * @ndev:	Pointer to the net_device structure
 *
 * This function is called to reset and initialize the Axi Ethernet core. This
 * is typically called during initialization. It does a reset of the Axi DMA
 * Rx/Tx channels and initializes the Axi DMA BDs. Since Axi DMA reset lines
 * areconnected to Axi Ethernet reset lines, this in turn resets the Axi
 * Ethernet core. No separate hardware reset is done for the Axi Ethernet
 * core.
 */
// static void xtnet_device_reset(struct net_device *ndev)
// {
// 	u32 axienet_status;
// 	struct xtenet_core_dev *lp = netdev_priv(ndev);
// 	u32 err, val;
//     struct axienet_dma_q *q;
// 	u32 i;

//     if (lp->xtnet_config->mactype == XAXIENET_10G_25G) {
// 		/* Reset the XXV MAC */
// 		val = xtenet_ior(lp, XXV_GT_RESET_OFFSET);
// 		val |= XXV_GT_RESET_MASK;
// 		xtenet_iow(lp, XXV_GT_RESET_OFFSET, val);
// 		/* Wait for 1ms for GT reset to complete as per spec */
// 		mdelay(1);
// 		val = xtenet_ior(lp, XXV_GT_RESET_OFFSET);
// 		val &= ~XXV_GT_RESET_MASK;
// 		xtenet_iow(lp, XXV_GT_RESET_OFFSET, val);
// 	}

//     if (!lp->is_tsn) {
//         for_each_rx_dma_queue(lp, i) {
//             q = lp->dq[i];
//             __axienet_device_reset(q);
//         }
//     }

//     lp->max_frm_size = XAE_MAX_VLAN_FRAME_SIZE;

//     if (lp->xtnet_config->mactype == XAXIENET_10G_25G ||
//         lp->xtnet_config->mactype == XAXIENET_MRMAC) {
//         lp->options |= XAE_OPTION_FCS_STRIP;
//         lp->options |= XAE_OPTION_FCS_INSERT;
//     }

//     if ((ndev->mtu > XAE_MTU) && (ndev->mtu <= XAE_JUMBO_MTU)) {
// 		lp->max_frm_size = ndev->mtu + VLAN_ETH_HLEN +
// 					XAE_TRL_SIZE;
// 		if (lp->max_frm_size <= lp->rxmem &&
// 		    (lp->xtnet_config->mactype != XAXIENET_10G_25G &&
// 		     lp->xtnet_config->mactype != XAXIENET_MRMAC))
// 			lp->options |= XAE_OPTION_JUMBO;
// 	}
//     if (!lp->is_tsn) {
// 		if (axienet_dma_bd_init(ndev)) {
// 			netdev_err(ndev, "%s: descriptor allocation failed\n",
// 				   __func__);
// 		}
// 	}

//     if (lp->xtnet_config->mactype == XAXIENET_10G_25G) {
// 		/* Check for block lock bit got set or not
// 		 * This ensures that 10G ethernet IP
// 		 * is functioning normally or not.
// 		 */
// 		err = readl_poll_timeout(lp->regs + XXV_STATRX_BLKLCK_OFFSET,
// 					 val, (val & XXV_RX_BLKLCK_MASK),
// 					 10, DELAY_OF_ONE_MILLISEC);
// 		if (err) {
// 			netdev_err(ndev, "XXV MAC block lock not complete! Cross-check the MAC ref clock configuration\n");
// 		}
// #ifdef CONFIG_XILINX_AXI_EMAC_HWTSTAMP
// 		if (!lp->is_tsn) {
// 			axienet_rxts_iow(lp, XAXIFIFO_TXTS_RDFR,
// 					 XAXIFIFO_TXTS_RESET_MASK);
// 			axienet_rxts_iow(lp, XAXIFIFO_TXTS_SRR,
// 					 XAXIFIFO_TXTS_RESET_MASK);
// 			axienet_txts_iow(lp, XAXIFIFO_TXTS_RDFR,
// 					 XAXIFIFO_TXTS_RESET_MASK);
// 			axienet_txts_iow(lp, XAXIFIFO_TXTS_SRR,
// 					 XAXIFIFO_TXTS_RESET_MASK);
// 		}
// #endif
//     }

// #ifdef CONFIG_XILINX_AXI_EMAC_HWTSTAMP
// 	if (lp->xtnet_config->mactype == XAXIENET_MRMAC) {
// 		axienet_rxts_iow(lp, XAXIFIFO_TXTS_RDFR,
// 				 XAXIFIFO_TXTS_RESET_MASK);
// 		axienet_rxts_iow(lp, XAXIFIFO_TXTS_SRR,
// 				 XAXIFIFO_TXTS_RESET_MASK);
// 		axienet_txts_iow(lp, XAXIFIFO_TXTS_RDFR,
// 				 XAXIFIFO_TXTS_RESET_MASK);
// 		axienet_txts_iow(lp, XAXIFIFO_TXTS_SRR,
// 				 XAXIFIFO_TXTS_RESET_MASK);
// 	}
// #endif

//     if (lp->xtnet_config->mactype == XAXIENET_10G_25G ||
// 	    lp->xtnet_config->mactype == XAXIENET_MRMAC) {
// 		lp->options |= XAE_OPTION_FCS_STRIP;
// 		lp->options |= XAE_OPTION_FCS_INSERT;
// 	}
//     lp->xtnet_config->setoptions(ndev, lp->options &
// 				       ~(XAE_OPTION_TXEN | XAE_OPTION_RXEN));

// 	axienet_set_mac_address(ndev, NULL);
// 	axienet_set_multicast_list(ndev);
// 	lp->xtnet_config->setoptions(ndev, lp->options);

// 	netif_trans_update(ndev);
// }

static int xtenet_open(struct net_device *ndev)
{
    int ret = 0, i = 0;
	struct xtenet_core_dev *lp = netdev_priv(ndev);
    xt_printk("%s start\n",__func__);
    // xtnet_device_reset(ndev);

    xt_printk("%s end\n",__func__);
    return 0;
}

static int xticenet_stop(struct net_device *ndev)
{
    xt_printk("%s start\n",__func__);

    xt_printk("%s end\n",__func__);
    return 0;
}

static int xticenet_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
    xt_printk("%s start\n",__func__);

    xt_printk("%s end\n",__func__);

    return 0;
}


static int netdev_set_mac_address(struct net_device *ndev, void *p)
{
    xt_printk("%s start\n",__func__);

    xt_printk("%s end\n",__func__);
    return 0;
}

void axienet_set_multicast_list(struct net_device *ndev)
{
    xt_printk("%s start\n",__func__);

    xt_printk("%s end\n",__func__);

}

static int axienet_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
    xt_printk("%s start\n",__func__);

    xt_printk("%s end\n",__func__);
    return 0;
}

static int xticenet_change_mtu(struct net_device *ndev, int new_mtu)
{
    xt_printk("%s start\n",__func__);

    xt_printk("%s end\n",__func__);
    return 0;
}

static const struct net_device_ops xtnet_netdev_ops = {
// #ifdef CONFIG_XILINX_TSN
//  .ndo_open = xticenet_tsn_open,
// #else
    .ndo_open = xtenet_open,
// #endif
 .ndo_stop = xticenet_stop,
// #ifdef CONFIG_XILINX_TSN
//  .ndo_start_xmit = xticenet_tsn_xmit,
// #else
 .ndo_start_xmit = xticenet_start_xmit,
// #endif
 .ndo_change_mtu = xticenet_change_mtu,
//  .ndo_set_mac_address = netdev_set_mac_address,
 .ndo_validate_addr = eth_validate_addr,
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
    release_bar(dev->pdev);
    pci_clear_master(dev->pdev);
    xtenet_pci_disable_device(dev);
}

/**
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

/**
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

/**
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
/**
 * PCI初始化
 */
static int xtenet_pci_init(struct xtenet_core_dev *dev, struct pci_dev *pdev,
             const struct pci_device_id *id)
{
    int err;
    int val;
    xt_printk("%s start!\n",__func__);

    pci_select_bars(pdev, IORESOURCE_MEM);
    /* 打开pci设备，成功返回0 */
    err = pci_enable_device_mem(pdev);
    if (err){
        xtenet_core_err(dev,"Cannot enable PCI device, aborting\n");
        return err;
    }

    /* 获取bar0地址 */
    dev->bar_addr = pci_resource_start(pdev, 0);
    xt_printk("bar0 = 0x%llx\n", dev->bar_addr);

    dev->bar_size = pci_resource_len(pdev, 0);
    xt_printk("bar0 size = 0x%x\n", dev->bar_size);
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
    dev->regs = pci_ioremap_bar(pdev, BAR_0);
    xt_printk("dev->regs = 0x%x\n",(unsigned int)(long)dev->regs);
    if (!dev->regs){
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

// xt_err_ioremap:
    iounmap(dev->regs);
/* 错误处理 */
xt_err_clr_master:
    pci_clear_master(dev->pdev);
    release_bar(dev->pdev);
xt_err_disable:
    xtenet_pci_disable_device(dev);
    return err;
}

static irqreturn_t xtnet_irq_handler(int irqn, void *data)
{

    return IRQ_HANDLED;
}

/**
 * 释放中断处理程序
 */
void xtnet_irq_deinit_pcie(struct xtenet_core_dev *dev)
{
	struct pci_dev *pdev = dev->pdev;
    struct net_device *ndev = dev->ndev;
	int k;

	 for (k = 0; k < 1; k++)
    {
		if (dev->irq[k]) {
            free_irq(pdev->irq, ndev);
			// pci_free_irq(pdev, k, dev->irq[k]);
			kfree(dev->irq[k]);
			dev->irq[k] = NULL;
		}
	}
	pci_free_irq_vectors(pdev);
}

/**
 * NAPI轮询回调函数
 * xtenet_rx_poll - Poll routine for rx packets (NAPI)
 * @napi:	napi structure pointer
 * @quota:	Max number of rx packets to be processed.
 *
 * This is the poll routine for rx part.
 * It will process the packets maximux quota value.
 *
 * Return: number of packets received
 */
int xtenet_rx_poll(struct napi_struct *napi, int quota)
{
    int work_done = 0;

    return work_done;
}

/**
 * axienet_ethtools_set_coalesce - Set DMA interrupt coalescing count.
 * @ndev:	Pointer to net_device structure
 * @ecoalesce:	Pointer to ethtool_coalesce structure
 *
 * This implements ethtool command for setting the DMA interrupt coalescing
 * count on Tx and Rx paths. Issue "ethtool -C ethX rx-frames 5" under linux
 * prompt to execute this function.
 *
 * Return: 0, on success, Non-zero error value on failure.
 */
static int axienet_ethtools_set_coalesce(struct net_device *ndec,
                                struct ethtool_coalesce *ecoalesce,
                                struct kernel_ethtool_coalesce *kecoalesce,
                                struct netlink_ext_ack *netlink_ext_ack)
{

	return 0;
}

static const struct ethtool_ops xtnet_ethtool_ops = {
    .supported_coalesce_params = ETHTOOL_COALESCE_MAX_FRAMES,
// 	.get_drvinfo    = axienet_ethtools_get_drvinfo,
// 	.get_regs_len   = axienet_ethtools_get_regs_len,
// 	.get_regs       = axienet_ethtools_get_regs,
// 	.get_link       = ethtool_op_get_link,
// 	.get_ringparam	= axienet_ethtools_get_ringparam,
// 	.set_ringparam  = axienet_ethtools_set_ringparam,
// 	.get_pauseparam = axienet_ethtools_get_pauseparam,
// 	.set_pauseparam = axienet_ethtools_set_pauseparam,
// 	.get_coalesce   = axienet_ethtools_get_coalesce,
	// .set_coalesce   = axienet_ethtools_set_coalesce,
// 	.get_sset_count	= axienet_ethtools_sset_count,
// 	.get_ethtool_stats = axienet_ethtools_get_stats,
// 	.get_strings = axienet_ethtools_strings,
// #if defined(CONFIG_XILINX_AXI_EMAC_HWTSTAMP) || defined(CONFIG_XILINX_TSN_PTP)
// 	.get_ts_info    = axienet_ethtools_get_ts_info,
// #endif
// 	.get_link_ksettings = phy_ethtool_get_link_ksettings,
// 	.set_link_ksettings = phy_ethtool_set_link_ksettings,
};

/**
 * 初始化PCI MSI中断
 */
static int xtnet_irq_init_pcie(struct xtenet_core_dev *dev)
{
    struct pci_dev *pdev = dev->pdev;
    struct net_device *ndev = dev->ndev;
    int ret = 0;
    u8 irq_;
    int k;
    // Allocate MSI IRQs
	// dev->eth_irq = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_MSIX);
	// if (dev->eth_irq < 0) {
	// 	xtenet_core_err(dev, "Failed to allocate IRQs");
	// 	return -ENOMEM;
	// }

    // Set up interrupts
	 //for (k = 0; k < dev->eth_irq; k++)
     for (k = 0; k < 1; k++)
    {
		struct xtnet_irq *irq;

		irq = kzalloc(sizeof(*irq), GFP_KERNEL);
		if (!irq) {
			ret = -ENOMEM;
			goto fail;
		}

		ATOMIC_INIT_NOTIFIER_HEAD(&irq->nh);

		// ret = pci_request_irq(pdev, k, xtnet_irq_handler, NULL, irq, "xtnet");
        ret = request_irq(pci_irq_vector(pdev, k), xtnet_irq_handler, 0, "xtnet", ndev);
		if (ret < 0) {
			kfree(irq);
			ret = -ENOMEM;
			xtenet_core_err(dev, "Failed to request IRQ %d", k);
			goto fail;
		}

		irq->index = k;
		irq->irqn = pci_irq_vector(pdev, k);
		dev->irq[k] = irq;
	}
	return 0;
fail:
	xtnet_irq_deinit_pcie(dev);
	return ret;
}

/**
 * 配置MAC地址
 */
static void xtenet_set_mac_address(struct net_device *ndev)
{
    struct xtenet_core_dev *dev = netdev_priv(ndev);
    // set MAC
	ndev->addr_len = ETH_ALEN;

    memset(dev->mac_addr, 0, ndev->addr_len);

    /* don't block initialization here due to bad MAC address */
	memcpy(ndev->dev_addr, dev->mac_addr, ndev->addr_len);
    /* 随机生成一个MAC地址 */
    eth_hw_addr_random(ndev);

    if (!is_valid_ether_addr(ndev->dev_addr))
		xtenet_core_err(dev, "Invalid MAC Address\n");
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
    ndev->netdev_ops = &xtnet_netdev_ops;
    ndev->ethtool_ops = &xtnet_ethtool_ops;
    ndev->min_mtu = 64;
    ndev->max_mtu = XTIC_NET_JUMBO_MTU;

    /* 将PCI设备与dev绑定 */
    SET_NETDEV_DEV(ndev, &pdev->dev);

    dev = netdev_priv(ndev);
    dev->ndev = ndev;
    dev->pdev = pdev;
    dev->options = XTIC_OPTION_DEFAULTS;

    dev->xtnet_config = &axienet_10g25g_config;

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
	/** 为了支持巨型框架，Axi以太网硬件必须具有
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
	/** phy_mode是可选的，但未指定时不应
	 * 是一个可以改变驱动程序行为的值，因此将其设置为无效
	 * 默认值。
	 */
	dev->phy_mode = PHY_INTERFACE_MODE_NA;

	/* Set default USXGMII rate */
	dev->usxgmii_rate = SPEED_1000;

	/* Set default MRMAC rate */
	dev->mrmac_rate = SPEED_10000;

    netif_napi_add(ndev, &dev->napi[0], xtenet_rx_poll, XAXIENET_NAPI_WEIGHT);

    err = xtnet_irq_init_pcie(dev);
    if (err) {
		xtenet_core_err(dev, "Failed to set up interrupts err (%i)\n", err);
		// goto fail_init_irq;
	}

    xtenet_set_mac_address(ndev);

    dev->coalesce_count_rx = XAXIDMA_DFT_RX_THRESHOLD;
	dev->coalesce_count_tx = XAXIDMA_DFT_TX_THRESHOLD;

    strcpy(ndev->name, "eth%d");
    err = register_netdev(ndev);
	if (err) {
		xtenet_core_err(dev, "register_netdev() error (%i)\n", err);
		// axienet_mdio_teardown(pdev);
		// goto err_disable_clk;
	}




    return 0;
/* 错误处理 */
// xt_err_init_one:
    xtenet_pci_close(dev);
xt_pci_init_err:

    return err;
}

/* xtenet卸载函数 */
static void xtenet_remove(struct pci_dev *pdev)
{
    struct xtenet_core_dev *dev = pci_get_drvdata(pdev);
    xt_printk("%s start\n",__func__);

    unregister_netdev(dev->ndev);
    iounmap(dev->regs);
    xtenet_pci_close(dev);
    xtnet_irq_deinit_pcie(dev);
    free_netdev(dev->ndev);

    xt_printk("%s end\n",__func__);
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
MODULE_ALIAS("pci_driver:xtnet");
MODULE_LICENSE("GPL");
