#include "xtic_enet.h"
#include "xtic_phy.h"
#include <linux/module.h>
#include <linux/ethtool.h>
#include <linux/pci.h>
#include <linux/of.h>
//#include <linux/dev_printk.h>
#include <linux/etherdevice.h>

static void release_bar(struct pci_dev *pdev);
void xtenet_set_mac_address(struct net_device *ndev, const void *address);
void axienet_set_multicast_list(struct net_device *ndev);
static void xxvenet_setoptions(struct net_device *ndev, u32 options);
static const struct pci_device_id xtenet_pci_tbl[] = {
    {PCI_DEVICE(PCI_VENDOR_ID_XTIC, PCI_DEVICE_ID_XTIC)},
    {0,}
};

MODULE_DEVICE_TABLE(pci, xtenet_pci_tbl);



static const struct xtnet_config axienet_10g25g_config = {
	.mactype = XAXIENET_10G_25G,
	.setoptions = xxvenet_setoptions,
	// .clk_init = xxvenet_clk_init,
	// .tx_ptplen = XXV_TX_PTP_LEN,
	// .ts_header_len = XXVENET_TS_HEADER_LEN,
};


/* Option table for setting up Axi Ethernet hardware options */
static struct xxvenet_option xxvenet_options[] = {
	{ /* Turn on FCS stripping on receive packets */
		.opt = XAE_OPTION_FCS_STRIP,
		.reg = XXV_RCW1_OFFSET,
		.m_or = XXV_RCW1_FCS_MASK,
	}, { /* Turn on FCS insertion on transmit packets */
		.opt = XAE_OPTION_FCS_INSERT,
		.reg = XXV_TC_OFFSET,
		.m_or = XXV_TC_FCS_MASK,
	}, { /* Enable transmitter */
		.opt = XAE_OPTION_TXEN,
		.reg = XXV_TC_OFFSET,
		.m_or = XXV_TC_TX_MASK,
	}, { /* Enable receiver */
		.opt = XAE_OPTION_RXEN,
		.reg = XXV_RCW1_OFFSET,
		.m_or = XXV_RCW1_RX_MASK,
	},
	{}
};

static void xxvenet_setoptions(struct net_device *ndev, u32 options)
{
	int reg;
	struct axienet_local *lp = netdev_priv(ndev);
	struct xxvenet_option *tp;

    tp = &xxvenet_options[0];

	while (tp->opt) {
		reg = ((xtenet_ior(lp, tp->reg)) & ~(tp->m_or));
		if (options & tp->opt)
			reg |= tp->m_or;
		xtenet_iow(lp, tp->reg, reg);
		tp++;
	}

	lp->options |= options;
}

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
	struct axienet_local *lp = netdev_priv(ndev);

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
static void xtnet_device_reset(struct net_device *ndev)
{
	// u32 axienet_status;
	struct axienet_local *lp = netdev_priv(ndev);
	u32 err, val;
    // struct axienet_dma_q *q;
	// u32 i;

    if (lp->xtnet_config->mactype == XAXIENET_10G_25G) {
		/* Reset the XXV MAC */
		val = xtenet_ior(lp, XXV_GT_RESET_OFFSET);
		val |= XXV_GT_RESET_MASK;
		xtenet_iow(lp, XXV_GT_RESET_OFFSET, val);
		/* Wait for 1ms for GT reset to complete as per spec */
		mdelay(1);
		val = xtenet_ior(lp, XXV_GT_RESET_OFFSET);
		val &= ~XXV_GT_RESET_MASK;
		xtenet_iow(lp, XXV_GT_RESET_OFFSET, val);
	}

    // if (!lp->is_tsn) {
    //     for_each_rx_dma_queue(lp, i) {
    //         q = lp->dq[i];
    //         __axienet_device_reset(q);
    //     }
    // }

    lp->max_frm_size = XAE_MAX_VLAN_FRAME_SIZE;

    if (lp->xtnet_config->mactype == XAXIENET_10G_25G ||
        lp->xtnet_config->mactype == XAXIENET_MRMAC) {
        lp->options |= XAE_OPTION_FCS_STRIP;
        lp->options |= XAE_OPTION_FCS_INSERT;
    }

    if ((ndev->mtu > XAE_MTU) && (ndev->mtu <= XAE_JUMBO_MTU)) {
		lp->max_frm_size = ndev->mtu + VLAN_ETH_HLEN +
					XAE_TRL_SIZE;
		if (lp->max_frm_size <= lp->rxmem &&
		    (lp->xtnet_config->mactype != XAXIENET_10G_25G &&
		     lp->xtnet_config->mactype != XAXIENET_MRMAC))
			lp->options |= XAE_OPTION_JUMBO;
	}

    if (lp->xtnet_config->mactype == XAXIENET_10G_25G) {
		/* Check for block lock bit got set or not
		 * This ensures that 10G ethernet IP
		 * is functioning normally or not.
		 */
		err = readl_poll_timeout(lp->regs + XXV_STATRX_BLKLCK_OFFSET,
					 val, (val & XXV_RX_BLKLCK_MASK),
					 10, DELAY_OF_ONE_MILLISEC);
		if (err) {
			netdev_err(ndev, "XXV MAC block lock not complete! Cross-check the MAC ref clock configuration\n");
		}
    }

    if (lp->xtnet_config->mactype == XAXIENET_10G_25G ||
	    lp->xtnet_config->mactype == XAXIENET_MRMAC) {
		lp->options |= XAE_OPTION_FCS_STRIP;
		lp->options |= XAE_OPTION_FCS_INSERT;
	}
    lp->xtnet_config->setoptions(ndev, lp->options &
				       ~(XAE_OPTION_TXEN | XAE_OPTION_RXEN));

	xtenet_set_mac_address(ndev, NULL);
	axienet_set_multicast_list(ndev);
	lp->xtnet_config->setoptions(ndev, lp->options);

	netif_trans_update(ndev);
}

static int xtenet_open(struct net_device *ndev)
{
    int ret = 0, i = 0;
    u32 reg, err;
	struct axienet_local *lp = netdev_priv(ndev);
    xt_printk("%s start\n",__func__);
    xtnet_device_reset(ndev);

    if (!lp->is_tsn) {
        /* DMA配置 */

        /* Enable NAPI scheduling before enabling Axi DMA Rx
        * IRQ, or you might run into a race condition; the RX
        * ISR disables IRQ processing before scheduling the
        * NAPI function to complete the processing. If NAPI
        * scheduling is (still) disabled at that time, no more
        * RX IRQs will be processed as only the NAPI function
        * re-enables them!
        */
        napi_enable(&lp->napi[i]);
    }

    if (lp->phy_mode == XXE_PHY_TYPE_USXGMII) {
        netdev_dbg(ndev, "RX reg: 0x%x\n",
			   xtenet_ior(lp, XXV_RCW1_OFFSET));
		/* USXGMII setup at selected speed */
		reg = xtenet_ior(lp, XXV_USXGMII_AN_OFFSET);
		reg &= ~USXGMII_RATE_MASK;
		netdev_dbg(ndev, "usxgmii_rate %d\n", lp->usxgmii_rate);
		switch (lp->usxgmii_rate) {
		case SPEED_1000:
			reg |= USXGMII_RATE_1G;
			break;
		case SPEED_2500:
			reg |= USXGMII_RATE_2G5;
			break;
		case SPEED_10:
			reg |= USXGMII_RATE_10M;
			break;
		case SPEED_100:
			reg |= USXGMII_RATE_100M;
			break;
		case SPEED_5000:
			reg |= USXGMII_RATE_5G;
			break;
		case SPEED_10000:
			reg |= USXGMII_RATE_10G;
			break;
		default:
			reg |= USXGMII_RATE_1G;
		}
        reg |= USXGMII_FD;
		reg |= (USXGMII_EN | USXGMII_LINK_STS);
		xtenet_iow(lp, XXV_USXGMII_AN_OFFSET, reg);
		reg |= USXGMII_AN_EN;
		xtenet_iow(lp, XXV_USXGMII_AN_OFFSET, reg);
		/* AN Restart bit should be reset, set and then reset as per
		 * spec with a 1 ms delay for a raising edge trigger
		 */
		xtenet_iow(lp, XXV_USXGMII_AN_OFFSET,
			    reg & ~USXGMII_AN_RESTART);
		mdelay(1);
		xtenet_iow(lp, XXV_USXGMII_AN_OFFSET,
			    reg | USXGMII_AN_RESTART);
		mdelay(1);
		xtenet_iow(lp, XXV_USXGMII_AN_OFFSET,
			    reg & ~USXGMII_AN_RESTART);
        /* Check block lock bit to make sure RX path is ok with
		 * USXGMII initialization.
		 */
		err = readl_poll_timeout(lp->regs + XXV_STATRX_BLKLCK_OFFSET,
					 reg, (reg & XXV_RX_BLKLCK_MASK),
					 100, DELAY_OF_ONE_MILLISEC);
		if (err) {
			netdev_err(ndev, "%s: USXGMII Block lock bit not set",
				   __func__);
			ret = -ENODEV;
			goto err_eth_irq;
		}

		err = readl_poll_timeout(lp->regs + XXV_USXGMII_AN_STS_OFFSET,
					 reg, (reg & USXGMII_AN_STS_COMP_MASK),
					 1000000, DELAY_OF_ONE_MILLISEC);
		if (err) {
			netdev_err(ndev, "%s: USXGMII AN not complete",
				   __func__);
			ret = -ENODEV;
			goto err_eth_irq;
		}

		netdev_info(ndev, "USXGMII setup at %d\n", lp->usxgmii_rate);
    }
    netif_tx_start_all_queues(ndev);
    xt_printk("%s end\n",__func__);
	return 0;

err_eth_irq:

    return ret;
}

static int xticenet_stop(struct net_device *ndev)
{
    xt_printk("%s start\n",__func__);

    xt_printk("%s end\n",__func__);
    return 0;
}

int axienet_queue_xmit(struct sk_buff *skb,
		       struct net_device *ndev, u16 map)
{
    	u32 ii;
	u32 num_frag;
	u32 csum_start_off;
	u32 csum_index_off;
	dma_addr_t tail_p;
	struct axienet_local *lp = netdev_priv(ndev);
	struct axidma_bd *cur_p;

	unsigned long flags;
	struct axienet_dma_q *q;

    if (lp->xtnet_config->mactype == XAXIENET_10G_25G ||
	    lp->xtnet_config->mactype == XAXIENET_MRMAC) {
		/* Need to manually pad the small frames in case of XXV MAC
		 * because the pad field is not added by the IP. We must present
		 * a packet that meets the minimum length to the IP core.
		 * When the IP core is configured to calculate and add the FCS
		 * to the packet the minimum packet length is 60 bytes.
		 */
		if (eth_skb_pad(skb)) {
			ndev->stats.tx_dropped++;
			ndev->stats.tx_errors++;
			return NETDEV_TX_OK;
		}
	}
    num_frag = skb_shinfo(skb)->nr_frags;

	q = lp->dq[map];

    return 0;
}

static int xticenet_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
    u16 map = skb_get_queue_mapping(skb); /* Single dma queue default*/
    xt_printk("%s start\n",__func__);

    xt_printk("%s end\n",__func__);
    return axienet_queue_xmit(skb, ndev, map);
}


/**
 * netdev_set_mac_address - Write the MAC address (from outside the driver)
 * @ndev:	Pointer to the net_device structure
 * @p:		6 byte Address to be written as MAC address
 *
 * Return: 0 for all conditions. Presently, there is no failure case.
 *
 * This function is called to initialize the MAC address of the Axi Ethernet
 * core. It calls the core specific xtenet_set_mac_address. This is the
 * function that goes into net_device_ops structure entry ndo_set_mac_address.
 */
static int netdev_set_mac_address(struct net_device *ndev, void *p)
{
	struct sockaddr *addr = p;
    xt_printk("%s start!\n", __func__);

	xtenet_set_mac_address(ndev, addr->sa_data);

    xt_printk("%s end!\n", __func__);
	return 0;
}
/**
 * axienet_set_multicast_list - Prepare the multicast table
 * @ndev:	Pointer to the net_device structure
 *
 * This function is called to initialize the multicast table during
 * initialization. The Axi Ethernet basic multicast support has a four-entry
 * multicast table which is initialized here. Additionally this function
 * goes into the net_device_ops structure entry ndo_set_multicast_list. This
 * means whenever the multicast table entries need to be updated this
 * function gets called.
 */
void axienet_set_multicast_list(struct net_device *ndev)
{
    int i;
	u32 reg, af0reg, af1reg;
	struct axienet_local *lp = netdev_priv(ndev);
    xt_printk("%s start\n",__func__);
    if ((lp->xtnet_config->mactype != XAXIENET_1G) || lp->eth_hasnobuf)
		return;
    xt_printk("%s end\n",__func__);

}

static int axienet_ioctl(struct net_device *ndev, struct ifreq *rq, int cmd)
{
    xt_printk("%s start\n",__func__);

    xt_printk("%s end\n",__func__);
    return 0;
}

static int xticenet_change_mtu(struct net_device *ndev, int new_mtu)
{
    struct axienet_local *lp = netdev_priv(ndev);

    xt_printk("%s start\n",__func__);
	if (netif_running(ndev))
		return -EBUSY;

	if ((new_mtu + VLAN_ETH_HLEN +
		XAE_TRL_SIZE) > lp->rxmem)
		return -EINVAL;

	ndev->mtu = new_mtu;
    xt_printk("%s end\n",__func__);
    return 0;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
/**
 * xticenet_poll_controller - Axi Ethernet poll mechanism.
 * @ndev:	Pointer to net_device structure
 *
 * This implements Rx/Tx ISR poll mechanisms. The interrupts are disabled prior
 * to polling the ISRs and are enabled back after the polling is done.
 */
static void xticenet_poll_controller(struct net_device *ndev)
{

    return;
}
#endif

/* Ioctl MII Interface */
static int xticenet_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
    if (!netif_running(dev))
        return -EINVAL;

    switch (cmd) {
    case SIOCGMIIPHY:
    case SIOCGMIIREG:
    case SIOCSMIIREG:
        if (!dev->phydev)
            return -EOPNOTSUPP;
        return phy_mii_ioctl(dev->phydev, rq, cmd);

    default:
		return -EOPNOTSUPP;
	}
}

static const struct net_device_ops xtnet_netdev_ops = {
    .ndo_open = xtenet_open,
    .ndo_stop = xticenet_stop,
    .ndo_start_xmit = xticenet_start_xmit,
    .ndo_change_mtu = xticenet_change_mtu,
    .ndo_set_mac_address = netdev_set_mac_address,
    .ndo_validate_addr = eth_validate_addr,
    .ndo_do_ioctl = xticenet_ioctl,
#ifdef CONFIG_NET_POLL_CONTROLLER
    .ndo_poll_controller = xticenet_poll_controller,
#endif
};


static void xtenet_pci_disable_device(struct axienet_local *dev)
{
    struct pci_dev *pdev = dev->pdev;
    if (dev->pci_status == XTNET_PCI_STATUS_ENABLED) {
        pci_disable_device(pdev);
        dev->pci_status = XTNET_PCI_STATUS_DISABLED;
    }
}

static void xtenet_pci_close(struct axienet_local *dev)
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
    struct axienet_local *dev  = pci_get_drvdata(pdev);

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
static int xtenet_pci_init(struct axienet_local *dev, struct pci_dev *pdev,
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
    dev->axidma_addr = dev->bar_addr + AXIDMA_1_BASE;
    dev->xdma_addr = dev->bar_addr + XDMA0_CTRL_BASE;
    dev->xxv_addr = dev->bar_addr + XXV_ETHERNET_0_BASE;
    xt_printk("bar0 = 0x%llx\n", dev->bar_addr);
    xt_printk("dev->axidma_addr = 0x%llx\n", dev->axidma_addr);
    xt_printk("dev->xdma_addr = 0x%llx\n", dev->xdma_addr);
    xt_printk("dev->xxv_addr = 0x%llx\n", dev->xxv_addr);

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
    // err = set_dma_caps(pdev);
    // if (err) {
    //     xtenet_core_err(dev, "Failed setting DMA capabilities mask, aborting\n");
    // }
    /* 映射bar0至虚拟地址空间 */
    dev->regs = pci_ioremap_bar(pdev, BAR_0);
    dev->axidma_regs = dev->regs + AXIDMA_1_BASE;
    dev->xdma_regs = dev->regs + XDMA0_CTRL_BASE;
    dev->xxv_regs = dev->regs + XXV_ETHERNET_0_BASE;
    xt_printk("dev->regs = 0x%x\n",(unsigned int)(long)dev->regs);
    xt_printk("dev->axidma_regs = 0x%x\n",(unsigned int)(long)dev->axidma_regs);
    xt_printk("dev->xdma_regs = 0x%x\n",(unsigned int)(long)dev->xdma_regs);
    xt_printk("dev->xxv_regs = 0x%x\n",(unsigned int)(long)dev->xxv_regs);
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
void xtnet_irq_deinit_pcie(struct axienet_local *dev)
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
static int xtnet_irq_init_pcie(struct axienet_local *dev)
{
    struct pci_dev *pdev = dev->pdev;
    struct net_device *ndev = dev->ndev;
    int ret = 0;
    u8 irq_;
    int k;
    // Allocate MSI IRQs
	// dev->eth_irq = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_MSI | PCI_IRQ_MSIX);
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
void xtenet_set_mac_address(struct net_device *ndev,
			     const void *address)
{
	struct axienet_local *lp = netdev_priv(ndev);

	if (address)
		ether_addr_copy(ndev->dev_addr, address);
	if (!is_valid_ether_addr(ndev->dev_addr))
		eth_hw_addr_random(ndev);
}

static int __maybe_unused axienet_dma_probe(struct pci_dev *pdev,
					    struct net_device *ndev)
{
	int i, ret;
	struct axienet_local *lp = netdev_priv(ndev);
	struct axienet_dma_q *q;
	struct device_node *np = NULL;
	struct resource dmares;

    for_each_rx_dma_queue(lp, i) {
		q = devm_kzalloc(&pdev->dev, sizeof(*q), GFP_KERNEL);
		if (!q)
			return -ENOMEM;

		/* parent */
		q->lp = lp;

		lp->dq[i] = q;
	}

    /* Find the DMA node, map the DMA registers, and decode the DMA IRQs */
	/* TODO handle error ret */
	for_each_rx_dma_queue(lp, i) {
		q = lp->dq[i];

        q->dma_regs = lp->axidma_regs;
        q->eth_hasdre = true;
        lp->dma_mask = XAE_DMA_MASK_MIN;

        /* 中断号获取 */
        // lp->dq[i]->tx_irq = ;
		// lp->dq[i]->rx_irq = ;
    }

    for_each_rx_dma_queue(lp, i) {
		struct axienet_dma_q *q = lp->dq[i];

		spin_lock_init(&q->tx_lock);
		spin_lock_init(&q->rx_lock);
	}

    for_each_rx_dma_queue(lp, i) {
		netif_napi_add(ndev, &lp->napi[i], xtenet_rx_poll,
			       XAXIENET_NAPI_WEIGHT);
	}

    return 0;
}

static int xtenet_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    int err;
    int ret = 0;
    struct axienet_local *lp;
    struct net_device *ndev;
    int txcsum;
    int rxcsum;
    u16 num_queues = XTIC_MAX_QUEUES;

    xt_printk("%s start!\n", __func__);

    /* 申请用于存放xtenet设备的空间 */
    ndev = alloc_etherdev(sizeof(struct axienet_local));
    if (!ndev) {
        xtenet_core_err(lp, "error alloc_etherdev for net_device\n");
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

    lp = netdev_priv(ndev);
    lp->ndev = ndev;
    lp->dev = &pdev->dev;
    lp->pdev = pdev;
    lp->options = XTIC_OPTION_DEFAULTS;

    lp->xtnet_config = &axienet_10g25g_config;

    lp->num_tx_queues = num_queues;
    lp->num_rx_queues = num_queues;
    lp->rx_bd_num = RX_BD_NUM_DEFAULT;
    lp->tx_bd_num = TX_BD_NUM_DEFAULT;

    err = xtenet_pci_init(lp, pdev, id);
    if (err) {
        xtenet_core_err(lp, "xtenet_pci_init failed with error code %d\n", err);
        goto xt_pci_init_err;
    }

    /* 设置校验和卸载，但如果未指定，则默认为关闭 */
    lp->features = 0;

    // txcsum = ;
    if(txcsum){
        switch(txcsum){
        case 1:
            lp->csum_offload_on_tx_path =
                XAE_FEATURE_PARTIAL_TX_CSUM;
            lp->features |= XAE_FEATURE_PARTIAL_TX_CSUM;
            /* Can checksum TCP/UDP over IPv4. */
            ndev->features |= NETIF_F_IP_CSUM | NETIF_F_SG;
        break;
        case 2:
            lp->csum_offload_on_tx_path =
                XAE_FEATURE_FULL_TX_CSUM;
            lp->features |= XAE_FEATURE_FULL_TX_CSUM;
            /* Can checksum TCP/UDP over IPv4. */
            ndev->features |= NETIF_F_IP_CSUM | NETIF_F_SG;
        break;
        default:
            lp->csum_offload_on_tx_path = XAE_NO_CSUM_OFFLOAD;
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
            lp->csum_offload_on_rx_path =
                XAE_FEATURE_PARTIAL_RX_CSUM;
            lp->features |= XAE_FEATURE_PARTIAL_RX_CSUM;
            /* Can checksum TCP/UDP over IPv4. */
            ndev->features |= NETIF_F_IP_CSUM | NETIF_F_SG;
        break;
        case 2:
            lp->csum_offload_on_rx_path =
                XAE_FEATURE_FULL_RX_CSUM;
            lp->features |= XAE_FEATURE_FULL_RX_CSUM;
            /* Can checksum TCP/UDP over IPv4. */
            ndev->features |= NETIF_F_IP_CSUM | NETIF_F_SG;
        break;
        default:
            lp->csum_offload_on_rx_path = XAE_NO_CSUM_OFFLOAD;
        break;
        }
    }

	lp->rxmem = RX_MEM;
	/** phy_mode是可选的，但未指定时不应
	 * 是一个可以改变驱动程序行为的值，因此将其设置为无效
	 * 默认值。
	 */
	lp->phy_mode = PHY_INTERFACE_MODE_NA;

	/* Set default USXGMII rate */
	lp->usxgmii_rate = SPEED_1000;

	/* Set default MRMAC rate */
	lp->mrmac_rate = SPEED_10000;

    if(!lp->is_tsn)
    {

        ret = axienet_dma_probe(pdev, ndev);

        if (ret) {
			pr_err("Getting DMA resource failed\n");
			goto free_netdev;
		}
        if (dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(lp->dma_mask)) != 0) {
			dev_warn(&pdev->dev, "default to %d-bit dma mask\n", XAE_DMA_MASK_MIN);
			if (dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(XAE_DMA_MASK_MIN)) != 0) {
				dev_err(&pdev->dev, "dma_set_mask_and_coherent failed, aborting\n");
				goto free_netdev;
			}
		}
    }

    err = xtnet_irq_init_pcie(lp);
    if (err) {
		xtenet_core_err(lp, "Failed to set up interrupts err (%i)\n", err);
		// goto fail_init_irq;
	}

    xtenet_set_mac_address(ndev, NULL);

    lp->coalesce_count_rx = XAXIDMA_DFT_RX_THRESHOLD;
	lp->coalesce_count_tx = XAXIDMA_DFT_TX_THRESHOLD;

    lp->phy_mode = PHY_INTERFACE_MODE_10GKR;
    lp->phy_interface = 0;



    strcpy(ndev->name, "eth%d");
    err = register_netdev(ndev);
	if (err) {
		xtenet_core_err(lp, "register_netdev() error (%i)\n", err);
		// axienet_mdio_teardown(pdev);
		// goto err_disable_clk;
	}




    return 0;
/* 错误处理 */
xt_err_init_one:
    xtenet_pci_close(lp);
xt_pci_init_err:
free_netdev:
	free_netdev(ndev);
    return err;
}

/* xtenet卸载函数 */
static void xtenet_remove(struct pci_dev *pdev)
{
    struct axienet_local *lp = pci_get_drvdata(pdev);
    xt_printk("%s start\n",__func__);

    unregister_netdev(lp->ndev);
    iounmap(lp->regs);
    xtenet_pci_close(lp);
    xtnet_irq_deinit_pcie(lp);
    free_netdev(lp->ndev);

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
