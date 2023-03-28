// SPDX-License-Identifier: GPL-2.0

/* Xilinx AXI Ethernet (DMA programming)
 *
 * Copyright (c) 2008 Nissin Systems Co., Ltd.,  Yoshio Kashiwagi
 * Copyright (c) 2005-2008 DLA Systems,  David H. Lynch Jr. <dhlii@dlasys.net>
 * Copyright (c) 2008-2009 Secret Lab Technologies Ltd.
 * Copyright (c) 2010 - 2011 Michal Simek <monstr@monstr.eu>
 * Copyright (c) 2010 - 2011 PetaLogix
 * Copyright (c) 2010 - 2012 Xilinx, Inc.
 * Copyright (C) 2018 Xilinx, Inc. All rights reserved.
 *
 * This file contains helper functions for AXI DMA TX and RX programming.
 */

#include "xtic_enet.h"


/**
 * axienet_bd_free - Release buffer descriptor rings for individual dma queue
 * @ndev:	Pointer to the net_device structure
 * @q:		Pointer to DMA queue structure
 *
 * This function is helper function to axienet_dma_bd_release.
 */

void __maybe_unused axienet_bd_free(struct net_device *ndev,
				    struct axienet_dma_q *q)
{
	int i;
	struct xtenet_core_dev *lp = netdev_priv(ndev);

	for (i = 0; i < lp->rx_bd_num; i++) {
		dma_unmap_single(ndev->dev.parent, q->rx_bd_v[i].phys,
				 lp->max_frm_size, DMA_FROM_DEVICE);
		dev_kfree_skb((struct sk_buff *)
			      (q->rx_bd_v[i].sw_id_offset));
	}

	if (q->rx_bd_v) {
		dma_free_coherent(ndev->dev.parent,
				  sizeof(*q->rx_bd_v) * lp->rx_bd_num,
				  q->rx_bd_v,
				  q->rx_bd_p);
	}
	if (q->tx_bd_v) {
		dma_free_coherent(ndev->dev.parent,
				  sizeof(*q->tx_bd_v) * lp->tx_bd_num,
				  q->tx_bd_v,
				  q->tx_bd_p);
	}
	if (q->tx_bufs) {
		dma_free_coherent(ndev->dev.parent,
				  XAE_MAX_PKT_LEN * lp->tx_bd_num,
				  q->tx_bufs,
				  q->tx_bufs_dma);
	}
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
	struct xtenet_core_dev *lp = netdev_priv(ndev);

#ifdef CONFIG_AXIENET_HAS_MCDMA
	for_each_tx_dma_queue(lp, i) {
		axienet_mcdma_tx_bd_free(ndev, lp->dq[i]);
	}
#endif
	for_each_rx_dma_queue(lp, i) {
#ifdef CONFIG_AXIENET_HAS_MCDMA
		axienet_mcdma_rx_bd_free(ndev, lp->dq[i]);
#else
		axienet_bd_free(ndev, lp->dq[i]);
#endif
	}
}

/**
 * __dma_txq_init - Setup buffer descriptor rings for individual Axi DMA-Tx
 * @ndev:	Pointer to the net_device structure
 * @q:		Pointer to DMA queue structure
 *
 * Return: 0, on success -ENOMEM, on failure
 *
 * This function is helper function to axienet_dma_q_init
 */
static int __dma_txq_init(struct net_device *ndev, struct axienet_dma_q *q)
{
	int i;
	u32 cr;
	struct xtenet_core_dev *lp = netdev_priv(ndev);

	q->tx_bd_ci = 0;
	q->tx_bd_tail = 0;

	q->tx_bd_v = dma_alloc_coherent(ndev->dev.parent,
					sizeof(*q->tx_bd_v) * lp->tx_bd_num,
					&q->tx_bd_p, GFP_KERNEL);
	if (!q->tx_bd_v)
		goto out;

	for (i = 0; i < lp->tx_bd_num; i++) {
		q->tx_bd_v[i].next = q->tx_bd_p +
				     sizeof(*q->tx_bd_v) *
				     ((i + 1) % lp->tx_bd_num);
	}

	if (!q->eth_hasdre) {
		q->tx_bufs = dma_alloc_coherent(ndev->dev.parent,
						XAE_MAX_PKT_LEN * lp->tx_bd_num,
						&q->tx_bufs_dma,
						GFP_KERNEL);
		if (!q->tx_bufs)
			goto out;

		for (i = 0; i < lp->tx_bd_num; i++)
			q->tx_buf[i] = &q->tx_bufs[i * XAE_MAX_PKT_LEN];
	}

	/* Start updating the Tx channel control register */
	cr = axienet_dma_in32(q, XAXIDMA_TX_CR_OFFSET);
	/* Update the interrupt coalesce count */
	cr = (((cr & ~XAXIDMA_COALESCE_MASK)) |
	      ((lp->coalesce_count_tx) << XAXIDMA_COALESCE_SHIFT));
	/* Update the delay timer count */
	cr = (((cr & ~XAXIDMA_DELAY_MASK)) |
	      (XAXIDMA_DFT_TX_WAITBOUND << XAXIDMA_DELAY_SHIFT));
	/* Enable coalesce, delay timer and error interrupts */
	cr |= XAXIDMA_IRQ_ALL_MASK;
	/* Write to the Tx channel control register */
	axienet_dma_out32(q, XAXIDMA_TX_CR_OFFSET, cr);

	/* Write to the RS (Run-stop) bit in the Tx channel control register.
	 * Tx channel is now ready to run. But only after we write to the
	 * tail pointer register that the Tx channel will start transmitting.
	 */
	axienet_dma_bdout(q, XAXIDMA_TX_CDESC_OFFSET, q->tx_bd_p);
	cr = axienet_dma_in32(q, XAXIDMA_TX_CR_OFFSET);
	axienet_dma_out32(q, XAXIDMA_TX_CR_OFFSET,
			  cr | XAXIDMA_CR_RUNSTOP_MASK);
	return 0;
out:
	return -ENOMEM;
}

/**
 * __dma_rxq_init - Setup buffer descriptor rings for individual Axi DMA-Rx
 * @ndev:	Pointer to the net_device structure
 * @q:		Pointer to DMA queue structure
 *
 * Return: 0, on success -ENOMEM, on failure
 *
 * This function is helper function to axienet_dma_q_init
 */
static int __dma_rxq_init(struct net_device *ndev,
			  struct axienet_dma_q *q)
{
	int i;
	u32 cr;
	struct sk_buff *skb;
	struct xtenet_core_dev *lp = netdev_priv(ndev);
	/* Reset the indexes which are used for accessing the BDs */
	q->rx_bd_ci = 0;

	/* Allocate the Rx buffer descriptors. */
	q->rx_bd_v = dma_alloc_coherent(ndev->dev.parent,
					sizeof(*q->rx_bd_v) * lp->rx_bd_num,
					&q->rx_bd_p, GFP_KERNEL);
	if (!q->rx_bd_v)
		goto out;

	for (i = 0; i < lp->rx_bd_num; i++) {
		q->rx_bd_v[i].next = q->rx_bd_p +
				     sizeof(*q->rx_bd_v) *
				     ((i + 1) % lp->rx_bd_num);

		skb = netdev_alloc_skb(ndev, lp->max_frm_size);
		if (!skb)
			goto out;

		/* Ensure that the skb is completely updated
		 * prio to mapping the DMA
		 */
		wmb();

		q->rx_bd_v[i].sw_id_offset = (phys_addr_t)skb;
		q->rx_bd_v[i].phys = dma_map_single(ndev->dev.parent,
						    skb->data,
						    lp->max_frm_size,
						    DMA_FROM_DEVICE);
		q->rx_bd_v[i].cntrl = lp->max_frm_size;
	}

	/* Start updating the Rx channel control register */
	cr = axienet_dma_in32(q, XAXIDMA_RX_CR_OFFSET);
	/* Update the interrupt coalesce count */
	cr = ((cr & ~XAXIDMA_COALESCE_MASK) |
	      ((lp->coalesce_count_rx) << XAXIDMA_COALESCE_SHIFT));
	/* Update the delay timer count */
	cr = ((cr & ~XAXIDMA_DELAY_MASK) |
	      (XAXIDMA_DFT_RX_WAITBOUND << XAXIDMA_DELAY_SHIFT));
	/* Enable coalesce, delay timer and error interrupts */
	cr |= XAXIDMA_IRQ_ALL_MASK;
	/* Write to the Rx channel control register */
	axienet_dma_out32(q, XAXIDMA_RX_CR_OFFSET, cr);

	/* Populate the tail pointer and bring the Rx Axi DMA engine out of
	 * halted state. This will make the Rx side ready for reception.
	 */
	axienet_dma_bdout(q, XAXIDMA_RX_CDESC_OFFSET, q->rx_bd_p);
	cr = axienet_dma_in32(q, XAXIDMA_RX_CR_OFFSET);
	axienet_dma_out32(q, XAXIDMA_RX_CR_OFFSET,
			  cr | XAXIDMA_CR_RUNSTOP_MASK);
	axienet_dma_bdout(q, XAXIDMA_RX_TDESC_OFFSET, q->rx_bd_p +
			  (sizeof(*q->rx_bd_v) * (lp->rx_bd_num - 1)));

	return 0;
out:
	return -ENOMEM;
}

/**
 * axienet_dma_q_init - Setup buffer descriptor rings for individual Axi DMA
 * @ndev:	Pointer to the net_device structure
 * @q:		Pointer to DMA queue structure
 *
 * Return: 0, on success -ENOMEM, on failure
 *
 * This function is helper function to axienet_dma_bd_init
 */
int __maybe_unused axienet_dma_q_init(struct net_device *ndev,
				      struct axienet_dma_q *q)
{
	if (__dma_txq_init(ndev, q))
		goto out;

	if (__dma_rxq_init(ndev, q))
		goto out;

	return 0;
out:
	axienet_dma_bd_release(ndev);
	return -ENOMEM;
}

MODULE_LICENSE("GPL");

