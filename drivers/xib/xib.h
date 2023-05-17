#ifndef _XIB_H_
#define _XIB_H_
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <rdma/ib_verbs.h>
#include "rnic.h"

#define XIB_MAX_BMAP_NAME	(16)
struct xib_bmap {
	unsigned long *bitmap;
	u32 max_count;
	char name[XIB_MAX_BMAP_NAME];
};

struct xilinx_ib_dev_attr {
	u32		max_pd;
	u32		max_cq_wqes;
	u32		max_qp_wqes;
	u32		max_qp;
	u32		max_mr;
	u32		max_send_sge;

};

struct xilinx_ib_dev {
	struct ib_device		ib_dev;
	struct pci_dev		*pdev;
	struct net_device		*netdev;
	u8				active_speed;
	u8				active_width;
	u16				mtu;
	struct xilinx_ib_dev_attr	dev_attr;
	struct xrnic_local		*xl;
	struct xib_qp			*gsi_qp;
	spinlock_t			lock;
	struct xib_bmap			pd_map;
	struct xib_bmap			qp_map;
	struct xib_bmap			mr_map;
	struct xib_qp			**qp_list;
	struct kobject			*pfc_kobj;
	struct axidma_q                 *dmaq;
};

struct xib_pd {
	struct ib_pd		ib_pd;
	u32			pdn;
};

enum xib_mr_type {
	XIB_MR_DMA,
	XIB_MR_USER
};
struct xib_mr {
	struct ib_mr		ib_mr;
	struct ib_umem		*umem;
	u64			size;
	u32			mr_idx;
	u32			pd;
	int			type;
	u8			rkey;
};

struct xib_ah {
	struct ib_ah ib_ah;
	struct rdma_ah_init_attr attr;
};

struct xib_ucontext {
	struct ib_ucontext	ib_uc;
	int			pfn;
	unsigned int		index;
};

#define UPPER_32_BITS(a) ((a) >> 32U)

#define XRNIC_SQ_WQE_SIZE 64
#define XRNIC_SEND_SGL_SIZE 4096
#if 0
#define XRNIC_SQ_DEPTH 128
#define USER_RQ_DEPTH 16
#define USER_SQ_DEPTH 64
#define USER_CQ_SIZE  2048
#endif
#define CQE_SIZE	4

static inline struct xilinx_ib_dev *get_xilinx_dev(struct ib_device *ibdev)
{
	return container_of(ibdev, struct xilinx_ib_dev, ib_dev);
}

static inline struct xib_ucontext *get_xib_ucontext(struct ib_ucontext *ibuc)
{
	return container_of(ibuc, struct xib_ucontext, ib_uc);
}

static inline struct xib_ah *get_xib_ah(struct ib_ah *ibah)
{
	return container_of(ibah, struct xib_ah, ib_ah);
}

static inline struct xib_pd *get_xib_pd(struct ib_pd *ibpd)
{
	return container_of(ibpd, struct xib_pd, ib_pd);
}

static inline struct xib_mr *get_xib_mr(struct ib_mr *ibmr)
{
	return container_of(ibmr, struct xib_mr, ib_mr);
}

irqreturn_t xib_irq(int irq, void *ptr);
void xib_gsi_comp_handler(unsigned long data);
int xib_bmap_alloc(struct xib_bmap *bmap, u32 max_count, char *name);
int xib_bmap_alloc_id(struct xib_bmap *bmap, u32 *id_num);
void xib_bmap_release_id(struct xib_bmap *bmap, u32 id_num);
void xib_fatal_handler(unsigned long data);
void xib_cnp_handler(unsigned long data);
#endif /* _XIB_H_ */