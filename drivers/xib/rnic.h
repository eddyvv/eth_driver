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

#define XRNIC_IPV6_ADD_1	0x20020

#define XRNIC_IPV4_ADDR		0x20070
#define XRNIC_MR_ACC_DESC_RD_WR 0x2
/* Global status registers */
#define XRNIC_RESP_HDLR_STAT		0x2013c



#define XRNIC_SQ_PICI_DB_CHECK_EN	(1 << 16)
/* Per QP registers */
#define XRNIC_QP_CONF(q)		(0x20200 + (q) * 0x100)
  #define QP_ENABLE			BIT(0)
  #define QP_UNDER_RECOVERY		BIT(6)
#define XRNIC_RCVQ_BUF_BASE_LSB(q)	(0x20208 + (q) * 0x100)
#define XRNIC_RCVQ_BUF_BASE_MSB(q)	(0x202C0 + (q) * 0x100)
#define XRNIC_SNDQ_BUF_BASE_LSB(q)	(0x20210 + (q) * 0x100)
#define XRNIC_CQ_BUF_BASE_LSB(q)	(0x20218 + (q) * 0x100)
#define XRNIC_RCVQ_WP_DB_ADDR_LSB(q)	(0x20220 + (q) * 0x100)
#define XRNIC_CQ_DB_ADDR_LSB(q)		(0x20228 + (q) * 0x100)
#define XRNIC_CQ_DB_ADDR_MSB(q)		(0x2022C + (q) * 0x100)
#define XRNIC_CQ_HEAD_PTR(q)		(0x20230 + (q) * 0x100)
  #define XRNIC_CQ_HEAD_PTR_MASK	0xffff
#define XRNIC_RQ_CONS_IDX(q)		(0x20234 + (q) * 0x100)
#define XRNIC_SQ_PROD_IDX(q)		(0x20238 + (q) * 0x100)
#define XRNIC_QUEUE_DEPTH(q)		(0x2023c + (q) * 0x100)
#define XRNIC_SNDQ_PSN(q)		(0x20240 + (q) * 0x100)
#define XRNIC_LAST_RQ_PSN(q)		(0x20244 + (q) * 0x100)
#define XRNIC_DEST_QP_CONF(q)		(0x20248 + (q) * 0x100)
#define XRNIC_MAC_DEST_ADDR_LO(q)	(0x20250 + (q) * 0x100)
#define XRNIC_MAC_DEST_ADDR_HI(q)	(0x20254 + (q) * 0x100)

#define XRNIC_IP_DEST_ADDR_1(q)		(0x20260 + (q) * 0x100)


#define XRNIC_STAT_QP(q)		(0x20288 + (q) * 0x100)

#define XRNIC_RCVQ_WP_DB_ADDR_MSB(q)	(0x20224 + (q) * 0x100)
#define XRNIC_CQ_BUF_BASE_MSB(q)	(0x202D0 + (q) * 0x100)
#define XRNIC_QP_PD_NUM(q)		(0x202b0 + (q) * 0x100)

#define XRNIC_SNDQ_BUF_BASE_MSB(q)	(0x202C8 + (q) * 0x100)
enum xrnic_wc_opcod {
	XRNIC_RDMA_WRITE = 0x0,
	XRNIC_RDMA_WRITE_WITH_IMM = 0x1,
	XRNIC_SEND_ONLY = 0x2,
	XRNIC_SEND_WITH_IMM = 0x3,
	XRNIC_RDMA_READ = 0x4,
	XRNIC_SEND_WITH_INV = 0xC,
};

#define XRNIC_RQ_BUF_SGE_SIZE	256

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
/* Work request 64Byte size */
#define XRNIC_WR_ID_MASK	0xffff
#define XRNIC_OPCODE_MASK	0xff
struct xrnic_wr
{
	u32	wrid;
	u64	l_addr;
	u32	length;
	u32	opcode;
	u64	r_offset;
	u32	r_tag;
#define XRNIC_MAX_SDATA		16
	u8	sdata[XRNIC_MAX_SDATA];
	u32	imm_data;
	u8	resvd[12];
}__attribute__((packed));

static inline void xrnic_iow(struct xrnic_local *xl, off_t offset, u32 value)
{
	iowrite32(value, (xl->reg_base + offset));
}

static inline u32 xrnic_ior(struct xrnic_local *xl, off_t offset)
{
	return ioread32( (xl->reg_base + offset));
}

int xrnic_reg_mr(struct xilinx_ib_dev *xib, u64 va, u64 len,
		u64 *pbl_tbl, int umem_pgs, int pdn, u32 mr_idx, u8 rkey);

u64 xrnic_get_sq_db_addr(struct xrnic_local *xl, int hw_qpn);
void config_raw_ip(struct xrnic_local *xl, u32 base, u32 *ip, bool is_ipv6);
u64 xrnic_get_rq_db_addr(struct xrnic_local *xl, int hw_qpn);
int xrnic_qp_set_pd(struct xilinx_ib_dev *xib, int qpn, int pdn);
#endif