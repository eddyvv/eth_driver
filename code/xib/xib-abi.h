/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/*
 * Copyright (c) 2018 Xilinx Inc.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef XIB_ABI_USER_H
#define XIB_ABI_USER_H

#include <linux/types.h>

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