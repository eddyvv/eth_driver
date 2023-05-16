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
    xib_printfunc("%s start\n", __func__);
	*id_num = find_first_zero_bit(bmap->bitmap, bmap->max_count);
	if (*id_num >= bmap->max_count)
		return -EINVAL;

	__set_bit(*id_num, bmap->bitmap);
    xib_printfunc("%s start\n", __func__);
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
}
/*
 *
 */
void xib_qp_add(struct xilinx_ib_dev *xib, struct xib_qp *qp)
{
	xib->qp_list[qp->hw_qpn] = qp;
}

static int xib_get_user_qp_area_sz(struct xib_qp *qp)
{
	int total_uarea;
	unsigned int size = 0;
	struct xib_cq *sq_cq = qp->sq_cq;

	if (!sq_cq) {
		printk("Invalid CQ association to QP RQ %s:%d\n",
				__func__, __LINE__);
		return -EFAULT;
	}

	total_uarea = qp->sq.max_wr * XRNIC_SQ_WQE_SIZE;
	total_uarea += qp->rq.max_wr * qp->rq_buf_size;
#if 1
	total_uarea += qp->sq.max_wr* CQE_SIZE;
#else
	total_uarea += sq_cq->ib_cq.cqe * CQE_SIZE;
#endif
	total_uarea += qp->rq.max_wr * sizeof(struct xib_imm_inv);
	size = total_uarea;
	size = (total_uarea & ((1 << 12) - 1));
	size = ((1 << 12) - size);
	total_uarea += size;
	total_uarea += qp->rq_buf_size;
	return total_uarea;
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
// 	dma_free_coherent(&ibdev->dev,
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

/*
 * rest rq for nvmf use case
 * with Hardware handshake
 */
int xib_rst_rq(struct xib_qp *qp)
{
	struct xilinx_ib_dev *xib = get_xilinx_dev(qp->ib_qp.device);
	struct xrnic_local *xl = xib->xl;
	struct xib_rq *rq = &qp->rq;
	u32 config;

	/* Enable SW override enable */
	xrnic_iow(xl, XRNIC_ADV_CONF, 1);
	wmb();

	xrnic_iow(xib->xl, XRNIC_RQ_CONS_IDX(qp->hw_qpn), 0);
	xrnic_iow(xib->xl, XRNIC_STAT_RQ_PROD_IDX(qp->hw_qpn), 0);

	xrnic_iow(xl, XRNIC_RCVQ_BUF_BASE_LSB(qp->hw_qpn), rq->rq_ba_p);
	xrnic_iow(xl, XRNIC_RCVQ_BUF_BASE_MSB(qp->hw_qpn),
					UPPER_32_BITS(rq->rq_ba_p));
	wmb();

	config = (QP_ENABLE | QP_HW_HSK_DIS | QP_CQE_EN);
	config |= QP_RQ_IRQ_EN | QP_CQ_IRQ_EN;

	/* TODO set pmtu ? */
	config |= (QP_PMTU_4096 << QP_PMTU_SHIFT);
	/* rq buf size in multiple of 256 */
	config |= ((qp->rq_buf_size >> 8) << QP_RQ_BUF_SZ_SHIFT);
	xrnic_iow(xl, XRNIC_QP_CONF(qp->hw_qpn), config);
	wmb();

	/* Disable SW override enable */
	xrnic_iow(xl, XRNIC_ADV_CONF, 0);
	wmb();

	/* TODO do we need these dummy reads ?*/
	xrnic_ior(xib->xl, XRNIC_RQ_CONS_IDX(qp->hw_qpn));
	xrnic_ior(xib->xl, XRNIC_STAT_RQ_PROD_IDX(qp->hw_qpn));

	xrnic_ior(xl, XRNIC_RCVQ_BUF_BASE_LSB(qp->hw_qpn));
	xrnic_ior(xl, XRNIC_RCVQ_BUF_BASE_MSB(qp->hw_qpn));

	return 0;
}

/*
 * rest sq and cq for nvmf use case
 * with Hardware handshake
 */
int xib_rst_cq_sq(struct xib_qp *qp, int nvmf_rhost)
{
	struct xilinx_ib_dev *xib = get_xilinx_dev(qp->ib_qp.device);
	struct xrnic_local *xl = xib->xl;
	dma_addr_t db_addr;
	u32 config, cnct_io_cnf;

	/* Enable SW override enable */
	xrnic_iow(xl, XRNIC_ADV_CONF, 1);

	xrnic_iow(xl, XRNIC_CQ_HEAD_PTR(qp->hw_qpn), 0);
	xrnic_iow(xl, XRNIC_SQ_PROD_IDX(qp->hw_qpn), 0);
	xrnic_iow(xl, XRNIC_STAT_CUR_SQ_PTR(qp->hw_qpn), 0);
	wmb();

	db_addr = 0x8E004000 + nvmf_rhost*4;
	xrnic_iow(xl, XRNIC_RCVQ_WP_DB_ADDR_LSB(qp->hw_qpn), db_addr);
	xrnic_iow(xl, XRNIC_RCVQ_WP_DB_ADDR_MSB(qp->hw_qpn),
						UPPER_32_BITS(db_addr));
	wmb();

	cnct_io_cnf = db_addr & 0xffff;

	db_addr = 0x8E004400 + nvmf_rhost*4;
	xrnic_iow(xl, XRNIC_CQ_DB_ADDR_LSB(qp->hw_qpn), db_addr);
	xrnic_iow(xl, XRNIC_CQ_DB_ADDR_MSB(qp->hw_qpn), UPPER_32_BITS(db_addr));
	wmb();

	config = xrnic_ior(xl, XRNIC_STAT_RQ_PROD_IDX(qp->hw_qpn));

	cnct_io_cnf |= (config & 0xffff) << 16;
	xrnic_iow(xl, XRNIC_CNCT_IO_CONF, cnct_io_cnf);


	/* enable HW Handshake */
	config = QP_ENABLE;
	config |= (QP_PMTU_4096 << QP_PMTU_SHIFT);
	/* rq buf size in multiple of 256 */
	config |= ((qp->rq_buf_size >> 8) << QP_RQ_BUF_SZ_SHIFT);
	xrnic_iow(xl, XRNIC_QP_CONF(qp->hw_qpn), config);
	wmb();

	/* Disable SW override enable */
	xrnic_iow(xl, XRNIC_ADV_CONF, 0);

	return 0;
}

int xib_dealloc_qp_buffers(struct ib_device *ibdev, struct xib_qp *qp)
{
	struct xib_rq *rq = &qp->rq;

	/* free sq wqe entries */
	dma_free_coherent(&ibdev->dev,
		(qp->sq.max_wr * XRNIC_SQ_WQE_SIZE),
		qp->sq_ba_v, qp->sq_ba_p);

	/* free rq buffers */
	dma_free_coherent(&ibdev->dev,
		(qp->rq.max_wr * qp->rq_buf_size),
		rq->rq_ba_v, rq->rq_ba_p);
	return 0;
}

int xib_dealloc_user_qp_buffers(struct ib_device *ibdev, struct xib_qp *qp)
{
	dma_free_coherent(&ibdev->dev,
			xib_get_user_qp_area_sz(qp),
			qp->ua_v, qp->ua_p);
	return 0;
}

int xib_dealloc_gsi_qp_buffers(struct ib_device *ibdev, struct xib_qp *qp)
{
	/* free sgl */
	dma_free_coherent(&ibdev->dev, XRNIC_SEND_SGL_SIZE,
		qp->send_sgl_v, qp->send_sgl_p);

	xib_dealloc_qp_buffers(ibdev, qp);
	return 0;
}

/*
 *
 */
int xib_build_qp1_send_v2(struct ib_qp *ib_qp,
			const struct ib_send_wr *wr,
			int payload_sz,
			bool *is_udp,
			u8 *ip_version)
{
	struct xib_qp *qp = get_xib_qp(ib_qp);
	struct rdma_ah_init_attr *ah_init_attr = &get_xib_ah(ud_wr(wr)->ah)->attr;
	const struct ib_global_route *grh = rdma_ah_read_grh(ah_init_attr->ah_attr);
	struct xilinx_ib_dev *xib = get_xilinx_dev(ib_qp->device);
	const struct ib_gid_attr *sgid_attr = grh->sgid_attr;
	u16 ether_type;
	bool is_eth = true;
	bool is_grh = false;
	// int ret;

	dev_dbg(&xib->ib_dev.dev, "%s <---------- \n", __func__);

/* TODO : Getting sgid_attr as NULL for the client-Connect Request if the
 *        server is not initiated in ERNIC. This occurs in ARM architect,
 *        works fine in microblaze. This NULL check is the work around.
 */
	if (!sgid_attr) {
		//dev_err(&xib->ib_dev.dev, "%s: sgid_attr is NULL\n", __func__);
		return 1;
	}
	*is_udp = sgid_attr->gid_type == IB_GID_TYPE_ROCE_UDP_ENCAP;

	if (*is_udp) {
		if (ipv6_addr_v4mapped((struct in6_addr *)&sgid_attr->gid)) {
			*ip_version = 4;
			ether_type = ETH_P_IP;
			is_grh = false;
		} else {
			*ip_version = 6;
			ether_type = ETH_P_IPV6;
			is_grh = true;
		}
	} else {
		dev_err(&xib->ib_dev.dev, "not udp!!\n");
		ether_type = ETH_P_IBOE;
		is_grh = true;
	}

	ib_ud_header_init(payload_sz, !is_eth, is_eth, false, is_grh,
			*ip_version, *is_udp, 0, &qp->qp1_hdr);

	/* ETH */
	ether_addr_copy(qp->qp1_hdr.eth.dmac_h, ah_init_attr->ah_attr->roce.dmac);
	ether_addr_copy(qp->qp1_hdr.eth.smac_h, xib->netdev->dev_addr);

	qp->qp1_hdr.eth.type = cpu_to_be16(ether_type);

	if (is_grh || (*ip_version == 6)) {
			/* copy the GIDs / IPV6 addresses */
		qp->qp1_hdr.grh.hop_limit = 64;
		qp->qp1_hdr.grh.traffic_class = 0;
		qp->qp1_hdr.grh.flow_label= 0;
		memcpy(&qp->qp1_hdr.grh.source_gid, sgid_attr->gid.raw, 16);
		memcpy(&qp->qp1_hdr.grh.destination_gid, grh->dgid.raw, 16);
	}

	if (*ip_version == 4) {
		qp->qp1_hdr.ip4.tos = 0;
		qp->qp1_hdr.ip4.id = 0;
		qp->qp1_hdr.ip4.frag_off = htons(IP_DF);
		/* qp->qp1_hdr.ip4.ttl = grh->hop_limit; */
		/* TODO check why its coming back as zero */
		qp->qp1_hdr.ip4.ttl = 64;

		memcpy(&qp->qp1_hdr.ip4.saddr, sgid_attr->gid.raw + 12, 4);
		memcpy(&qp->qp1_hdr.ip4.daddr, grh->dgid.raw + 12, 4);
		qp->qp1_hdr.ip4.check = ib_ud_ip4_csum(&qp->qp1_hdr);
	}

	if (*is_udp) {
		qp->qp1_hdr.udp.dport = htons(ROCE_V2_UDP_DPORT);
		qp->qp1_hdr.udp.sport = htons(0x8CD1);
		qp->qp1_hdr.udp.csum = 0;
	}

	/* BTH */
	if (wr->opcode == IB_WR_SEND_WITH_IMM) {
		qp->qp1_hdr.bth.opcode = IB_OPCODE_UD_SEND_ONLY_WITH_IMMEDIATE;
		qp->qp1_hdr.immediate_present = 1;
	} else {
		qp->qp1_hdr.bth.opcode = IB_OPCODE_UD_SEND_ONLY;
	}

	if (wr->send_flags & IB_SEND_SOLICITED)
		qp->qp1_hdr.bth.solicited_event = 1;

	/* pad_count */
	qp->qp1_hdr.bth.pad_count = (4 - payload_sz) & 3;

	/* P_key for QP1 is for all members */
	qp->qp1_hdr.bth.pkey = cpu_to_be16(0xFFFF);
	qp->qp1_hdr.bth.destination_qpn = IB_QP1;
	qp->qp1_hdr.bth.ack_req = 0;
	qp->send_psn++;
	qp->qp1_hdr.bth.psn = cpu_to_be32(qp->send_psn);
	/* DETH */
	/* Use the priviledged Q_Key for QP1 */
	qp->qp1_hdr.deth.qkey = cpu_to_be32(IB_QP1_QKEY);
	qp->qp1_hdr.deth.source_qpn = IB_QP1;

	return 0;
// fail:
// 	return ret;
}

/**
 * xib_drain_sq - drain sq
 * @ibqp: pointer to ibqp
 */
void xib_drain_sq(struct ib_qp *ibqp)
{
	// struct xilinx_ib_dev *xib = get_xilinx_dev(ibqp->device);
	// struct xib_qp *qp = get_xib_qp(ibqp);
    //     struct xib_sq *sq = &qp->sq;
    return ;
}

/**
 * xib_drain_rq - drain rq
 * @ibqp: pointer to ibqp
 */
void xib_drain_rq(struct ib_qp *ibqp)
{
	struct xilinx_ib_dev *xib = get_xilinx_dev(ibqp->device);
	struct xib_qp *qp = get_xib_qp(ibqp);
	struct ib_qp_attr attr = { .qp_state = IB_QPS_ERR };
	int ret, val;
	unsigned long timeout;

	ret = ib_modify_qp(ibqp, &attr, IB_QP_STATE);
	if (ret) {
		pr_warn("failed to drain sq :%d\n", ret);
		return;
	}

	/* Wait till STAT_RQ_PI_DB == RQ_CI_DB */
	timeout = jiffies;
	do {
		val = xrnic_ior(xib->xl, XRNIC_RQ_CONS_IDX(qp->hw_qpn));
		ret = xrnic_ior(xib->xl, XRNIC_STAT_RQ_PROD_IDX(qp->hw_qpn));
		if (time_after(jiffies, (timeout + 1 * HZ)))
			break;
	} while(!(val == ret));
}

/*
 *
 */
int xib_kernel_qp_post_recv(struct ib_qp *ibqp, const struct ib_recv_wr *wr,
			const struct ib_recv_wr **bad_wr)
{
	struct xib_qp *qp = get_xib_qp(ibqp);
	struct xib_rqe *rqe;
	unsigned long flags;
	int i,ret, data_len;

	spin_lock_irqsave(&qp->rq_lock, flags);

	while (wr) {
		rqe = &qp->rq.rqe_list[qp->rq.prod];
		memset(rqe, 0, sizeof(struct xib_rqe));
		data_len = xib_get_payload_size(wr->sg_list, wr->num_sge);
		if ((data_len > qp->rq_buf_size) ||
			 (wr->num_sge > XIB_MAX_RQE_SGE)) {
			ret = -ENOMEM;
			pr_err("%s: no mem, wr size: %d, num_sge: %d\n",
				__func__, data_len, wr->num_sge);
			goto fail;
		}

		//if (wr->num_sge > 1) /* suppress this warning */
		for (i = 0; i < wr->num_sge; i++)
			rqe->sg_list[i] = wr->sg_list[i];
		rqe->wr_id = wr->wr_id;
		rqe->num_sge = wr->num_sge;
		xib_rq_prod_inc(&qp->rq);

		wr = wr->next;
	}

	spin_unlock_irqrestore(&qp->rq_lock, flags);

	return 0;
fail:
	spin_unlock_irqrestore(&qp->rq_lock, flags);
	*bad_wr = wr;
	return ret;
}
