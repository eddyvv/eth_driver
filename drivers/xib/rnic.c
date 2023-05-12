#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <asm/byteorder.h>
#include <rdma/ib_addr.h>
#include <rdma/ib_smi.h>
#include <rdma/ib_user_verbs.h>
#include <rdma/ib_cache.h>
#include "rnic.h"
#include "xib.h"
#include "ib_verbs.h"

void xrnic_set_mac(struct xrnic_local *xl, u8 *mac)
{
	u32 val;

	val = mac[5] | (mac[4] << 8) |
		(mac[3] << 16) | (mac[2] << 24);

	xrnic_iow(xl, XRNIC_MAC_ADDR_LO, val);

  	wmb();
  	val =  mac[1] | (mac[0] << 8);
	xrnic_iow(xl, XRNIC_MAC_ADDR_HI, val);
}

void config_raw_ip(struct xrnic_local *xl, u32 base, u32 *ip, bool is_ipv6)
{
	u32 val = 0, i;

	if (!is_ipv6) {
		val = cpu_to_be32(*ip);
		xrnic_iow(xl, base, val);
	} else {
		for (i = 0; i < 4; i++) {
			val = cpu_to_be32(ip[i]);
			xrnic_iow(xl, base + (3 - i) * 4, val);
		}
	}
}

/*
 *
 */
int xrnic_start(struct xrnic_local *xl)
{
	u32 val;

	val = ( XRNIC_EN | (1 << 5) |
		((xl->qps_enabled & XRNIC_NUM_QP_MASK) << XRNIC_NUM_QP_SHIFT) |
		((xl->udp_sport & XRNIC_UDP_SPORT_MASK) <<
		XRNIC_UDP_SPORT_SHIFT) );
	xrnic_iow(xl, XRNIC_CONF, val);

	return 0;
}

#define NUM_CQ_INT_BANKS 8
void xib_irq_event(struct xilinx_ib_dev *xib, u32 status)
{
	struct xrnic_local *xl = xib->xl;
	u32 cq_ints, base;
	struct xib_qp *qp = NULL;
	uint32_t i, qpn, qp_num;

	if (status & XRNIC_INT_WQE_COMP)
		base = XRNIC_COMPQ_INT_STAT_0_31;
	else if(status & XRNIC_INT_RQ_PKT_RCVD)
		base = XRNIC_RCVQ_INT_STAT_0_31;
	else if (status & XRNIC_INT_CNP)
		base = XRNIC_CNP_INT_STAT_0_31;
	else
		return;

	for (i = 0; i < NUM_CQ_INT_BANKS; i++) {
		cq_ints = xrnic_ior(xl, (base + (i * 4)));

		/* clear cq int*/
		xrnic_iow(xl, (base + (i * 4)), cq_ints);

		qpn = ffs(cq_ints);
		while (qpn) {
			//dev_dbg(&xib->ib_dev.dev, "wqe complete on qpn: %d\n", qpn);
			qp_num = (i * 32) + qpn;
			qp = xib->qp_list[qp_num - 2];

			/* the Bottom half invokes kernel user app's functions registered
				for RQ & CQ completions which are not invoked when it's a
				user QP */
			if (!qp) {
				pr_err("qp doesnt exist : qpn is %d \n", qp_num);
			}

			if (status & XRNIC_INT_CNP)
				tasklet_schedule(&qp->cnp_task);
			else if (qp->qp_type != XIB_QP_TYPE_USER)
				tasklet_schedule(&qp->comp_task);

			cq_ints &= ~(1 << (qpn - 1));
			qpn = ffs(cq_ints);
		}
	}
}

void xib_fatal_event(struct xilinx_ib_dev *xib)
{
	u32 in_pkt_err_db;
	struct xrnic_local *xl = xib->xl;
	u32 entry, qp_num, val;
	int i, received;
	struct xib_qp *qp = NULL;

	in_pkt_err_db = xrnic_ior(xl, XRNIC_INCG_PKT_ERRQ_WPTR);

	if (in_pkt_err_db == xl->in_pkt_err_db_local) {
		return;
	}

	if (xl->in_pkt_err_db_local > in_pkt_err_db)
		received = (in_pkt_err_db + XRNIC_IN_PKT_ERRQ_DEPTH) -
				xl->in_pkt_err_db_local;
	else
		received = in_pkt_err_db - xl->in_pkt_err_db_local;

	for (i = 0; i < received; i++) {
		if (xl->in_pkt_err_db_local == XRNIC_IN_PKT_ERRQ_DEPTH)
			xl->in_pkt_err_db_local = 0;
		entry = xl->in_pkt_err_db_local;

		val = *(volatile u32 *)(xl->in_pkt_err_va + (8 * entry));
		qp_num = (val & 0xffff0000) >> 16;
		printk("Fatal error on qp_num: %d\n", qp_num);
		val = xrnic_ior(xib->xl, XRNIC_QP_CONF(qp_num - 1));
		val &= ~(QP_ENABLE);
		val |= QP_UNDER_RECOVERY;
		xrnic_iow(xib->xl, XRNIC_QP_CONF(qp_num - 1), val);

		qp = xib->qp_list[qp_num - 1];
		if (qp)
			tasklet_schedule(&qp->fatal_hdlr_task);
		xl->in_pkt_err_db_local++;
	}

	/* clear fatal interrupt */
	val = xrnic_ior(xib->xl, XRNIC_INT_STAT);
	val |= XRNIC_INT_FATAL_ERR_RCVD;
	xrnic_iow(xib->xl, XRNIC_INT_STAT, val);
}

/*
 *
 */
irqreturn_t xib_irq(int irq, void *ptr)
{
	struct xilinx_ib_dev *xib = (struct xilinx_ib_dev *)ptr;
	struct xrnic_local *xl = xib->xl;
	u32 status;
	struct xib_qp *qp;

	status = xrnic_ior(xl, XRNIC_INT_STAT);
	rmb();

	//dev_dbg(&xib->ib_dev.dev, "%s status : %d <---------- \n", __func__, status);

	/* clear the interrupt */
	xrnic_iow(xl, XRNIC_INT_STAT, status);
	wmb();

	if (status & XRNIC_INT_WQE_COMP)
		xib_irq_event(xib, XRNIC_INT_WQE_COMP);

	if (status & XRNIC_INT_RQ_PKT_RCVD)
		/* @TODO This is to  test CNP. Remove this once the testing is done */
		xib_irq_event(xib, XRNIC_INT_RQ_PKT_RCVD);

	if (status & XRNIC_INT_CNP)
		xib_irq_event(xib, XRNIC_INT_CNP);

	if (status & XRNIC_INT_INC_MAD_PKT) {
		qp = xib->gsi_qp;
		tasklet_schedule(&qp->comp_task);
	}

	if (status & XRNIC_INT_FATAL_ERR_RCVD) {
		pr_err("xib_irq: qp fatal error!\n");
		xib_fatal_event(xib);
	}


	return IRQ_HANDLED;
}

int xrnic_reg_mr(struct xilinx_ib_dev *xib, u64 va, u64 len,
		u64 *pbl_tbl, int umem_pgs, int pdn, u32 mr_idx, u8 rkey)
{
	struct xrnic_local *xl = xib->xl;
	int i;
	u64 pa = pbl_tbl[0];
	u32 value = 0;

//	dev_dbg(&xib->ib_dev.dev, "%s: pbl_tbl: %px \n", __func__, pbl_tbl);

	for(i = 0; i < umem_pgs; i++)
		dev_dbg(&xib->ib_dev.dev, "pbl_tbl[%d]: %llx \n", i, pbl_tbl[i]);
	xrnic_iow(xl, XRNIC_MR_PDNUM(mr_idx), pdn);
	xrnic_iow(xl, XRNIC_MR_VA_LO(mr_idx), (va & 0xffffffff));
	xrnic_iow(xl, XRNIC_MR_VA_HI(mr_idx), (va >> 32) & 0xFFFFFFFF);
	wmb();

	xrnic_iow(xl, XRNIC_MR_BUF_BASE_LO(mr_idx), pa & 0xffffffff);
	xrnic_iow(xl, XRNIC_MR_BUF_BASE_HI(mr_idx), ((pa >> 32) & 0xFFFFFFFF));
	wmb();
	xrnic_iow(xl, XRNIC_MR_BUF_RKEY(mr_idx), rkey & XRNIC_BUF_RKEY_MASK);
	xrnic_iow(xl, XRNIC_MR_WRRD_BUF_LEN(mr_idx), (len & 0xffffffff));

	value = XRNIC_MR_ACC_DESC_RD_WR;
	value |= (len >> 32) << 16;
	xrnic_iow(xl, XRNIC_MR_ACC_DESC(mr_idx), value);
	wmb();

	return 0;
}

/*
 *
 */
dma_addr_t xrnic_buf_alloc(struct xrnic_local *xl, u32 size, u32 count)
{
    int order;
	void *buf;
    dma_addr_t addr;

	order = get_order(PAGE_ALIGN(size * count));
	buf = (void *)__get_free_pages(GFP_KERNEL, order);
	if (!buf) {
		dev_err(&xl->pdev->dev, "failed to alloc xrnic buffers, order\
				:%d\n", order);
		return 0;
	}

	addr = dma_map_single(&xl->pdev->dev, buf,
				PAGE_ALIGN(size * count),
				DMA_FROM_DEVICE);

	if (dma_mapping_error(&xl->pdev->dev, addr)) {
		dev_err(&xl->pdev->dev, "failed to alloc xrnic buffers, order\
				:%d\n", order);
		return 0;
	}
	return addr;
}

static void xrnic_set_bufs(struct pci_dev *pdev, struct xrnic_local *xl)
{
    dma_addr_t addr;
	u32 val;

    addr = xrnic_buf_alloc(xl, XRNIC_SIZE_OF_ERROR_BUF, XRNIC_NUM_OF_ERROR_BUF);
	if (!addr) {
		dev_err(&pdev->dev, "xrnic_set_bufs: Failed to allocate err bufs\n");
		return;
	}
    xrnic_iow(xl, XRNIC_ERR_BUF_BASE_LSB, addr);
	xrnic_iow(xl, XRNIC_ERR_BUF_BASE_MSB, UPPER_32_BITS(addr));
	wmb();
	val = XRNIC_NUM_OF_ERROR_BUF | ( XRNIC_SIZE_OF_ERROR_BUF << 16);
	xrnic_iow(xl, XRNIC_ERR_BUF_SZ, val);
	wmb();

	addr = xrnic_buf_alloc(xl, PAGE_SIZE, 4);
	if (!addr) {
		dev_err(&pdev->dev, "xrnic_set_bufs: Failed to allocate incg pkt err bufs\n");
		return;
	}
	xrnic_iow(xl, XRNIC_INCG_PKT_ERRQ_BASE_LSB, addr);
	xrnic_iow(xl, XRNIC_INCG_PKT_ERRQ_BASE_MSB, UPPER_32_BITS(addr));
	wmb();
	xrnic_iow(xl, XRNIC_INCG_PKT_ERRQ_SZ, XRNIC_IN_ERRST_Q_NUM_ENTRIES);
	wmb();

	addr = xrnic_buf_alloc(xl, XRNIC_RESP_ERR_BUF_SIZE, XRNIC_RESP_ERR_BUF_DEPTH);
	xrnic_iow(xl, XRNIC_RSP_ERR_BUF_BA_LSB, addr);
	xrnic_iow(xl, XRNIC_RSP_ERR_BUF_BA_MSB, UPPER_32_BITS(addr));
	wmb();
	xrnic_iow(xl, XRNIC_RSP_ERR_BUF_DEPTH, (XRNIC_RESP_ERR_BUF_DEPTH << 16 | XRNIC_RESP_ERR_BUF_SIZE));
	wmb();
}


struct xrnic_local *xrnic_hw_init(struct xib_dev_info *dev_info, struct xilinx_ib_dev *xib)
{
    struct xrnic_local *xl;
    struct pci_dev *pdev = dev_info->pdev;
	struct resource *res;
	u32 *db_buf;
	u64 db_buf_pa;

	xl = kzalloc(sizeof(*xl), GFP_KERNEL);
	if (!xl) {
		dev_err(&pdev->dev, "memory alloc failed\n");
		return NULL;
	}

    xl->pdev = pdev;
	xl->retry_buf_va = NULL;
	xl->in_pkt_err_va = NULL;
    xl->reg_base = dev_info->xib_regAddr;
    /* store pa of reg for db access */
	xl->db_pa = (res->start + 0x20000);
	xl->db_size = (res->end - (res->start + 0x20000) + 1);

	dev_dbg(&pdev->dev, "xl->reg_base: %hhn\n", xl->reg_base);

	// xl->irq = ;
	if (xl->irq <= 0) {
		dev_err(&pdev->dev, "ernic dev get of irq failed!\n");
		goto fail;
	}

    /* set default hw config */
	xrnic_set_bufs(pdev, xl);

    /* enable all interrupts */
	xrnic_iow(xl, XRNIC_INT_EN, 0xff);

    // db_buf = ;
    if (!db_buf) {
		printk("failed to alloc db mem %s:%d\n",
							__func__, __LINE__);
		goto fail;
	}

    xl->qp1_rq_db_p = db_buf_pa;
	xl->qp1_rq_db_v = db_buf;

    db_buf = dma_alloc_coherent(&pdev->dev, PAGE_SIZE, (dma_addr_t *)&db_buf_pa,
			GFP_KERNEL);
	if (!db_buf) {
		dev_err(&pdev->dev, "failed to alloc db mem\n");
		goto fail;
	}

	/* 512 bytes for all DBs for all QPs? TODO */
	xl->qp1_sq_db_v = db_buf;
	xl->qp1_sq_db_p = db_buf_pa;

    return xl;

fail:
	kfree(xl);
	return NULL;
}

/*
 *
 */
void xrnic_hw_deinit(struct xilinx_ib_dev *xib)
{
	struct xrnic_local *xl = xib->xl;
    struct device *dev = &xib->pdev->dev;

    dma_free_coherent(dev, PAGE_SIZE, xl->qp1_sq_db_v,
			xl->qp1_sq_db_p);
    dma_free_coherent(dev, PAGE_SIZE, xl->qp1_rq_db_v,
			xl->qp1_rq_db_p);

	free_irq(xl->irq, xib);
	kfree(xl);
}