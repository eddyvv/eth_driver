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


int xrnic_reg_mr(struct xilinx_ib_dev *xib, u64 va, u64 len,
		u64 *pbl_tbl, int umem_pgs, int pdn, u32 mr_idx, u8 rkey)
{
	struct xrnic_local *xl = xib->xl;
	int i;
	u64 pa = pbl_tbl[0];
	u32 value = 0;

//	dev_dbg(&xib->ib_dev.dev, "%s: pbl_tbl: %px \n", __func__, pbl_tbl);

	for(i = 0; i < umem_pgs; i++)
		dev_dbg(&xib->ib_dev.dev, "pbl_tbl[%d]: %lx \n", i, pbl_tbl[i]);
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


struct xrnic_local *xrnic_hw_init(struct pci_dev *pdev, struct xilinx_ib_dev *xib)
{
    struct xrnic_local *xl;
	struct resource *res;
    // struct axienet_local *lp = pci_get_drvdata(pdev);
	u32 *db_buf;
	u64 db_buf_pa;
	int err;

	xl = kzalloc(sizeof(*xl), GFP_KERNEL);
	if (!xl) {
		dev_err(&pdev->dev, "memory alloc failed\n");
		return NULL;
	}

    xl->pdev = pdev;
	xl->retry_buf_va = NULL;
	xl->in_pkt_err_va = NULL;
    // xl->reg_base = ;
    /* store pa of reg for db access */
	xl->db_pa = (res->start + 0x20000);
	xl->db_size = (res->end - (res->start + 0x20000) + 1);

	dev_dbg(&pdev->dev, "xl->reg_base: %lx\n", xl->reg_base);

	// xl->irq = ;
	if (xl->irq <= 0) {
		dev_err(&pdev->dev, "ernic dev get of irq failed!\n");
		goto fail;
	}

    /* set default hw config */
	xrnic_set_bufs(pdev, xl);


fail:
	kfree(xl);
	return NULL;
}

