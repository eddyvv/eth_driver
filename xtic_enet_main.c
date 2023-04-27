#include "xtic_enet.h"
#include "xtic_phy.h"
#include <linux/module.h>
#include <linux/ethtool.h>
#include <linux/circ_buf.h>
#include <linux/pci.h>
#include <linux/of.h>
#include <linux/iopoll.h>
#include <linux/etherdevice.h>
char xtenet_driver_name[] = "xtenet_eth";

#define DRIVER_NAME		"xaxienet"
#define DRIVER_DESCRIPTION	"Xilinx Axi Ethernet driver"
#define DRIVER_VERSION		"1.00a"

#define AXIENET_REGS_N		40

static void release_bar(struct pci_dev *pdev);
void axienet_set_mac_address(struct net_device *ndev, const void *address);
void axienet_set_multicast_list(struct net_device *ndev);
static void xxvenet_setoptions(struct net_device *ndev, u32 options);
static const struct pci_device_id xtenet_pci_tbl[] = {
    {PCI_DEVICE(PCI_VENDOR_ID_XTIC, PCI_DEVICE_ID_XTIC)},
    {0,}
};

MODULE_DEVICE_TABLE(pci, xtenet_pci_tbl);



static const struct axienet_config axienet_10g25g_config = {
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

struct axienet_ethtools_stat {
	const char *name;
};

static struct axienet_ethtools_stat axienet_get_ethtools_strings_stats[] = {
	{ "tx_packets" },
	{ "rx_packets" },
	{ "tx_bytes" },
	{ "rx_bytes" },
	{ "tx_errors" },
	{ "rx_errors" },
};

static void xxvenet_setoptions(struct net_device *ndev, u32 options)
{
	int reg;
	struct axienet_local *lp = netdev_priv(ndev);
	struct xxvenet_option *tp;

    xt_printk("%s start\n", __func__);
    tp = &xxvenet_options[0];

	while (tp->opt) {
		reg = ((axienet_xxv_ior(lp, tp->reg)) & ~(tp->m_or));
		if (options & tp->opt)
			reg |= tp->m_or;
		axienet_xxv_iow(lp, tp->reg, reg);
		tp++;
	}

	lp->options |= options;

    xt_printk("%s end\n", __func__);
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
    xt_printk("%s start\n", __func__);
    for_each_rx_dma_queue(lp, i) {
        ret = axienet_dma_q_init(ndev, lp->dq[i]);
        if (ret != 0) {
			netdev_err(ndev, "%s: Failed to init DMA buf\n", __func__);
			break;
		}
    }
    xt_printk("%s end\n", __func__);
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
    struct axienet_dma_q *q;
	u32 i;
    xt_printk("%s start\n", __func__);
    if (lp->axienet_config->mactype == XAXIENET_10G_25G) {
		/* Reset the XXV MAC */
		val = axienet_xxv_ior(lp, XXV_GT_RESET_OFFSET);
		val |= XXV_GT_RESET_MASK;
		xt_printk("val = %x\n", val);
		axienet_xxv_iow(lp, XXV_GT_RESET_OFFSET, val);
		/* Wait for 1ms for GT reset to complete as per spec */
		mdelay(1);
		val = axienet_xxv_ior(lp, XXV_GT_RESET_OFFSET);
		xt_printk("XXV_GT_RESET_OFFSET = %x\n", val);
		val &= ~XXV_GT_RESET_MASK;
		axienet_xxv_iow(lp, XXV_GT_RESET_OFFSET, val);
	}

    if (!lp->is_tsn) {
        for_each_rx_dma_queue(lp, i) {
            q = lp->dq[i];
            __axienet_device_reset(q);
        }
    }

    lp->max_frm_size = XAE_MAX_VLAN_FRAME_SIZE;

    if (lp->axienet_config->mactype == XAXIENET_10G_25G ||
        lp->axienet_config->mactype == XAXIENET_MRMAC) {
        lp->options |= XAE_OPTION_FCS_STRIP;
        lp->options |= XAE_OPTION_FCS_INSERT;
    }

    if ((ndev->mtu > XAE_MTU) && (ndev->mtu <= XAE_JUMBO_MTU)) {
		lp->max_frm_size = ndev->mtu + VLAN_ETH_HLEN +
					XAE_TRL_SIZE;
		if (lp->max_frm_size <= lp->rxmem &&
		    (lp->axienet_config->mactype != XAXIENET_10G_25G &&
		     lp->axienet_config->mactype != XAXIENET_MRMAC))
			lp->options |= XAE_OPTION_JUMBO;
	}

    if (!lp->is_tsn) {
		if (axienet_dma_bd_init(ndev)) {
			netdev_err(ndev, "%s: descriptor allocation failed\n",
				   __func__);
		}
	}

    if (lp->axienet_config->mactype == XAXIENET_10G_25G) {
		/* Check for block lock bit got set or not
		 * This ensures that 10G ethernet IP
		 * is functioning normally or not.
		 */
#if defined(LINUX_5_4)
		err = readl_poll_timeout(lp->xxv_regs + XXV_STATRX_BLKLCK_OFFSET,
					 val, (val & XXV_RX_BLKLCK_MASK),
					 10, DELAY_OF_ONE_MILLISEC);
#endif
		if (err) {
			netdev_err(ndev, "XXV MAC block lock not complete! Cross-check the MAC ref clock configuration\n");
		}
    }

    if (lp->axienet_config->mactype == XAXIENET_10G_25G ||
	    lp->axienet_config->mactype == XAXIENET_MRMAC) {
		lp->options |= XAE_OPTION_FCS_STRIP;
		lp->options |= XAE_OPTION_FCS_INSERT;
	}
    lp->axienet_config->setoptions(ndev, lp->options &
				       ~(XAE_OPTION_TXEN | XAE_OPTION_RXEN));

	axienet_set_mac_address(ndev, NULL);
	axienet_set_multicast_list(ndev);
	lp->axienet_config->setoptions(ndev, lp->options);

	netif_trans_update(ndev);

    xt_printk("%s end\n", __func__);
}

/**
 * axienet_dma_bd_release - Release buffer descriptor rings
 * @ndev:	Pointer to the net_device structure
 *
 * This function is used to release the descriptors allocated in
 * axienet_dma_bd_init. axienet_dma_bd_release is called when Axi Ethernet
 * driver stop api is called.
 */
void axienet_dma_bd_release(struct net_device *ndev)
{
	int i;
	struct axienet_local *lp = netdev_priv(ndev);
    xt_printk("%s start\n",__func__);
    for_each_rx_dma_queue(lp, i) {
        axienet_bd_free(ndev, lp->dq[i]);
    }
    xt_printk("%s end\n",__func__);
}
static int xtenet_open(struct net_device *ndev)
{
    int ret = 0, i = 0;
    u32 reg, err;
	struct axienet_local *lp = netdev_priv(ndev);
    struct axienet_dma_q *q;
    xt_printk("%s start\n",__func__);
    xtnet_device_reset(ndev);
#if defined(LINUX_5_4)
    if (!lp->is_tsn) {
#elif defined(LINUX_5_15)
    if (lp->is_tsn) {
#endif
        /* DMA配置 */
        /* Enable tasklets for Axi DMA error handling */
		for_each_rx_dma_queue(lp, i) {
            tasklet_init(&lp->dma_err_tasklet[i],
				     axienet_dma_err_handler,
				     (unsigned long)lp->dq[i]);
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
        for_each_tx_dma_queue(lp, i) {
			struct axienet_dma_q *q = lp->dq[i];
            /* Enable interrupts for Axi DMA Tx */
			ret = request_irq(q->tx_irq, axienet_tx_irq,
					  0, ndev->name, ndev);
			if (ret)
				goto err_tx_irq;
        }
        for_each_rx_dma_queue(lp, i) {
            struct axienet_dma_q *q = lp->dq[i];
            /* Enable interrupts for Axi DMA Rx */
                ret = request_irq(q->rx_irq, axienet_rx_irq,
                        0, ndev->name, ndev);
                if (ret)
                    goto err_rx_irq;
        }
    }

    if (lp->phy_mode == XXE_PHY_TYPE_USXGMII) {
        netdev_dbg(ndev, "RX reg: 0x%x\n",
			   axienet_xxv_ior(lp, XXV_RCW1_OFFSET));
		/* USXGMII setup at selected speed */
		reg = axienet_xxv_ior(lp, XXV_USXGMII_AN_OFFSET);
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
		axienet_xxv_iow(lp, XXV_USXGMII_AN_OFFSET, reg);
		reg |= USXGMII_AN_EN;
		axienet_xxv_iow(lp, XXV_USXGMII_AN_OFFSET, reg);
		/* AN Restart bit should be reset, set and then reset as per
		 * spec with a 1 ms delay for a raising edge trigger
		 */
		axienet_xxv_iow(lp, XXV_USXGMII_AN_OFFSET,
			    reg & ~USXGMII_AN_RESTART);
		mdelay(1);
		axienet_xxv_iow(lp, XXV_USXGMII_AN_OFFSET,
			    reg | USXGMII_AN_RESTART);
		mdelay(1);
		axienet_xxv_iow(lp, XXV_USXGMII_AN_OFFSET,
			    reg & ~USXGMII_AN_RESTART);
        /* Check block lock bit to make sure RX path is ok with
		 * USXGMII initialization.
		 */
#if defined(LINUX_5_4)
		err = readl_poll_timeout(lp->xxv_regs + XXV_STATRX_BLKLCK_OFFSET,
					 reg, (reg & XXV_RX_BLKLCK_MASK),
					 100, DELAY_OF_ONE_MILLISEC);
#endif
		if (err) {
			netdev_err(ndev, "%s: USXGMII Block lock bit not set",
				   __func__);
			ret = -ENODEV;
			goto err_eth_irq;
		}
#if defined(LINUX_5_4)
		err = readl_poll_timeout(lp->xxv_regs + XXV_USXGMII_AN_STS_OFFSET,
					 reg, (reg & USXGMII_AN_STS_COMP_MASK),
					 1000000, DELAY_OF_ONE_MILLISEC);
#endif
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
    while (i--) {
		free_irq(q->rx_irq, ndev);
	}
	i = lp->num_tx_queues;
err_rx_irq:
    while (i--) {
		q = lp->dq[i];
		free_irq(q->tx_irq, ndev);
	}
err_tx_irq:
    for_each_rx_dma_queue(lp, i)
		napi_disable(&lp->napi[i]);
	for_each_rx_dma_queue(lp, i)
		tasklet_kill(&lp->dma_err_tasklet[i]);
	dev_err(lp->dev, "request_irq() failed\n");
    return ret;
}

static int xticenet_stop(struct net_device *ndev)
{
    	u32 cr, sr;
	int count;
	u32 i;
	struct axienet_local *lp = netdev_priv(ndev);
	struct axienet_dma_q *q;
    xt_printk("%s start\n",__func__);
	dev_dbg(&ndev->dev, "axienet_close()\n");

	lp->axienet_config->setoptions(ndev, lp->options &
			   ~(XAE_OPTION_TXEN | XAE_OPTION_RXEN));
#if defined(LINUX_5_4)
    if (!lp->is_tsn)
#elif defined(LINUX_5_15)
    if (lp->is_tsn)
#endif
    {
        for_each_tx_dma_queue(lp, i) {
			q = lp->dq[i];
			cr = axienet_dma_in32(q, XAXIDMA_RX_CR_OFFSET);
            cr &= ~(XAXIDMA_CR_RUNSTOP_MASK | XAXIDMA_IRQ_ALL_MASK);
			axienet_dma_out32(q, XAXIDMA_RX_CR_OFFSET, cr);

			cr = axienet_dma_in32(q, XAXIDMA_TX_CR_OFFSET);
			cr &= ~(XAXIDMA_CR_RUNSTOP_MASK | XAXIDMA_IRQ_ALL_MASK);
			axienet_dma_out32(q, XAXIDMA_TX_CR_OFFSET, cr);

			axienet_xxv_iow(lp, XAE_IE_OFFSET, 0);
			/* Give DMAs a chance to halt gracefully */
			sr = axienet_dma_in32(q, XAXIDMA_RX_SR_OFFSET);
			for (count = 0; !(sr & XAXIDMA_SR_HALT_MASK) && count < 5; ++count) {
				msleep(20);
				sr = axienet_dma_in32(q, XAXIDMA_RX_SR_OFFSET);
			}

			sr = axienet_dma_in32(q, XAXIDMA_TX_SR_OFFSET);
			for (count = 0; !(sr & XAXIDMA_SR_HALT_MASK) && count < 5; ++count) {
				msleep(20);
				sr = axienet_dma_in32(q, XAXIDMA_TX_SR_OFFSET);
			}

			__axienet_device_reset(q);
			free_irq(q->tx_irq, ndev);
        }
        for_each_rx_dma_queue(lp, i) {
			q = lp->dq[i];
			netif_stop_queue(ndev);
			napi_disable(&lp->napi[i]);
			tasklet_kill(&lp->dma_err_tasklet[i]);
			free_irq(q->rx_irq, ndev);
		}

        if (!lp->is_tsn)
			axienet_dma_bd_release(ndev);
    }

    xt_printk("%s end\n",__func__);
    return 0;
}

/**
 * axienet_check_tx_bd_space - Checks if a BD/group of BDs are currently busy
 * @q:		Pointer to DMA queue structure
 * @num_frag:	The number of BDs to check for
 *
 * Return: 0, on success
 *	    NETDEV_TX_BUSY, if any of the descriptors are not free
 *
 * This function is invoked before BDs are allocated and transmission starts.
 * This function returns 0 if a BD or group of BDs can be allocated for
 * transmission. If the BD or any of the BDs are not free the function
 * returns a busy status. This is invoked from axienet_start_xmit.
 */
static inline int axienet_check_tx_bd_space(struct axienet_dma_q *q,
					    int num_frag)
{
	struct axienet_local *lp = q->lp;

    struct axidma_bd *cur_p;

	if (CIRC_SPACE(q->tx_bd_tail, q->tx_bd_ci, lp->tx_bd_num) < (num_frag + 1))
		return NETDEV_TX_BUSY;

	cur_p = &q->tx_bd_v[(q->tx_bd_tail + num_frag) % lp->tx_bd_num];
	if (cur_p->status & XAXIDMA_BD_STS_ALL_MASK)
		return NETDEV_TX_BUSY;
    return 0;
}

int axienet_queue_xmit(struct sk_buff *skb,
		       struct net_device *ndev, u16 map)
{
    	u32 ii;
	u32 num_frag;
	dma_addr_t tail_p;
	struct axienet_local *lp = netdev_priv(ndev);
	struct axidma_bd *cur_p;

	unsigned long flags;
	struct axienet_dma_q *q;

    xt_printk("%s start\n",__func__);
    if (lp->axienet_config->mactype == XAXIENET_10G_25G ||
	    lp->axienet_config->mactype == XAXIENET_MRMAC) {
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

    cur_p = &q->tx_bd_v[q->tx_bd_tail];

    spin_lock_irqsave(&q->tx_lock, flags);
    if (axienet_check_tx_bd_space(q, num_frag)) {
        if (netif_queue_stopped(ndev)) {
			spin_unlock_irqrestore(&q->tx_lock, flags);
			return NETDEV_TX_BUSY;
		}

		netif_stop_queue(ndev);

        /* Matches barrier in axienet_start_xmit_done */
		smp_mb();

		/* Space might have just been freed - check again */
		if (axienet_check_tx_bd_space(q, num_frag)) {
			spin_unlock_irqrestore(&q->tx_lock, flags);
			return NETDEV_TX_BUSY;
		}

		netif_wake_queue(ndev);
    }

    cur_p->cntrl = (skb_headlen(skb) | XAXIDMA_BD_CTRL_TXSOF_MASK);

    if (!q->eth_hasdre &&
	    (((phys_addr_t)skb->data & 0x3) || num_frag > 0)) {
		skb_copy_and_csum_dev(skb, q->tx_buf[q->tx_bd_tail]);

        cur_p->phys = q->tx_bufs_dma +
			      (q->tx_buf[q->tx_bd_tail] - q->tx_bufs);

        cur_p->cntrl = skb_pagelen(skb) | XAXIDMA_BD_CTRL_TXSOF_MASK;
        goto out;
    } else {
        cur_p->phys = dma_map_single(ndev->dev.parent, skb->data,
					     skb_headlen(skb), DMA_TO_DEVICE);
    }

    for (ii = 0; ii < num_frag; ii++) {
		u32 len;
		skb_frag_t *frag;

		if (++q->tx_bd_tail >= lp->tx_bd_num)
			q->tx_bd_tail = 0;

        cur_p = &q->tx_bd_v[q->tx_bd_tail];

        frag = &skb_shinfo(skb)->frags[ii];
		len = skb_frag_size(frag);
		cur_p->phys = skb_frag_dma_map(ndev->dev.parent, frag, 0, len,
					       DMA_TO_DEVICE);
		cur_p->cntrl = len;
		cur_p->tx_desc_mapping = DESC_DMA_MAP_PAGE;
	}

    xt_printk("%s end\n",__func__);

out:
    cur_p->cntrl |= XAXIDMA_BD_CTRL_TXEOF_MASK;
	tail_p = q->tx_bd_p + sizeof(*q->tx_bd_v) * q->tx_bd_tail;

    cur_p->tx_skb = (phys_addr_t)skb;
	cur_p->tx_skb = (phys_addr_t)skb;

	tail_p = q->tx_bd_p + sizeof(*q->tx_bd_v) * q->tx_bd_tail;
	/* Ensure BD write before starting transfer */
	wmb();

    /* Start the transfer */
    xt_printk("write axidma reg 0x%x val 0x%llx\n", XAXIDMA_TX_TDESC_OFFSET, tail_p);
    axienet_dma_bdout(q, XAXIDMA_TX_TDESC_OFFSET, tail_p);
    xt_printk("read axidma reg 0x%x val 0x%x\n", XAXIDMA_TX_TDESC_OFFSET, axienet_dma_in32(q,XAXIDMA_TX_TDESC_OFFSET));
    if (++q->tx_bd_tail >= lp->tx_bd_num)
		q->tx_bd_tail = 0;

    spin_unlock_irqrestore(&q->tx_lock, flags);

    xt_printk("%s out end\n",__func__);

    return NETDEV_TX_OK;
}


/**
 * axienet_start_xmit_done - Invoked once a transmit is completed by the
 * Axi DMA Tx channel.
 * @ndev:	Pointer to the net_device structure
 * @q:		Pointer to DMA queue structure
 *
 * This function is invoked from the Axi DMA Tx isr to notify the completion
 * of transmit operation. It clears fields in the corresponding Tx BDs and
 * unmaps the corresponding buffer so that CPU can regain ownership of the
 * buffer. It finally invokes "netif_wake_queue" to restart transmission if
 * required.
 */
void axienet_start_xmit_done(struct net_device *ndev,
			     struct axienet_dma_q *q)
{
	u32 size = 0;
	u32 packets = 0;
	struct axienet_local *lp = netdev_priv(ndev);
    struct axidma_bd *cur_p;
    unsigned int status = 0;
    cur_p = &q->tx_bd_v[q->tx_bd_ci];
	status = cur_p->status;

    while (status & XAXIDMA_BD_STS_COMPLETE_MASK) {
        if (cur_p->tx_desc_mapping == DESC_DMA_MAP_PAGE)
			dma_unmap_page(ndev->dev.parent, cur_p->phys,
				       cur_p->cntrl &
				       XAXIDMA_BD_CTRL_LENGTH_MASK,
				       DMA_TO_DEVICE);
		else
			dma_unmap_single(ndev->dev.parent, cur_p->phys,
					 cur_p->cntrl &
					 XAXIDMA_BD_CTRL_LENGTH_MASK,
					 DMA_TO_DEVICE);
		if (cur_p->tx_skb)
			dev_kfree_skb_irq((struct sk_buff *)cur_p->tx_skb);
		/*cur_p->phys = 0;*/
		cur_p->app0 = 0;
		cur_p->app1 = 0;
		cur_p->app2 = 0;
		cur_p->app4 = 0;
		cur_p->status = 0;
		cur_p->tx_skb = 0;

        size += status & XAXIDMA_BD_STS_ACTUAL_LEN_MASK;
		packets++;

		if (++q->tx_bd_ci >= lp->tx_bd_num)
        q->tx_bd_ci = 0;

        cur_p = &q->tx_bd_v[q->tx_bd_ci];
		status = cur_p->status;
    }

    ndev->stats.tx_packets += packets;
	ndev->stats.tx_bytes += size;
	q->tx_packets += packets;
	q->tx_bytes += size;

	/* Matches barrier in axienet_start_xmit */
	smp_mb();

	/* Fixme: With the existing multiqueue implementation
	 * in the driver it is difficult to get the exact queue info.
	 * We should wake only the particular queue
	 * instead of waking all ndev queues.
	 */
	netif_tx_wake_all_queues(ndev);


}
static int xticenet_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
    u16 map = skb_get_queue_mapping(skb); /* Single dma queue default*/

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
 * core. It calls the core specific axienet_set_mac_address. This is the
 * function that goes into net_device_ops structure entry ndo_set_mac_address.
 */
static int netdev_set_mac_address(struct net_device *ndev, void *p)
{
	struct sockaddr *addr = p;
    xt_printk("%s start!\n", __func__);

	axienet_set_mac_address(ndev, addr->sa_data);

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
	struct axienet_local *lp = netdev_priv(ndev);
    // xt_printk("%s start\n",__func__);
    if ((lp->axienet_config->mactype != XAXIENET_1G) || lp->eth_hasnobuf)
		return;
    // xt_printk("%s end\n",__func__);

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
    struct axienet_local *lp = netdev_priv(ndev);
	int i;
    xt_printk("%s start\n", __func__);
	for_each_tx_dma_queue(lp, i)
		disable_irq(lp->dq[i]->tx_irq);
	for_each_rx_dma_queue(lp, i)
		disable_irq(lp->dq[i]->rx_irq);

	for_each_rx_dma_queue(lp, i)

	axienet_rx_irq(lp->dq[i]->rx_irq, ndev);
	for_each_tx_dma_queue(lp, i)

	axienet_tx_irq(lp->dq[i]->tx_irq, ndev);

	for_each_tx_dma_queue(lp, i)
		enable_irq(lp->dq[i]->tx_irq);
	for_each_rx_dma_queue(lp, i)
		enable_irq(lp->dq[i]->rx_irq);
    xt_printk("%s end\n", __func__);
}
#endif

/* Ioctl MII Interface */
static int xticenet_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
    xt_printk("%s start\n",__func__);

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
    .ndo_set_rx_mode = axienet_set_multicast_list,
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
    pci_read_config_byte(pdev, PCI_INTERRUPT_LINE, &val1);
    xt_printk("pci_irq_pin:0x%x\n",val1);

    pci_read_config_byte(pdev, PCI_INTERRUPT_PIN, &val1);
    xt_printk("pci_irq_line:0x%x\n",val1);

    reg_0 = axienet_xxv_ior(dev, 0x0);
    xt_printk("0x00100000 Value = 0x%x\n",reg_0);

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
    xt_printk("bar0 \t\t= 0x%llx\n", dev->bar_addr);
    xt_printk("dev->axidma_addr = 0x%llx\n", dev->axidma_addr);
    xt_printk("dev->xdma_addr \t= 0x%llx\n", dev->xdma_addr);
    xt_printk("dev->xxv_addr \t= 0x%llx\n", dev->xxv_addr);

    dev->bar_size = pci_resource_len(pdev, 0);
    xt_printk("bar0 size \t= 0x%x\n", dev->bar_size);
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
    dev->axidma_regs = dev->regs + AXIDMA_1_BASE;
    dev->xdma_regs = dev->regs + XDMA0_CTRL_BASE;
    dev->xxv_regs = dev->regs + XXV_ETHERNET_0_BASE;
    xt_printk("dev->regs \t= 0x%x\n",(unsigned int)(long)dev->regs);
    xt_printk("dev->axidma_regs = 0x%x\n",(unsigned int)(long)dev->axidma_regs);
    xt_printk("dev->xdma_regs \t= 0x%x\n",(unsigned int)(long)dev->xdma_regs);
    xt_printk("dev->xxv_regs \t= 0x%x\n",(unsigned int)(long)dev->xxv_regs);
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

/**
 * 释放中断处理程序
 */
void xtnet_irq_deinit_pcie(struct axienet_local *dev)
{
	struct pci_dev *pdev = dev->pdev;
	int k;
#if defined(LINUX_5_15)
    for (k = 0; k < 1; k++)
    {
		if (dev->irqn[k]) {
			dev->irqn[k] = 0;
		}
	}
#endif
	pci_free_irq_vectors(pdev);
}

/**
 * axienet_recv - Is called from Axi DMA Rx Isr to complete the received
 *		  BD processing.
 * @ndev:	Pointer to net_device structure.
 * @budget:	NAPI budget
 * @q:		Pointer to axienet DMA queue structure
 *
 * This function is invoked from the Axi DMA Rx isr(poll) to process the Rx BDs
 * It does minimal processing and invokes "netif_receive_skb" to complete
 * further processing.
 * Return: Number of BD's processed.
 */
static int axienet_recv(struct net_device *ndev, int budget,
			struct axienet_dma_q *q)
{
	u32 length;
	u32 size = 0;
	u32 packets = 0;
	dma_addr_t tail_p = 0;
	struct axienet_local *lp = netdev_priv(ndev);
	struct sk_buff *skb, *new_skb;
    struct axidma_bd *cur_p;
    unsigned int numbdfree = 0;

    /* Get relevat BD status value */
	rmb();

    cur_p = &q->rx_bd_v[q->rx_bd_ci];

    while ((numbdfree < budget) &&
	       (cur_p->status & XAXIDMA_BD_STS_COMPLETE_MASK)) {
		new_skb = netdev_alloc_skb(ndev, lp->max_frm_size);
		if (!new_skb) {
			dev_err(lp->dev, "No memory for new_skb\n");
			break;
		}
        tail_p = q->rx_bd_p + sizeof(*q->rx_bd_v) * q->rx_bd_ci;
        dma_unmap_single(ndev->dev.parent, cur_p->phys,
				 lp->max_frm_size,
				 DMA_FROM_DEVICE);

		skb = (struct sk_buff *)(cur_p->sw_id_offset);

		if (lp->eth_hasnobuf ||
		    (lp->axienet_config->mactype != XAXIENET_1G))
			length = cur_p->status & XAXIDMA_BD_STS_ACTUAL_LEN_MASK;
		else
			length = cur_p->app4 & 0x0000FFFF;

		skb_put(skb, length);

        skb->protocol = eth_type_trans(skb, ndev);
		/*skb_checksum_none_assert(skb);*/
		skb->ip_summed = CHECKSUM_NONE;

        if ((lp->features & XAE_FEATURE_PARTIAL_RX_CSUM) != 0 &&
			   skb->protocol == htons(ETH_P_IP) &&
			   skb->len > 64 && !lp->eth_hasnobuf &&
			   (lp->axienet_config->mactype == XAXIENET_1G)) {
			skb->csum = be32_to_cpu(cur_p->app3 & 0xFFFF);
			skb->ip_summed = CHECKSUM_COMPLETE;
		}
        netif_receive_skb(skb);

        size += length;
		packets++;

		/* Ensure that the skb is completely updated
		 * prio to mapping the DMA
		 */
		wmb();

		cur_p->phys = dma_map_single(ndev->dev.parent, new_skb->data,
					   lp->max_frm_size,
					   DMA_FROM_DEVICE);
		cur_p->cntrl = lp->max_frm_size;
		cur_p->status = 0;
		cur_p->sw_id_offset = (phys_addr_t)new_skb;

		if (++q->rx_bd_ci >= lp->rx_bd_num)
			q->rx_bd_ci = 0;

		/* Get relevat BD status value */
		rmb();

        cur_p = &q->rx_bd_v[q->rx_bd_ci];
        numbdfree++;
    }

    ndev->stats.rx_packets += packets;
	ndev->stats.rx_bytes += size;
	q->rx_packets += packets;
	q->rx_bytes += size;

	if (tail_p) {
        axienet_dma_bdout(q, XAXIDMA_RX_TDESC_OFFSET, tail_p);
    }

    return numbdfree;
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
    struct net_device *ndev = napi->dev;
	struct axienet_local *lp = netdev_priv(ndev);
	int work_done = 0;
	unsigned int status, cr;

	int map = napi - lp->napi;

	struct axienet_dma_q *q = lp->dq[map];

    spin_lock(&q->rx_lock);

    status = axienet_dma_in32(q, XAXIDMA_RX_SR_OFFSET);
	while ((status & (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_DELAY_MASK)) &&
	       (work_done < quota)) {
		axienet_dma_out32(q, XAXIDMA_RX_SR_OFFSET, status);
		if (status & XAXIDMA_IRQ_ERROR_MASK) {
			dev_err(lp->dev, "Rx error 0x%x\n\r", status);
			break;
		}
		work_done += axienet_recv(lp->ndev, quota - work_done, q);
		status = axienet_dma_in32(q, XAXIDMA_RX_SR_OFFSET);
	}

    spin_unlock(&q->rx_lock);

    if (work_done < quota) {
		napi_complete(napi);
        /* Enable the interrupts again */
		cr = axienet_dma_in32(q, XAXIDMA_RX_CR_OFFSET);
		cr |= (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_DELAY_MASK);
		axienet_dma_out32(q, XAXIDMA_RX_CR_OFFSET, cr);
    }

    return work_done;
}

/**
 * axienet_ethtools_get_drvinfo - Get various Axi Ethernet driver information.
 * @ndev:	Pointer to net_device structure
 * @ed:		Pointer to ethtool_drvinfo structure
 *
 * This implements ethtool command for getting the driver information.
 * Issue "ethtool -i ethX" under linux prompt to execute this function.
 */
static void axienet_ethtools_get_drvinfo(struct net_device *ndev,
					 struct ethtool_drvinfo *ed)
{
	strlcpy(ed->driver, DRIVER_NAME, sizeof(ed->driver));
	strlcpy(ed->version, DRIVER_VERSION, sizeof(ed->version));
}

/**
 * axienet_ethtools_get_regs_len - Get the total regs length present in the
 *				   AxiEthernet core.
 * @ndev:	Pointer to net_device structure
 *
 * This implements ethtool command for getting the total register length
 * information.
 *
 * Return: the total regs length
 */
static int axienet_ethtools_get_regs_len(struct net_device *ndev)
{
	return sizeof(u32) * AXIENET_REGS_N;
}


static void axienet_ethtools_get_ringparam(struct net_device *ndev,
					   struct ethtool_ringparam *ering)
{
	struct axienet_local *lp = netdev_priv(ndev);

	ering->rx_max_pending = RX_BD_NUM_MAX;
	ering->rx_mini_max_pending = 0;
	ering->rx_jumbo_max_pending = 0;
	ering->tx_max_pending = TX_BD_NUM_MAX;
	ering->rx_pending = lp->rx_bd_num;
	ering->rx_mini_pending = 0;
	ering->rx_jumbo_pending = 0;
	ering->tx_pending = lp->tx_bd_num;
}

static int axienet_ethtools_set_ringparam(struct net_device *ndev,
					  struct ethtool_ringparam *ering)
{
	struct axienet_local *lp = netdev_priv(ndev);

	if (ering->rx_pending > RX_BD_NUM_MAX ||
	    ering->rx_mini_pending ||
	    ering->rx_jumbo_pending ||
	    ering->rx_pending > TX_BD_NUM_MAX)
		return -EINVAL;

	if (netif_running(ndev))
		return -EBUSY;

	lp->rx_bd_num = ering->rx_pending;
	lp->tx_bd_num = ering->tx_pending;
	return 0;
}

/**
 * axienet_ethtools_sset_count - Get number of strings that
 *				 get_strings will write.
 * @ndev:	Pointer to net_device structure
 * @sset:	Get the set strings
 *
 * Return: number of strings, on success, Non-zero error value on
 *	   failure.
 */
int axienet_ethtools_sset_count(struct net_device *ndev, int sset)
{
	switch (sset) {
	case ETH_SS_STATS:
		return AXIENET_ETHTOOLS_SSTATS_LEN;
	default:
		return -EOPNOTSUPP;
	}
}

/**
 * axienet_ethtools_get_stats - Get the extended statistics
 *				about the device.
 * @ndev:	Pointer to net_device structure
 * @stats:	Pointer to ethtool_stats structure
 * @data:	To store the statistics values
 *
 * Return: None.
 */
void axienet_ethtools_get_stats(struct net_device *ndev,
				struct ethtool_stats *stats,
				u64 *data)
{
	unsigned int i = 0;

	data[i++] = ndev->stats.tx_packets;
	data[i++] = ndev->stats.rx_packets;
	data[i++] = ndev->stats.tx_bytes;
	data[i++] = ndev->stats.rx_bytes;
	data[i++] = ndev->stats.tx_errors;
	data[i++] = ndev->stats.rx_missed_errors + ndev->stats.rx_frame_errors;
}

/**
 * axienet_ethtools_strings - Set of strings that describe
 *			 the requested objects.
 * @ndev:	Pointer to net_device structure
 * @sset:	Get the set strings
 * @data:	Data of Transmit and Receive statistics
 *
 * Return: None.
 */
void axienet_ethtools_strings(struct net_device *ndev, u32 sset, u8 *data)
{
	int i;

	for (i = 0; i < AXIENET_ETHTOOLS_SSTATS_LEN; i++) {
		if (sset == ETH_SS_STATS)
			memcpy(data + i * ETH_GSTRING_LEN,
			       axienet_get_ethtools_strings_stats[i].name,
			       ETH_GSTRING_LEN);
	}
}

#if defined(LINUX_5_15)
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
#endif

#if defined(LINUX_5_4)
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
static int axienet_ethtools_set_coalesce(struct net_device *ndev,
					 struct ethtool_coalesce *ecoalesce)
{
    return 0;
}
#endif // LINUX_5_4

static const struct ethtool_ops xtnet_ethtool_ops = {
#if defined(LINUX_5_15)
     .supported_coalesce_params = ETHTOOL_COALESCE_MAX_FRAMES,
#endif
	.get_drvinfo    = axienet_ethtools_get_drvinfo,
	.get_regs_len   = axienet_ethtools_get_regs_len,
// 	.get_regs       = axienet_ethtools_get_regs,
	.get_link       = ethtool_op_get_link,
	.get_ringparam	= axienet_ethtools_get_ringparam,
	.set_ringparam  = axienet_ethtools_set_ringparam,
// 	.get_pauseparam = axienet_ethtools_get_pauseparam,
// 	.set_pauseparam = axienet_ethtools_set_pauseparam,
// 	.get_coalesce   = axienet_ethtools_get_coalesce,
	.set_coalesce   = axienet_ethtools_set_coalesce,
	.get_sset_count	= axienet_ethtools_sset_count,
// 	.get_ethtool_stats = axienet_ethtools_get_stats,
	.get_strings = axienet_ethtools_strings,
	.get_link_ksettings = phy_ethtool_get_link_ksettings,
	.set_link_ksettings = phy_ethtool_set_link_ksettings,
};

/**
 * 初始化PCI MSI中断
 */
static int xtnet_irq_init_pcie(struct axienet_local *dev)
{
    struct pci_dev *pdev = dev->pdev;
    int i;
    // Allocate MSI IRQs
#if defined(LINUX_5_4)
	dev->eth_irq = pci_alloc_irq_vectors(pdev, 1, XTIC_PCIE_MAX_IRQ, PCI_IRQ_MSI);
    xt_printk("dev->eth_irq = %d\n", dev->eth_irq);
	if (dev->eth_irq < 0) {
		xtenet_core_err(dev, "Failed to allocate IRQs");
		return -ENOMEM;
	}
#endif
    dev->irqn[0] = pci_irq_vector(pdev, 0);

    // Set up interrupts
#if defined(LINUX_5_4)
    dev->irqn[1] = pci_irq_vector(pdev, 1);
    for_each_rx_dma_queue(dev, i)
#elif defined(LINUX_5_15)
    for (i = 0; i < 1; i++)
#endif
    {
        struct axienet_dma_q *q = dev->dq[i];

        q->tx_irq = dev->irqn[0];
        q->rx_irq = dev->irqn[1];
        xt_printk("q->tx_irq = %d\n", q->tx_irq);
        xt_printk("q->rx_irq = %d\n", q->rx_irq);
	}
	return 0;
}

/**
 * 配置MAC地址
 */
void axienet_set_mac_address(struct net_device *ndev,
			     const void *address)
{
	if (address)
		ether_addr_copy(ndev->dev_addr, address);
	if (!is_valid_ether_addr(ndev->dev_addr))
		eth_hw_addr_random(ndev);
}

static int __maybe_unused axienet_dma_probe(struct pci_dev *pdev,
					    struct net_device *ndev)
{
	int i;
	struct axienet_local *lp = netdev_priv(ndev);
	struct axienet_dma_q *q;

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
    struct xtic_cdev *xcdev;
    int txcsum;
    int rxcsum;
    u16 num_queues = XTIC_MAX_QUEUES;

    xt_printk("%s start!\n", __func__);
#if defined(LINUX_5_4)
    xt_printk("define LINUX_5_4\n");
#elif defined(LINUX_5_15)
    xt_printk("define LINUX_5_15\n");
#endif
    /* 申请用于存放xtenet设备的空间 */
    ndev = alloc_etherdev_mq(sizeof(struct axienet_local), num_queues);
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

    lp->axienet_config = &axienet_10g25g_config;

    lp->num_tx_queues = num_queues;
    lp->num_rx_queues = num_queues;
    lp->rx_bd_num = RX_BD_NUM_DEFAULT;
    lp->tx_bd_num = TX_BD_NUM_DEFAULT;

    err = xtenet_pci_init(lp, pdev, id);
    if (err) {
        xtenet_core_err(lp, "xtenet_pci_init failed with error code %d\n", err);
        goto free_netdev;
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
			goto xt_err_init_one;
		}
        if (dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(lp->dma_mask)) != 0) {
			dev_warn(&pdev->dev, "default to %d-bit dma mask\n", XAE_DMA_MASK_MIN);
			if (dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(XAE_DMA_MASK_MIN)) != 0) {
				dev_err(&pdev->dev, "dma_set_mask_and_coherent failed, aborting\n");
				goto xt_err_init_one;
			}
		}
    }

    err = xtnet_irq_init_pcie(lp);
    if (err) {
		xtenet_core_err(lp, "Failed to set up interrupts err (%i)\n", err);
		goto xt_err_init_one;
	}

    axienet_set_mac_address(ndev, NULL);

    lp->coalesce_count_rx = XAXIDMA_DFT_RX_THRESHOLD;
	lp->coalesce_count_tx = XAXIDMA_DFT_TX_THRESHOLD;

    lp->phy_mode = PHY_INTERFACE_MODE_10GKR;
    lp->phy_interface = 0;

    xcdev = kzalloc(sizeof(*xcdev), GFP_KERNEL);
    if (!xcdev) {
        ret = -ENOMEM;
    }
    lp->xcdev = xcdev;
    xcdev->axidev = lp;

    xtic_cdev_create_interfaces(xcdev);

    strcpy(ndev->name, "eth%d");
    err = register_netdev(ndev);
	if (err) {
		xtenet_core_err(lp, "register_netdev() error (%i)\n", err);
		// axienet_mdio_teardown(pdev);
		goto xt_err_init_one;
	}

    return 0;
/* 错误处理 */
xt_err_init_one:
    xtenet_pci_close(lp);
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
    xtic_cdev_destroy_interfaces(lp->xcdev);
    xt_printk("%s end\n",__func__);
}

static void xtenet_shutdown(struct pci_dev *pdev)
{
	struct net_device *ndev = pci_get_drvdata(pdev);

    xt_printk("%s start\n",__func__);
	rtnl_lock();
	netif_device_detach(ndev);

	if (netif_running(ndev))
		dev_close(ndev);

	rtnl_unlock();
    xt_printk("%s end\n",__func__);
}

static struct pci_driver xtenet_driver = {
    .name     = xtenet_driver_name,
    .id_table   = xtenet_pci_tbl,
    .probe      = xtenet_probe,
    .remove     = xtenet_remove,
    .shutdown   = xtenet_shutdown,
};

static int __init xtenet_init_module(void)
{
    int ret;
    xt_printk("%s\n",__func__);

    // xtic_cdev_init();

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
