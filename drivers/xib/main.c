/*
 * Xilinx FPGA Xilinx ERNIC Infiniband Driver
 *
 * Copyright (c) 2019 Xilinx Pvt., Ltd
 *
 * Author: Syed S <syeds@xilinx.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/of_platform.h>
#include <linux/of_net.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <rdma/ib_addr.h>
#include <rdma/ib_smi.h>
#include <rdma/ib_user_verbs.h>
#include <rdma/ib_cache.h>
#include <rdma/ib_umem.h>

#include <rdma/uverbs_ioctl.h>
#include <net/addrconf.h>
#include <linux/of_address.h>
#include <linux/jiffies.h>
#include "xib-abi.h"
#include "xtic_common.h"
#include "../eth/xt_roce.h"
#include "xib.h"
#include "ib_verbs.h"

struct xilinx_ib_dev *ibdev;

unsigned int app_qp_cnt = 10;

unsigned int app_qp_depth = 16;

unsigned int max_rq_sge = 16;

static int check_qp_depths(unsigned int qp_depth)
{
	return (qp_depth && (!(qp_depth & (qp_depth-1)))) && (qp_depth >=2);
}

int xib_alloc_ucontext(struct ib_ucontext *uctx, struct ib_udata *udata)
{
	struct ib_device *ib_dev = uctx->device;
	struct xilinx_ib_dev *xib = get_xilinx_dev(ib_dev);
	struct xrnic_local *xl = xib->xl;
	struct xib_ib_alloc_ucontext_resp resp;
	size_t min_len;
	int ret = 0;

	dev_dbg(&xib->ib_dev.dev, "%s : <---------- \n", __func__);

	resp.qp_tab_size = xib->dev_attr.max_qp;

	resp.db_pa = (u64)xl->db_pa;
	resp.db_size = xl->db_size;
	resp.cq_ci_db_pa = (u64)xl->qp1_sq_db_p;
	resp.rq_pi_db_pa = (u64)xl->qp1_rq_db_p;

	resp.cq_ci_db_size = PAGE_SIZE;
	resp.rq_pi_db_size = PAGE_SIZE;

	dev_dbg(&xib->ib_dev.dev, "%s: db_pa: %llx db_size: %x\n", __func__, resp.db_pa,
			resp.db_size);

	min_len = min_t(size_t, sizeof(struct xib_ib_alloc_ucontext_resp),
			udata->outlen);
	ret = ib_copy_to_udata(udata, &resp, min_len);
	if (ret)
		return ret;

	return 0;
}

static void xib_dealloc_ucontext(struct ib_ucontext *ibucontext)
{
	struct xib_ucontext *context = get_xib_ucontext(ibucontext);

	dev_dbg(&ibucontext->device->dev, "%s : <---------- \n", __func__);

}

static int xib_mmap(struct ib_ucontext *ibucontext,
			 struct vm_area_struct *vma)
{
	struct xilinx_ib_dev *xib = get_xilinx_dev(ibucontext->device);
	struct xrnic_local *xl = xib->xl;
	u64 pfn;
	ssize_t length;

	if (((vma->vm_end - vma->vm_start) % PAGE_SIZE) != 0)
		return -EINVAL;

	length = vma->vm_end - vma->vm_start;

	dev_dbg(&xib->ib_dev.dev, "%s : pg_off: %x length: %x \n", __func__, vma->vm_pgoff, length);
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	if (io_remap_pfn_range(vma,
				vma->vm_start,
				vma->vm_pgoff,
			       length,
			       vma->vm_page_prot)) {
		dev_err(&xib->ib_dev.dev, "Failed to map device memory");
		return -EAGAIN;
	}
	return 0;
}

int xib_map_mr_sge(struct ib_mr *ibmr, struct scatterlist *sg, int sg_nents,
			 unsigned int *sg_offset)

{
        struct xilinx_ib_dev *xib = ibdev;
	struct xib_mr *mr = get_xib_mr(ibmr);
	u64 pa = sg->dma_address;
	u64 va = sg->offset;
	u64 vaddr = va;
	u64 length = sg_dma_len(sg);
	int ret;
#ifndef CONFIG_64BIT	
	if (pa < 0xFFFFFFFF) {
		vaddr = (u32)(uintptr_t)va;
	}
#endif
	if (sg == NULL) {
		pr_err("%s: scatterlist is NULL\n", __func__);
		goto fail;
	}

	if(sg_nents > 1) {
		pr_err("%s: SGE entries cannot be more than 1\n", __func__);
		goto fail;
	}
	ret = xrnic_reg_mr(xib, vaddr, length, &pa, sg_nents, mr->pd, mr->mr_idx, mr->rkey);
	if (ret) {
		pr_err("%s:Failed to register MR\n", __func__);
		goto fail;
	}
	return ret;
fail:
	spin_lock_bh(&xib->lock);
	xib_bmap_release_id(&xib->mr_map, mr->mr_idx);
	spin_unlock_bh(&xib->lock);
	kfree(mr);
	return -1;
}

struct ib_mr *xib_alloc_mr(struct ib_pd *ibpd,
				enum ib_mr_type mr_type,
				u32 max_num_sg)
{
        struct xilinx_ib_dev *xib = ibdev;
	struct xib_mr *mr = NULL;
	struct xib_pd *pd = get_xib_pd(ibpd);
	u32 mr_idx;
	u8 rkey;
	int ret;

	mr = kzalloc(sizeof(*mr), GFP_KERNEL);
	if (!mr) {
		pr_err("Failed to allocate memory for mr\n");
		return NULL;
	}

	spin_lock_bh(&xib->lock);
	ret = xib_bmap_alloc_id(&xib->mr_map, &mr_idx);
	spin_unlock_bh(&xib->lock);
	if (ret < 0)
		goto fail;

	get_random_bytes(&rkey, sizeof(rkey));
	/* Alloc mr pointer */
	mr->ib_mr.lkey = (mr_idx << 8) | rkey;
	mr->ib_mr.rkey = (mr_idx << 8) | rkey;
	mr->mr_idx = mr_idx;
	mr->rkey = rkey;
	mr->pd = pd->pdn;
	mr->ib_mr.device = &xib->ib_dev;

	mr->type = XIB_MR_USER;
	return &mr->ib_mr;
fail:
	kfree(mr);
	return NULL;
}

static int xib_alloc_pd(struct ib_pd *ibpd,
			struct ib_udata *udata)
{
	struct ib_device *ibdev = ibpd->device;
	struct xilinx_ib_dev *xib = get_xilinx_dev(ibdev);
	struct xib_pd *pd = get_xib_pd(ibpd);
	int ret;
	struct xib_ucontext *context = rdma_udata_to_drv_context(
				udata, struct xib_ucontext, ib_uc);

	/* TODO we need to lock to protect allocations */
	spin_lock_bh(&xib->lock);
	ret = xib_bmap_alloc_id(&xib->pd_map, &pd->pdn);
	spin_unlock_bh(&xib->lock);
	if (ret < 0)
		return ret;

	dev_dbg(&xib->ib_dev.dev, "%s : pd: %d \n", __func__, pd->pdn);
	if (udata && context) {
		struct xib_ib_alloc_pd_resp uresp;

		uresp.pdn = pd->pdn;
		/* TODO check udata->outlen ? */
		ret = ib_copy_to_udata(udata, &uresp, sizeof(uresp));
		if (ret)
			goto err;
	}

	return 0;
err:
	spin_lock_bh(&xib->lock);
	xib_bmap_release_id(&xib->pd_map, pd->pdn);
	spin_unlock_bh(&xib->lock);
	return ret;
}

static int xib_dealloc_pd(struct ib_pd *ibpd, struct ib_udata * udata)
{
	struct xilinx_ib_dev *xib = get_xilinx_dev(ibpd->device);
	struct xib_pd *pd = get_xib_pd(ibpd);

	dev_dbg(&xib->ib_dev.dev, "%s : <---------- \n", __func__);

	xib_bmap_release_id(&xib->pd_map, pd->pdn);
	
	/* TODO tell hw about dealloc? */
	return 0;
}

/*u8 port 编译不通过 更改为u32,函数定义在netfiliter/x_tables.h*/
static enum rdma_link_layer xib_get_link_layer(struct ib_device *device,
						    u32 port_num)
{
	return IB_LINK_LAYER_ETHERNET;
}

/* Device */
/*u8 port 编译不通过 更改为u32,函数定义在netfiliter/x_tables.h*/
struct net_device *xib_get_netdev(struct ib_device *ibdev, u32 port_num)
{
	struct xilinx_ib_dev *xib = get_xilinx_dev(ibdev);
	struct net_device *netdev = NULL;

	dev_dbg(&ibdev->dev, "%s : <---------- \n", __func__);

	rcu_read_lock();
	if (xib)
		netdev = xib->netdev;
	/* dev_put shall be called by whoever calls get_netdev */
	if (netdev)
		dev_hold(netdev);

	rcu_read_unlock();
	return netdev;
}

static int xib_query_device(struct ib_device *ibdev,
				struct ib_device_attr *props,
				struct ib_udata *uhw)
{
	struct xilinx_ib_dev *xib = get_xilinx_dev(ibdev);

	dev_dbg(&xib->ib_dev.dev, "%s : <---------- \n", __func__);

	memset(props, 0, sizeof(*props));
#if 1
	props->max_qp		= app_qp_cnt;
#else
	props->max_qp		= xib->dev_attr.max_qp;
#endif
	props->max_send_sge	= xib->dev_attr.max_send_sge;
	props->max_sge_rd	= xib->dev_attr.max_send_sge;
#if 1
	props->max_qp_wr	= app_qp_depth;
	props->max_recv_sge	= max_rq_sge;
#else
	props->max_qp_wr	= 32;
#endif
	props->max_pd		= xib->dev_attr.max_pd;
	/* TODO ernic doesnt support scatter list
	 * in mr, restrict mr to 1 page
	 */
	props->max_mr		= xib->dev_attr.max_mr;
	props->atomic_cap	= IB_ATOMIC_NONE;

	props->device_cap_flags    = IB_DEVICE_CHANGE_PHY_PORT |
				IB_DEVICE_PORT_ACTIVE_EVENT |
				IB_DEVICE_RC_RNR_NAK_GEN;

#if 1
	props->max_cq		= app_qp_depth;
#else
	props->max_cq		= xib->dev_attr.max_qp - 1;
#endif
	props->max_cqe		= xib->dev_attr.max_cq_wqes;
	props->max_pkeys	= 1;
	props->max_qp_rd_atom   = 0x10; /* TODO how to arrive at these */
	props->max_qp_init_rd_atom = 0x10;

	return 0;
}

/*不使用umm.c，删除IB_QPT_GSI、xib_create_kernel_qp*/
struct ib_qp *xib_create_qp(struct ib_pd *pd,
				struct ib_qp_init_attr *init_attr,
				struct ib_udata *udata)
{
	struct xilinx_ib_dev *xib = get_xilinx_dev(pd->device);
	struct xrnic_local *xl = xib->xl;
	struct ib_qp *ibqp;
	struct xib_qp *qp;
	u32 val;

	dev_dbg(&pd->device->dev, "%s : <---------- \n", __func__);

	if (init_attr->srq)
		return ERR_PTR(-EINVAL);

	switch (init_attr->qp_type) {
	//case IB_QPT_GSI:
		//return xib_gsi_create_qp(pd, init_attr); 
	case IB_QPT_RC:
	if (!check_qp_depths(init_attr->cap.max_send_wr)) {
		dev_err(&pd->device->dev, "qp depth should be a power of 2\n");
		return ERR_PTR(-EINVAL);
	}
	if (!check_qp_depths(init_attr->cap.max_recv_wr)) {
		dev_err(&pd->device->dev, "qp depth should be a power of 2\n");
		return ERR_PTR(-EINVAL);
	}
	if (udata) 
		//ibqp = xib_create_user_qp(pd, init_attr, udata);
		// else
		// 	ibqp = xib_create_kernel_qp(pd, init_attr);
/* 
 * AR# 75247: Initialize STAT_QPN.curr_rnr_retry_cnt and curr_retry_cnt
 */
		qp = get_xib_qp(ibqp);
		val = xrnic_ior(xl, XRNIC_STAT_QP(qp->hw_qpn));
		if ((val >> 24) != 0x77) {
			xrnic_iow(xl, XRNIC_STAT_QP(qp->hw_qpn), val | (0x77 << 24));
			wmb();
		}
		return ibqp;
	default:
		 	dev_err(&pd->device->dev, "unsupported qp type %d\n",
		 		    init_attr->qp_type);
			/* Don't support raw QPs */
		 	return ERR_PTR(-EINVAL);
		}
}

/*u8 port 编译不通过 更改为u32,函数定义在netfiliter/x_tables.h*/
int xib_query_port(struct ib_device *ibdev, u32 port,
		       struct ib_port_attr *props)
{
	struct xilinx_ib_dev *xib = get_xilinx_dev(ibdev);

	dev_dbg(&ibdev->dev, "%s : port: %d <---------- \n", __func__, port);

	memset(props, 0, sizeof(struct ib_port_attr));

	if (netif_running(xib->netdev) && netif_carrier_ok(xib->netdev)) {
		props->state = IB_PORT_ACTIVE;
		props->phys_state = 5;
	} else {
		props->state = IB_PORT_DOWN;
		props->phys_state = 3;
	}

	props->gid_tbl_len = 128; /* TODO */
	props->max_mtu = IB_MTU_4096;
	props->lid = 0;
	props->lmc = 0;
	props->sm_lid = 0;
	props->sm_sl = 0;
	props->active_mtu = iboe_get_mtu(xib->netdev->mtu);
	props->port_cap_flags = IB_PORT_CM_SUP | IB_PORT_REINIT_SUP |
				IB_PORT_DEVICE_MGMT_SUP |
				IB_PORT_VENDOR_CLASS_SUP;

	props->active_speed = xib->active_speed;
	props->active_width = xib->active_width;
	props->max_msg_sz = 0x80000000;
	props->bad_pkey_cntr = 0;
	props->qkey_viol_cntr = 0;
	props->subnet_timeout = 0;
	props->init_type_reply = 0;
	props->pkey_tbl_len = 1; /* TODO is it 1? */

	return 0;
}

/*u8 port 编译不通过 更改为u32,函数定义在netfiliter/x_tables.h*/
#define PKEY_ID	0xffff
static int xib_query_pkey(struct ib_device *ibdev, u32 port, u16 index,
			      u16 *pkey)
{
	dev_dbg(&ibdev->dev, "%s : <---------- \n", __func__);
	*pkey = PKEY_ID; /* TODO */
	return 0;
}

#define XIB_MAX_PORT	1
int xib_add_gid(const struct ib_gid_attr *attr, void **context)
{
	if (!rdma_cap_roce_gid_table(attr->device, attr->port_num))
		return -EINVAL;

	if (attr->port_num > XIB_MAX_PORT)
		return -EINVAL;

	if (!context)
		return -EINVAL;

	return 0;
}

int xib_del_gid(const struct ib_gid_attr *attr, void **context)
{
	if (!rdma_cap_roce_gid_table(attr->device, attr->port_num))
		return -EINVAL;

	if (attr->port_num > XIB_MAX_PORT)
		return -EINVAL;

	if (!context)
		return -EINVAL;

	return 0;
}

int xib_create_ah(struct ib_ah *ibah, struct rdma_ah_init_attr *ah_attr,
				struct ib_udata *udata)
{
	struct xib_ah *ah = get_xib_ah(ibah);

	ah->attr = *ah_attr;
	return 0;
}


int xib_destroy_ah(struct ib_ah *ib_ah, uint32_t flags)
{
	struct xib_ah *ah = get_xib_ah(ib_ah);

	dev_dbg(&ib_ah->device->dev, "%s : <---------- \n", __func__);
	return 0;

}


static const struct ib_device_ops xib_dev_ops = {
    .owner	= THIS_MODULE,
	// .driver_id = RDMA_DRIVER_XLNX,
    .uverbs_abi_ver	= 1,

    .query_device	= xib_query_device,
    .query_port	= xib_query_port,
    .query_pkey	= xib_query_pkey,
    .alloc_ucontext	= xib_alloc_ucontext,
    .dealloc_ucontext = xib_dealloc_ucontext,
    .mmap	= xib_mmap,

    .add_gid	= xib_add_gid,
	.del_gid	= xib_del_gid,
    .alloc_pd	= xib_alloc_pd,
	.alloc_mr	= xib_alloc_mr, 
	.map_mr_sg	= xib_map_mr_sge,
	.dealloc_pd	= xib_dealloc_pd,	
	.get_link_layer	= xib_get_link_layer,
	.get_netdev	= xib_get_netdev,

	.create_ah	= xib_create_ah,	
	.destroy_ah	= xib_destroy_ah,
	//.create_qp	= xib_create_qp,

};





static struct xilinx_ib_dev *xib_add(struct xib_dev_info *dev_info)
{

    return NULL;
}

static void xib_remove(struct xilinx_ib_dev *dev)
{

}

static struct xib_driver xib_driver = {
    .name = "xib driver",
    .add = xib_add,
    .remove = xib_remove,
    .xt_abi_version = XT_XIB_ROCE_ABI_VERSION,
};

static int __init xtic_ib_init(void)
{
    int status;
    xt_printk("%s\n", __func__);

    status = xt_roce_register_driver(&xib_driver);
    if (status)
		goto err_be_reg;

    return 0;

err_be_reg:

	return status;
}

static void __exit xtic_ib_exit(void)
{
    xt_printk("%s\n", __func__);
    xt_roce_unregister_driver(&xib_driver);
}

module_init(xtic_ib_init);
module_exit(xtic_ib_exit);

MODULE_AUTHOR("XTIC Corporation,<xtic@xtic.com>");
MODULE_DESCRIPTION("XTIC ERNIC IB driver");
MODULE_LICENSE("GPL");


