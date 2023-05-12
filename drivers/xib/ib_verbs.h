#ifndef _XIB_IB_VERBS_H_
#define _XIB_IB_VERBS_H_

#define SQ_BASE_ALIGN_SZ	32
#define SQ_BASE_ALIGN(addr)	ALIGN(addr, SQ_BASE_ALIGN_SZ)
#define SQ_BASE_ALIGNED(addr)	IS_ALIGNED((unsigned long)(addr), \
				SQ_BASE_ALIGN_SZ)

#define XIB_MAX_RQE_SGE		8
#define XIB_MAX_SQE_SGE		8

enum xib_cq_type {
	XIB_CQ_TYPE_GSI,
	XIB_CQ_TYPE_KERNEL,
	XIB_CQ_TYPE_USER,
};

enum xib_qp_type {
	XIB_QP_TYPE_GSI,
	XIB_QP_TYPE_KERNEL,
	XIB_QP_TYPE_USER,
};

enum xib_qp_state {
	XIB_QP_STATE_RESET,
	XIB_QP_STATE_INIT,
	XIB_QP_STATE_RTR,
	XIB_QP_STATE_RTS,
	XIB_QP_STATE_SQD,
	XIB_QP_STATE_ERR,
	XIB_QP_STATE_SQE
};


struct xib_rqe {
	u64 wr_id;
	u32		num_sge;
	u32		ip_version;
#ifdef DEBUG_IPV6
	u32		reserved;
#endif
	struct ib_sge sg_list[XIB_MAX_RQE_SGE];
};

struct xib_rq {
	void			*rq_ba_v;
	u64			rq_ba_p;
	u32			rq_wrptr_db_local;
	u32			rq_ci_db_local;
	u32			prod;
	u32			cons;
	u32			gsi_cons;
	u32			max_wr;

	/* store rqe from stack */
	struct xib_rqe		*rqe_list;
};

struct xib_sq {
	u32			sq_cmpl_db_local;
	u32			send_cq_db_local;
	struct {
		u64		wr_id;
		bool		signaled;
	} *wr_id_array;
        struct xib_sqd *sqd_wr_list;
        uint64_t        sqd_length;
	u64			*sgl_pa;
	struct xib_pl_buf	*pl_buf_list;
	u32			max_wr;
};


struct xib_cq {
	struct ib_cq		ib_cq;
	struct ib_umem		*umem;
	void			*buf_v;
	u64			buf_p;
	enum xib_cq_type	cq_type;
	struct xib_qp		*qp;
	spinlock_t		cq_lock;
};

struct qp_hw_hsk_cfg {
	u64			data_ba_p;
	void			*data_ba_va;
	u64			sq_ba_p;
	void			*sq_ba_va;
};


struct xib_qp {
	struct ib_qp		ib_qp;
	u32			sq_depth;
	u64			sq_ba_p;
	void			*sq_ba_v;

	bool			send_sgl_busy;
	bool			is_ipv6;
#ifdef DEBUG_IPV6
	bool			res1;
#endif
	u64			send_sgl_p;
	void			*send_sgl_v;

	enum xib_qp_state	state;

	spinlock_t		sq_lock;
	spinlock_t		rq_lock;

	void			*ua_v;
	dma_addr_t		ua_p;
	u32			ua_size;

	/* RQ */
	struct xib_rq		rq;

	/* SQ */
	struct xib_sq		sq;

	/* CQ */
	struct xib_cq		*sq_cq;
	struct xib_cq		*rq_cq;

	u32			hw_qpn;
	enum xib_qp_type	qp_type;

	/* QP1 */
	u32			send_psn;
	bool			io_qp;
	struct ib_ud_header	qp1_hdr;
	struct tasklet_struct	comp_task;
	struct tasklet_struct	cnp_task;
	struct qp_hw_hsk_cfg	hw_hs_cfg;
	struct tasklet_struct	fatal_hdlr_task;
	struct completion	sq_drained;
	u32			sq_polled_count;
	u32			post_send_count;
	u32			rq_buf_size;
	struct xib_imm_inv	*imm_inv_data;
};

static inline struct xib_qp *get_xib_qp(struct ib_qp *ibqp)
{
	return container_of(ibqp, struct xib_qp, ib_qp);
}

static inline struct xib_cq *get_xib_cq(struct ib_cq *ibcq)
{
	return container_of(ibcq, struct xib_cq, ib_cq);
}

struct ib_qp *xib_create_user_qp(struct ib_pd *pd,
				struct ib_qp_init_attr *init_attr,
				struct ib_udata *udata);




#endif /* _XIB_IB_VERBS_H_ */