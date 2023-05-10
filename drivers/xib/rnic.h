#ifndef _XRNIC_H_
#define _XRNIC_H_

#define XRNIC_BUF_RKEY_MASK		(0xFF)
#define XRNIC_MR_PDNUM(mrn)		(0x00 + (mrn) * 0x100)
#define XRNIC_MR_VA_LO(mrn)		(0x04 + (mrn) * 0x100)
#define XRNIC_MR_VA_HI(mrn)		(0x08 + (mrn) * 0x100)
#define XRNIC_MR_BUF_BASE_LO(mrn)	(0x0c + (mrn) * 0x100)
#define XRNIC_MR_BUF_BASE_HI(mrn)	(0x10 + (mrn) * 0x100)
#define XRNIC_MR_BUF_RKEY(mrn)		(0x14 + (mrn) * 0x100)
#define XRNIC_MR_WRRD_BUF_LEN(mrn)	(0x18 + (mrn) * 0x100)
#define XRNIC_MR_ACC_DESC(mrn)		(0x1c + (mrn) * 0x100)


#define XRNIC_MR_ACC_DESC_RD_WR 0x2

struct xrnic_local {
	struct xilinx_ib_dev		*xib;
	struct platform_device		*pdev;
	u8 __iomem			*reg_base;
	int				irq;
	u64				qp1_sq_db_p;
	u32				*qp1_sq_db_v;
	u64				qp1_rq_db_p;
	u32				*qp1_rq_db_v;
	int				qps_enabled;
	u16				udp_sport;
	dma_addr_t			db_pa;
	u32				db_size;
	u8 __iomem			*ext_hh_base;
	u8 __iomem			*hw_hsk_base;
	u8 __iomem			*qp_hh_base;
	dma_addr_t			in_pkt_err_ba;
	dma_addr_t			retry_buf_pa;
	void				*in_pkt_err_va;
	void				*retry_buf_va;
	u32				in_pkt_err_db_local;
	u32				db_chunk_id;
};


static inline void xrnic_iow(struct xrnic_local *xl, off_t offset, u32 value)
{
	iowrite32(value, (xl->reg_base + offset));
}
int xrnic_reg_mr(struct xilinx_ib_dev *xib, u64 va, u64 len,
		u64 *pbl_tbl, int umem_pgs, int pdn, u32 mr_idx, u8 rkey);
#endif