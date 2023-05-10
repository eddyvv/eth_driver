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
#include <rdma/ib_verbs.h>
#include <rdma/uverbs_ioctl.h>
#include <net/addrconf.h>
#include <linux/of_address.h>
#include <linux/jiffies.h>
#include "xib-abi.h"
#include "xtic_common.h"
#include "../eth/xt_roce.h"
#include "xib.h"
#include "ib_verbs.h"

unsigned int app_qp_cnt = 10;

unsigned int app_qp_depth = 16;

unsigned int max_rq_sge = 16;

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
};





static void xib_add(void)
{

}

static void xib_remove(void)
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
    xt_printk("%s start\n", __func__);

    status = xt_roce_register_driver(&xib_driver);
    if (status)
		goto err_be_reg;

    xt_printk("%s end\n", __func__);
    return 0;

err_be_reg:

	return status;
}

static void __exit xtic_ib_exit(void)
{
    xt_printk("%s start\n", __func__);
    xt_roce_unregister_driver(&xib_driver);
    xt_printk("%s end\n", __func__);
}

module_init(xtic_ib_init);
module_exit(xtic_ib_exit);

MODULE_AUTHOR("XTIC Corporation,<xtic@xtic.com>");
MODULE_DESCRIPTION("XTIC ERNIC IB driver");
MODULE_LICENSE("GPL");


