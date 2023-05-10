#ifndef XIB_ABI_USER_H
#define XIB_ABI_USER_H

#include <linux/types.h>

#define XT_XIB_ROCE_ABI_VERSION 1
struct xib_ib_create_cq {
	__aligned_u64 buf_addr;
	__aligned_u64 db_addr;
};

struct xib_ib_create_qp {
	__aligned_u64 db_addr;
	__aligned_u64 sq_ba;
	__aligned_u64 rq_ba;
	__aligned_u64 cq_ba;
	__aligned_u64 imm_inv_ba;
};

struct xib_ib_alloc_pd_resp {
	__u32 pdn;
};

struct xib_ib_create_cq_resp {
	__aligned_u64 cqn;
	__aligned_u64 cap_flags;
};

struct xib_ib_create_qp_resp {
	__u32 qpn;
	__u32 cap_flags;
};

struct xib_ib_alloc_ucontext_resp {
	__aligned_u64 db_pa;
	__u32 db_size;
	__aligned_u64 cq_ci_db_pa;
	__aligned_u64 rq_pi_db_pa;
	uint32_t cq_ci_db_size, rq_pi_db_size;
	__u32 qp_tab_size;
};

#endif /* XIB_ABI_USER_H  */
