#ifndef _XRNIC_H_
#define _XRNIC_H_

/* Per QP registers */
#define XRNIC_QP_CONF(q)		(0x20200 + (q) * 0x100)
  #define QP_ENABLE			BIT(0)
  #define QP_RQ_IRQ_EN			BIT(2)
  #define QP_CQ_IRQ_EN			BIT(3)
  #define QP_HW_HSK_DIS			BIT(4)
  #define QP_CQE_EN			BIT(5)
  #define QP_UNDER_RECOVERY		BIT(6)
  #define QP_CONF_IPV6			BIT(7)
  #define QP_PMTU_SHIFT			8
  #define QP_PMTU_MASK			0x7
  #define QP_PMTU_256			0x0
  #define QP_PMTU_512			0x1
  #define QP_PMTU_1024			0x2
  #define QP_PMTU_2048			0x3
  #define QP_PMTU_4096			0x4
  #define QP_RQ_BUF_SZ_SHIFT		16
  #define QP_RQ_BUF_SZ_MASK		0xffff

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


#endif