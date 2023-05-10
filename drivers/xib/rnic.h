#ifndef _XRNIC_H_
#define _XRNIC_H_


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