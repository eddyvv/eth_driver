#include <linux/module.h>
// #include <rdma/ib_umem.h>
#include <rdma/ib_cache.h>
#include <rdma/ib_pack.h>
#include <rdma/ib_mad.h>
#include <linux/etherdevice.h>
#include <asm/page.h>
#include "xib.h"
#include "ib_verbs.h"
#include "xib-abi.h"

/* In existing code, instead of using the user space buffers
 * for RDMA_READ, the code uses SGL memory configured when create qp is
 * called, and thus the data rx in READ response is being filled in the
 * SGL memory but not in the user space buffer directly. The fix, that's
 * being made here works only for 32Bit machines */

#define XIB_SGL_FIX 1
#define IB_QP_CREATE_HW_OFLD IB_QP_CREATE_RESERVED_START

int xib_bmap_alloc(struct xib_bmap *bmap, u32 max_count, char *name)
{
	unsigned long *bitmap;

	bitmap = kcalloc(BITS_TO_LONGS(max_count), sizeof(long),
			GFP_KERNEL);
	if(!bitmap)
		return -ENOMEM;

	bmap->bitmap = bitmap;
	bmap->max_count = max_count;
	snprintf(bmap->name, XIB_MAX_BMAP_NAME, "%s", name);

	return 0;
}


int xib_bmap_alloc_id(struct xib_bmap *bmap, u32 *id_num)
{
	*id_num = find_first_zero_bit(bmap->bitmap, bmap->max_count);
	if (*id_num >= bmap->max_count)
		return -EINVAL;

	__set_bit(*id_num, bmap->bitmap);

	return 0;
}

void xib_bmap_release_id(struct xib_bmap *bmap, u32 id_num)
{
	bool b_acquired;

	b_acquired = test_and_clear_bit(id_num, bmap->bitmap);
#if 0
	if (!b_acquired) {
		dev_err(NULL, "%s bitmap: id %d already released\n",
				bmap->name, id_num);
		return;
	}
#endif

/*
 * 
 */
void xib_qp_add(struct xilinx_ib_dev *xib, struct xib_qp *qp)
{
	xib->qp_list[qp->hw_qpn] = qp;
}

// static int xib_alloc_gsi_qp_buffers(struct ib_device *ibdev, struct xib_qp *qp)
// {
// 	struct xilinx_ib_dev *xib = get_xilinx_dev(ibdev);
// 	struct xrnic_local *xl = xib->xl;
// 	struct xib_rq *rq = &qp->rq;
// 	dma_addr_t db_addr;
// 	char *from;

// 	/* TODO 256bit aligned
// 	 * allocate RQ buffer
// 	 */
// 	rq->rq_ba_v = xib_zalloc_coherent(xib_get_rq_mem(), xib,
// 					(qp->rq.max_wr * qp->rq_buf_size),
// 					&rq->rq_ba_p, GFP_KERNEL );
// 	if (!rq->rq_ba_v) {
// 		dev_err(&ibdev->dev, "failed to alloc rq dma mem\n");
// 		goto fail_rq;
// 	}
// 	//dev_dbg(&xib->ib_dev.dev, "%s: rq_ba_v: %px rq_ba_p : %x qpn: %d", __func__, rq->rq_ba_v,
// 	printk("%s: rq_ba_v: %px rq_ba_p : %x qpn: %d", __func__, rq->rq_ba_v,
// 			rq->rq_ba_p, qp->hw_qpn);

// 	xrnic_iow(xl, XRNIC_RCVQ_BUF_BASE_LSB(qp->hw_qpn), rq->rq_ba_p);
// 	xrnic_iow(xl, XRNIC_RCVQ_BUF_BASE_MSB(qp->hw_qpn),
// 				UPPER_32_BITS(rq->rq_ba_p));
// 	wmb();

// #if 0
// 	/* dont allocate qp1 from bram
// 	 * instead use pl
// 	 * if pl not present use ps
// 	 */
// 	if (xib_pl_present())
// 		from = "pl";
// 	else
// 		from = "ps";
// #endif

// 	/* 32 bit aligned
// 	* allocate SQ WQE buffer
// 	*/
// 	qp->sq_ba_v = xib_zalloc_coherent(xib_get_sq_mem(), xib,
// 					(qp->sq.max_wr * XRNIC_SQ_WQE_SIZE),
// 					&qp->sq_ba_p, GFP_KERNEL );
// 	if (!qp->sq_ba_v) {
// 		dev_err(&ibdev->dev, "failed to alloc sq dma mem\n");
// 		goto fail_sq;
// 	}
// 	qp->send_sgl_v = xib_zalloc_coherent(xib_get_sq_mem(), xib,
// 				XRNIC_SEND_SGL_SIZE,
// 				&qp->send_sgl_p, GFP_KERNEL);
// 	if (!qp->send_sgl_v) {
// 		dev_err(&ibdev->dev, "failed to alloc sgl dma mem\n");
// 		goto fail_sgl;
// 	}
// 	//dev_dbg(&xib->ib_dev.dev, "%s: sq_ba_v: %px sq_ba_p : %x qpn: %d", __func__, qp->sq_ba_v,
// 	printk("%s: sq_ba_v: %px sq_ba_p : %x qpn: %d", __func__, qp->sq_ba_v,
// 			qp->sq_ba_p, qp->hw_qpn);

// 	xrnic_iow(xl, XRNIC_SNDQ_BUF_BASE_LSB(qp->hw_qpn), qp->sq_ba_p);
// 	xrnic_iow(xl, XRNIC_SNDQ_BUF_BASE_MSB(qp->hw_qpn),
// 				UPPER_32_BITS(qp->sq_ba_p));
// 	wmb();

// 	/* tell hw sq and rq depth */
// 	xrnic_iow(xl, XRNIC_QUEUE_DEPTH(qp->hw_qpn),
// 			(qp->sq.max_wr | (qp->rq.max_wr << 16)));
// 	wmb();

// 	//dev_dbg(&xib->ib_dev.dev, "send_sgl_p: %lx\n", qp->send_sgl_p);
// 	printk("%s: send_sgl_v: %px send_sgl_p: %llx\n", __func__, qp->send_sgl_v, qp->send_sgl_p);
// 	qp->send_sgl_busy = false;

// 	/* get pre-allocated buffer pointer */
// 	db_addr = (dma_addr_t)xrnic_get_sq_db_addr(xl, qp->hw_qpn);
// 	xrnic_iow(xl, XRNIC_CQ_DB_ADDR_LSB(qp->hw_qpn), db_addr);
// 	xrnic_iow(xl, XRNIC_CQ_DB_ADDR_MSB(qp->hw_qpn), UPPER_32_BITS(db_addr));
// 	wmb();

// 	db_addr = (dma_addr_t)xrnic_get_rq_db_addr(xl, qp->hw_qpn);
// 	xrnic_iow(xl, XRNIC_RCVQ_WP_DB_ADDR_LSB(qp->hw_qpn), db_addr);
// 	xrnic_iow(xl, XRNIC_RCVQ_WP_DB_ADDR_MSB(qp->hw_qpn),
// 			UPPER_32_BITS(db_addr));
// 	wmb();

// 	return 0;
// fail_sgl:
// 	xib_free_coherent(xib,
// 			XRNIC_SEND_SGL_SIZE,
// 			qp->send_sgl_v, qp->send_sgl_p);
// fail_sq:
// 	dma_free_coherent(&ibdev->dev,
// 			(qp->rq.max_wr * qp->rq_buf_size),
// 			rq->rq_ba_v, rq->rq_ba_p);
// fail_rq:
// 	return -ENOMEM;
// }

/*
 * 
 */
dma_addr_t get_phys_addr(uint64_t va)
{
	return va;
}

struct ib_qp *xib_create_user_qp(struct ib_pd *ib_pd,
				struct ib_qp_init_attr *init_attr,
				struct ib_udata *udata)
{
	struct xilinx_ib_dev *xib = get_xilinx_dev(ib_pd->device);
	struct xrnic_local *xl = xib->xl;
	struct xib_qp *qp;
	struct xib_pd *pd = get_xib_pd(ib_pd);
	struct xib_ib_create_qp_resp uresp;
	struct xib_ib_create_qp req;
	dma_addr_t db_addr;
	int ret;
	size_t min_len;

	qp = kzalloc(sizeof(*qp), GFP_KERNEL);
	if (!qp)
		return ERR_PTR(-ENOMEM);

	spin_lock_bh(&xib->lock);
	ret = xib_bmap_alloc_id(&xib->qp_map, &qp->hw_qpn);
	spin_unlock_bh(&xib->lock);

	if (ret < 0) {
		pr_err("%s : QPs are not available\n", __func__);
		return ERR_PTR(-EFAULT);
	}

	ret = ib_copy_from_udata((void *)&req, udata, sizeof(req));

	dev_dbg(&xib->ib_dev.dev, "%s : qpn: %d \n", __func__, qp->hw_qpn);
	qp->is_ipv6 = false;
	qp->sq.max_wr = init_attr->cap.max_send_wr;
	qp->rq.max_wr = init_attr->cap.max_recv_wr;
	qp->rq_buf_size = init_attr->cap.max_recv_sge * XRNIC_RQ_BUF_SGE_SIZE;

	qp->sq_cq = get_xib_cq(init_attr->send_cq);
	qp->sq_cq->cq_type = XIB_CQ_TYPE_USER;

	qp->rq.rq_ba_p = get_phys_addr(req.rq_ba);
	xrnic_iow(xl, XRNIC_RCVQ_BUF_BASE_LSB(qp->hw_qpn), qp->rq.rq_ba_p);
	xrnic_iow(xl, XRNIC_RCVQ_BUF_BASE_MSB(qp->hw_qpn),
					UPPER_32_BITS(qp->rq.rq_ba_p));

	qp->sq_ba_p = get_phys_addr(req.sq_ba);
	xrnic_iow(xl, XRNIC_SNDQ_BUF_BASE_LSB(qp->hw_qpn), qp->sq_ba_p);
	xrnic_iow(xl, XRNIC_SNDQ_BUF_BASE_MSB(qp->hw_qpn),
					UPPER_32_BITS(qp->sq_ba_p));

	xrnic_iow(xl, XRNIC_QUEUE_DEPTH(qp->hw_qpn),
			(qp->sq.max_wr | (qp->rq.max_wr << 16)));

	/* get imm base */
	db_addr = xrnic_get_sq_db_addr(xl, qp->hw_qpn);
	xrnic_iow(xl, XRNIC_CQ_DB_ADDR_LSB(qp->hw_qpn), db_addr);
	xrnic_iow(xl, XRNIC_CQ_DB_ADDR_MSB(qp->hw_qpn), UPPER_32_BITS(db_addr));
	wmb();

	db_addr = xrnic_get_rq_db_addr(xl, qp->hw_qpn);
	xrnic_iow(xl, XRNIC_RCVQ_WP_DB_ADDR_LSB(qp->hw_qpn), db_addr);
	xrnic_iow(xl, XRNIC_RCVQ_WP_DB_ADDR_MSB(qp->hw_qpn),
					UPPER_32_BITS(db_addr));
	wmb();

	qp->rq.rq_wrptr_db_local = 0;
	qp->rq.rq_ci_db_local = 0;

	qp->sq.sq_cmpl_db_local = 0;
	qp->sq.send_cq_db_local = 0;

	/* assign the pd to qp */
	xrnic_qp_set_pd(xib, qp->hw_qpn, pd->pdn);

	/* there is no QP0 in ernic
	 * so QP1 index starts with 0
	 */
	qp->ib_qp.qp_num = qp->hw_qpn + 1; 
	qp->qp_type	 = XIB_QP_TYPE_USER;

	/* program the send cq base */
	qp->sq_cq->buf_p = get_phys_addr(req.cq_ba);
	xrnic_iow(xl, XRNIC_CQ_BUF_BASE_LSB(qp->hw_qpn), qp->sq_cq->buf_p);
	xrnic_iow(xl, XRNIC_CQ_BUF_BASE_MSB(qp->hw_qpn),
				UPPER_32_BITS(qp->sq_cq->buf_p));
	wmb();

	qp->imm_inv_data = __va(req.imm_inv_ba);
	uresp.qpn = qp->hw_qpn;
	min_len = min_t(size_t, sizeof(struct xib_ib_create_qp_resp), udata->outlen);
	ib_copy_to_udata(udata, &uresp, min_len);

	qp->state = XIB_QP_STATE_RESET;
	xib_qp_add(xib, qp);
	tasklet_init(&qp->fatal_hdlr_task, xib_fatal_handler,
				(unsigned long)qp);
	tasklet_init(&qp->cnp_task, xib_cnp_handler,
			(unsigned long)qp);
	return &qp->ib_qp;
}

// struct ib_qp *xib_gsi_create_qp(struct ib_pd *pd,
// 				struct ib_qp_init_attr *init_attr)
// {
// 	struct xilinx_ib_dev *xib = get_xilinx_dev(pd->device);
// 	struct xrnic_local *xl = xib->xl;
// 	struct xib_qp *qp;
// 	int ret;

// 	dev_dbg(&xib->ib_dev.dev, " %s <----------- \n", __func__);

// 	dev_dbg(&xib->ib_dev.dev, "%s: send_cq %px recv_cq: %px \n", __func__, init_attr->send_cq,
// 			init_attr->recv_cq);

// 	qp = kzalloc(sizeof(*qp), GFP_KERNEL);
// 	if (!qp)
// 		return ERR_PTR(-ENOMEM);

// 	qp->hw_qpn = 0; /* QP1 */
// 	qp->qp_type = XIB_QP_TYPE_GSI;
// 	qp->rq.rq_wrptr_db_local = 0;
// 	qp->rq.rq_ci_db_local = 0;

// 	qp->sq.sq_cmpl_db_local = 0;
// 	qp->sq.send_cq_db_local = 0;

// 	#if 1
// 	qp->rq.max_wr = init_attr->cap.max_recv_wr;
// 	qp->sq.max_wr = init_attr->cap.max_send_wr;
// 	#else
// 	qp->rq.max_wr = XRNIC_GSI_RQ_DEPTH;
// 	qp->sq.max_wr = XRNIC_GSI_SQ_DEPTH;
// 	#endif
// 	qp->is_ipv6 = false;
// 	qp->rq_buf_size = XRNIC_GSI_RECV_PKT_SIZE;

// 	qp->rq.rqe_list = kcalloc(qp->rq.max_wr, sizeof(struct xib_rqe),
// 				GFP_KERNEL);
// 	if (!qp->rq.rqe_list)
// 		goto fail_1;

// 	/* xrnic only supports 16bit wrid 
// 	 * array to store 64bit wrid from the stack 
// 	 */
// 	qp->sq.wr_id_array = kcalloc(qp->sq.max_wr, sizeof(*qp->sq.wr_id_array), GFP_KERNEL);

// 	ret = xib_alloc_gsi_qp_buffers(pd->device, qp);
// 	if (ret < 0) {
// 		dev_err(&xib->ib_dev.dev, "%s: qp buf alloc fail\n", __func__);
// 		goto fail_2;
// 	}

// 	qp->sq_cq = get_xib_cq(init_attr->send_cq);
// 	qp->sq_cq->cq_type = XIB_CQ_TYPE_GSI;

// 	/* program the send cq base */
// 	xrnic_iow(xl, XRNIC_CQ_BUF_BASE_LSB(qp->hw_qpn), qp->sq_cq->buf_p);
// 	xrnic_iow(xl, XRNIC_CQ_BUF_BASE_MSB(qp->hw_qpn),
// 					UPPER_32_BITS(qp->sq_cq->buf_p));
// 	wmb();

// 	qp->rq_cq = get_xib_cq(init_attr->recv_cq);
// 	qp->rq_cq->cq_type = XIB_CQ_TYPE_GSI;

// 	qp->ib_qp.qp_num = 1;

// 	xib->gsi_qp = qp;

// 	tasklet_init(&qp->comp_task, xib_gsi_comp_handler, 
// 			(unsigned long)xib);

// 	spin_lock_init(&qp->sq_lock);
// 	spin_lock_init(&qp->rq_lock);

// 	qp->state = XIB_QP_STATE_RESET;

// 	qp->imm_inv_data = kcalloc(qp->rq.max_wr, sizeof(struct xib_imm_inv),
// 				GFP_KERNEL);
// 	if (!qp->imm_inv_data)
// 		goto fail_2;

// 	xib_qp_add(xib, qp);
// 	tasklet_init(&qp->cnp_task, xib_cnp_handler,
// 			(unsigned long)qp);

// 	return &qp->ib_qp;
// fail_2:
// 	kfree(qp->rq.rqe_list);
// fail_1:
// 	kfree(qp);
// 	return ERR_PTR(-ENOMEM);

// }

}
