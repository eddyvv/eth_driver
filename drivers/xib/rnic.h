#ifndef _XRNIC_H_
#define _XRNIC_H_
#include "xtic_common.h"
#include "../eth/xt_roce.h"
#include "xib_conf.h"
#define XRNIC_INVALID_OPC -1

#define XRNIC_BUF_RKEY_MASK		(0xFF)
#define XRNIC_MR_PDNUM(mrn)		(0x00 + (mrn) * 0x100)
#define XRNIC_MR_VA_LO(mrn)		(0x04 + (mrn) * 0x100)
#define XRNIC_MR_VA_HI(mrn)		(0x08 + (mrn) * 0x100)
#define XRNIC_MR_BUF_BASE_LO(mrn)	(0x0c + (mrn) * 0x100)
#define XRNIC_MR_BUF_BASE_HI(mrn)	(0x10 + (mrn) * 0x100)
#define XRNIC_MR_BUF_RKEY(mrn)		(0x14 + (mrn) * 0x100)
#define XRNIC_MR_WRRD_BUF_LEN(mrn)	(0x18 + (mrn) * 0x100)
#define XRNIC_MR_ACC_DESC(mrn)		(0x1c + (mrn) * 0x100)

#define XRNIC_VIRT_ADDR_0(q)	(0x04 + (q) * 0x100)
#define XRNIC_VIRT_ADDR_1(q)	(0x08 + (q) * 0x100)
#define XRNIC_BUFBASE_ADDR_0(q)	(0x0c + (q) * 0x100)
#define XRNIC_BUFBASE_ADDR_1(q)	(0x10 + (q) * 0x100)
#define XRNIC_BUF_R_KEY(q)	(0x14 + (q) * 0x100)
#define XRNIC_W_R_BUF_LEN(q)	(0x18 + (q) * 0x100)
#define XRNIC_ACCESS_DESC(q)	(0x1c + (q) * 0x100)

#define XRNIC_CONF		0x20000
#define XRNIC_EN		BIT(0)
#define XRNIC_NUM_QP_SHIFT	8
#define XRNIC_NUM_QP_MASK	0xff
#define XRNIC_UDP_SPORT_SHIFT	16
#define XRNIC_UDP_SPORT_MASK	0xffff

#define XRNIC_ADV_CONF		0x20004
#define XRNIC_ROCE_PAUSE_OFFSET	0x20008
#define XRNIC_PAUSE_CONF	0x2000C
/* PFC enable & prioirty config */
#define XRNIC_PFC_GLOBAL_PRIOIRTY	8
#define XRNIC_PFC_PRIO_BIT_MASK		0xF
enum {
	XRNIC_ROCE_PFC_EN_BIT = 0,
	XRNIC_NON_ROCE_PFC_EN_BIT = 1,
	XRNIC_ROCE_PFC_PRIO_BIT = 4,
	XRNIC_NON_ROCE_PFC_PRIO_BIT = 8,
	XRNIC_DIS_PRIO_CHECK_BIT = 13,
};
#define XRNIC_MAC_ADDR_LO	0x20010
#define XRNIC_MAC_ADDR_HI	0x20014
#define XRNIC_NON_ROCE_PAUSE_OFFSET 	0x20018

#define XRNIC_IPV6_ADD_1	0x20020
#define XRNIC_IPV6_ADD_2	0x20024
#define XRNIC_IPV6_ADD_3	0x20028
#define XRNIC_IPV6_ADD_4	0x2002c

#define XRNIC_ERR_BUF_BASE_LSB	0x20060
#define XRNIC_ERR_BUF_SZ	0x20068
#define XRNIC_ERR_BUF_WR_PTR	0x2006c
#define XRNIC_IPV6_ADD_1	0x20020

#define XRNIC_IPV4_ADDR		0x20070
#define XRNIC_MR_ACC_DESC_RD_WR 0x2
/* Global status registers */
#define XRNIC_RESP_HDLR_STAT		0x2013c



#define XRNIC_SQ_PICI_DB_CHECK_EN	(1 << 16)
/* Per QP registers */
#define XRNIC_QP_CONF(q)		(0x20200 + (q) * 0x100)
  #define QP_ENABLE			BIT(0)
  #define QP_UNDER_RECOVERY		BIT(6)
#define XRNIC_RCVQ_BUF_BASE_LSB(q)	(0x20208 + (q) * 0x100)
#define XRNIC_RCVQ_BUF_BASE_MSB(q)	(0x202C0 + (q) * 0x100)
#define XRNIC_SNDQ_BUF_BASE_LSB(q)	(0x20210 + (q) * 0x100)
#define XRNIC_CQ_BUF_BASE_LSB(q)	(0x20218 + (q) * 0x100)
#define XRNIC_RCVQ_WP_DB_ADDR_LSB(q)	(0x20220 + (q) * 0x100)
#define XRNIC_CQ_DB_ADDR_LSB(q)		(0x20228 + (q) * 0x100)
#define XRNIC_CQ_DB_ADDR_MSB(q)		(0x2022C + (q) * 0x100)
#define XRNIC_CQ_HEAD_PTR(q)		(0x20230 + (q) * 0x100)
  #define XRNIC_CQ_HEAD_PTR_MASK	0xffff
#define XRNIC_RQ_CONS_IDX(q)		(0x20234 + (q) * 0x100)
#define XRNIC_SQ_PROD_IDX(q)		(0x20238 + (q) * 0x100)
#define XRNIC_QUEUE_DEPTH(q)		(0x2023c + (q) * 0x100)
#define XRNIC_SNDQ_PSN(q)		(0x20240 + (q) * 0x100)
#define XRNIC_LAST_RQ_PSN(q)		(0x20244 + (q) * 0x100)
#define XRNIC_DEST_QP_CONF(q)		(0x20248 + (q) * 0x100)
#define XRNIC_MAC_DEST_ADDR_LO(q)	(0x20250 + (q) * 0x100)
#define XRNIC_MAC_DEST_ADDR_HI(q)	(0x20254 + (q) * 0x100)

#define XRNIC_IP_DEST_ADDR_1(q)		(0x20260 + (q) * 0x100)


#define XRNIC_STAT_QP(q)		(0x20288 + (q) * 0x100)

#define XRNIC_RCVQ_WP_DB_ADDR_MSB(q)	(0x20224 + (q) * 0x100)
#define XRNIC_CQ_BUF_BASE_MSB(q)	(0x202D0 + (q) * 0x100)
#define XRNIC_QP_PD_NUM(q)		(0x202b0 + (q) * 0x100)

#define XRNIC_SNDQ_BUF_BASE_MSB(q)	(0x202C8 + (q) * 0x100)
enum xrnic_wc_opcod {
	XRNIC_RDMA_WRITE = 0x0,
	XRNIC_RDMA_WRITE_WITH_IMM = 0x1,
	XRNIC_SEND_ONLY = 0x2,
	XRNIC_SEND_WITH_IMM = 0x3,
	XRNIC_RDMA_READ = 0x4,
	XRNIC_SEND_WITH_INV = 0xC,
};

#define XRNIC_RQ_BUF_SGE_SIZE	256

/* Deprecated
#define XRNIC_OUTG_PKT_ERRQ_BASE	0x20078
#define XRNIC_OUTG_PKT_ERRQ_SZ		0x20080
#define XRNIC_OUTG_PKT_ERRQ_WPTR	0x20084
*/
#define XRNIC_INCG_PKT_ERRQ_BASE_LSB	0x20088
#define XRNIC_INCG_PKT_ERRQ_SZ		0x20090
#define XRNIC_INCG_PKT_ERRQ_WPTR	0x20094

#define XRNIC_DATA_BUF_BASE_LSB	0x200a0
#define XRNIC_DATA_BUF_SZ	0x200a8
/* only valid for nvmf use case */
#define XRNIC_CNCT_IO_CONF	0x200ac
#define XRNIC_RSP_ERR_BUF_BA_LSB	(0x200B0)
#define XRNIC_RSP_ERR_BUF_DEPTH	(0x200B8)

/* Global status registers */
#define XRNIC_INCG_SND_RDRSP_PKT_CNT	0x20100
#define XRNIC_INCG_ACK_MAD_PKT_CNT	0x20104
#define XRNIC_OUTG_SND_RDWR_PKT_CNT	0x20108
#define XRNIC_OUTG_ACK_MAD_PKT_CNT	0x2010c
#define XRNIC_LST_INCG_PKT		0x20110
#define XRNIC_LST_OUTG_PKT		0x20114
#define XRNIC_INCG_INV_DUP_PKT_CNT	0x20118
#define XRNIC_INCG_NAK_PKT_STAT		0x2011c
#define XRNIC_OUTG_RNR_PKT_STAT		0x20120
#define XRNIC_WQE_PROC_STAT		0x20124
#define XRNIC_QP_MGR_STAT		0x2012c
#define XRNIC_INCG_ALL_DRP_PKT_CNT	0x20130
#define XRNIC_INCG_NAK_PKT_CNT		0x20134
#define XRNIC_OUTG_NAK_PKT_CNT		0x20138
#define XRNIC_RESP_HDLR_STAT		0x2013c
#define XRNIC_RETRY_CNT_STAT		0x20140

#define XRNIC_INT_EN			0x20180
#define XRNIC_INT_STAT			0x20184
  #define XRNIC_INT_PKT_VALID_ERR	BIT(0)
  #define XRNIC_INT_INC_MAD_PKT		BIT(1)
  #define XRNIC_INT_BYPASS_PKT		BIT(2)
  #define XRNIC_INT_RNR_NACK		BIT(3)
  #define XRNIC_INT_WQE_COMP		BIT(4)
  #define XRNIC_INT_ILLEG_OP_SQ		BIT(5)
  #define XRNIC_INT_RQ_PKT_RCVD		BIT(6)
  #define XRNIC_INT_FATAL_ERR_RCVD	BIT(7)
  #define XRNIC_INT_CNP			BIT(8)
#define XRNIC_RCVQ_INT_STAT_0_31	0x20190
#define XRNIC_RCVQ_INT_STAT_31_63	0x20194
#define XRNIC_RCVQ_INT_STAT_64_95	0x20198
#define XRNIC_RCVQ_INT_STAT_96_127	0x2019c
#define XRNIC_RCVQ_INT_STAT_128_159	0x201a0
#define XRNIC_RCVQ_INT_STAT_160_191	0x201a4
#define XRNIC_RCVQ_INT_STAT_192_223	0x201a8
#define XRNIC_RCVQ_INT_STAT_224_255	0x201ac

#define XRNIC_COMPQ_INT_STAT_0_31	0x201b0
#define XRNIC_COMPQ_INT_STAT_31_63	0x201b4
#define XRNIC_COMPQ_INT_STAT_64_95	0x201b8
#define XRNIC_COMPQ_INT_STAT_96_127	0x201bc
#define XRNIC_COMPQ_INT_STAT_128_159	0x201c0
#define XRNIC_COMPQ_INT_STAT_160_191	0x201c4
#define XRNIC_COMPQ_INT_STAT_192_223	0x201c8
#define XRNIC_COMPQ_INT_STAT_224_255	0x201cc
#define XRNIC_CNP_INT_STAT_0_31		0x201D0

#define XRNIC_SQ_PICI_DB_CHECK_EN	(1 << 16)

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
#define XRNIC_QP_ADV_CONF(q)		(0x20204 + (q) * 0x100)
  #define QP_ADV_TC_MASK		0x1f
  #define QP_ADV_TTL_MASK		0xff
  #define QP_ADV_TTL_SHIFT		8
  #define QP_ADV_PKEY_MASK		0xffff
  #define QP_ADV_PKEY_SHIFT		16
#define XRNIC_RCVQ_BUF_BASE_LSB(q)	(0x20208 + (q) * 0x100)
#define XRNIC_RCVQ_BUF_BASE_MSB(q)	(0x202C0 + (q) * 0x100)
#define XRNIC_SNDQ_BUF_BASE_LSB(q)	(0x20210 + (q) * 0x100)
#define XRNIC_CQ_BUF_BASE_LSB(q)	(0x20218 + (q) * 0x100)
#define XRNIC_RCVQ_WP_DB_ADDR_LSB(q)	(0x20220 + (q) * 0x100)
#define XRNIC_CQ_DB_ADDR_LSB(q)		(0x20228 + (q) * 0x100)
#define XRNIC_CQ_DB_ADDR_MSB(q)		(0x2022C + (q) * 0x100)
#define XRNIC_CQ_HEAD_PTR(q)		(0x20230 + (q) * 0x100)
  #define XRNIC_CQ_HEAD_PTR_MASK	0xffff
#define XRNIC_RQ_CONS_IDX(q)		(0x20234 + (q) * 0x100)
#define XRNIC_SQ_PROD_IDX(q)		(0x20238 + (q) * 0x100)
#define XRNIC_QUEUE_DEPTH(q)		(0x2023c + (q) * 0x100)
#define XRNIC_SNDQ_PSN(q)		(0x20240 + (q) * 0x100)
#define XRNIC_LAST_RQ_PSN(q)		(0x20244 + (q) * 0x100)
#define XRNIC_DEST_QP_CONF(q)		(0x20248 + (q) * 0x100)
#define XRNIC_MAC_DEST_ADDR_LO(q)	(0x20250 + (q) * 0x100)
#define XRNIC_MAC_DEST_ADDR_HI(q)	(0x20254 + (q) * 0x100)

#define XRNIC_IP_DEST_ADDR_1(q)		(0x20260 + (q) * 0x100)
#define XRNIC_IP_DEST_ADDR_2(q)		(0x20264 + (q) * 0x100)
#define XRNIC_IP_DEST_ADDR_3(q)		(0x20268 + (q) * 0x100)
#define XRNIC_IP_DEST_ADDR_4(q)		(0x2026c + (q) * 0x100)
#define XRNIC_TIMEOUT_CONF(q)		(0x2024c + (q) * 0x100)
#define XRNIC_STAT_SND_SQN(q)		(0x20280 + (q) * 0x100)
#define XRNIC_STAT_MSG_SQN(q)		(0x20284 + (q) * 0x100)
#define XRNIC_STAT_QP(q)		(0x20288 + (q) * 0x100)
#define XRNIC_STAT_CUR_SQ_PTR(q)	(0x2028c + (q) * 0x100)
#define XRNIC_STAT_RESP_PSN(q)		(0x20290 + (q) * 0x100)
#define XRNIC_STAT_RQ_BUF_LSB(q)	(0x20294 + (q) * 0x100)
#define XRNIC_STAT_WQE(q)		(0x20298 + (q) * 0x100)
#define XRNIC_STAT_RQ_PROD_IDX(q)	(0x2029c + (q) * 0x100)
#define XRNIC_QP_PD_NUM(q)		(0x202b0 + (q) * 0x100)

#define XRNIC_RSP_ERR_BUF_BA_MSB	(0x200B4)
#define XRNIC_ERR_BUF_BASE_MSB		(0x20064)
#define XRNIC_INCG_PKT_ERRQ_BASE_MSB	(0x2008C)
#define XRNIC_DATA_BUF_BASE_MSB		(0x200a4)
#define XRNIC_RCVQ_WP_DB_ADDR_MSB(q)	(0x20224 + (q) * 0x100)
#define XRNIC_CQ_DB_ADDR_MSB(q)		(0x2022C + (q) * 0x100)
#define XRNIC_OUT_ERR_STAT_BA_LSB(q)	(0x20278 + (q) * 0x100)
#define XRNIC_OUT_ERR_STAT_BA_MSB(q)	(0x2027C + (q) * 0x100)
#define XRNIC_SNDQ_BUF_BASE_MSB(q)	(0x202C8 + (q) * 0x100)
#define XRNIC_CQ_BUF_BASE_MSB(q)	(0x202D0 + (q) * 0x100)
#define XRNIC_STAT_RQ_BUF_MSB(q)	(0x202D8 + (q) * 0x100)

#define XRNIC_SEND_SGL_SIZE		4096
#define XRNIC_GSI_SQ_DEPTH		128
#define XRNIC_NUM_OF_ERROR_BUF		64
#define XRNIC_SIZE_OF_ERROR_BUF		256
#define XRNIC_RESP_ERR_BUF_SIZE		64
#define XRNIC_RESP_ERR_BUF_DEPTH	256
#define XRNIC_NUM_OF_DATA_BUF		16
#define XRNIC_SIZE_OF_DATA_BUF		4096
#define XRNIC_OUT_ERRST_Q_NUM_ENTRIES	64
#define XRNIC_IN_ERRST_Q_NUM_ENTRIES	64

#define XRNIC_SW_OVER_RIDE_EN		1
#define XRNIC_SW_OVER_RIDE_BIT		(0)

#define PFC_XON_XOFF_MIN		0
#define PFC_XON_XOFF_MAX		512



#define XRNIC_INVALID_OPC -1

struct xlnx_ernic_config {
	int dummy;
};
#define XRNIC_SQ_WQE_SIZE	64
#define XRNIC_GSI_RECV_PKT_SIZE 512
#define XRNIC_GSI_RQ_DEPTH	64
#define XRNIC_IN_PKT_ERRQ_DEPTH 64

#define XRNIC_RQ_BUF_SGE_SIZE	256
#define XRNIC_DEF_RQ_DEPTH	16
#define XRNIC_DEF_SQ_DEPTH	32

/* Deprecated
#define XRNIC_OUTG_PKT_ERRQ_BASE	0x20078
#define XRNIC_OUTG_PKT_ERRQ_SZ		0x20080
#define XRNIC_OUTG_PKT_ERRQ_WPTR	0x20084
*/
#define XRNIC_INCG_PKT_ERRQ_BASE_LSB	0x20088
#define XRNIC_INCG_PKT_ERRQ_SZ		0x20090
#define XRNIC_INCG_PKT_ERRQ_WPTR	0x20094

#define XRNIC_DATA_BUF_BASE_LSB	0x200a0
#define XRNIC_DATA_BUF_SZ	0x200a8
/* only valid for nvmf use case */
#define XRNIC_CNCT_IO_CONF	0x200ac
#define XRNIC_RSP_ERR_BUF_BA_LSB	(0x200B0)
#define XRNIC_RSP_ERR_BUF_DEPTH	(0x200B8)

/* Global status registers */
#define XRNIC_INCG_SND_RDRSP_PKT_CNT	0x20100
#define XRNIC_INCG_ACK_MAD_PKT_CNT	0x20104
#define XRNIC_OUTG_SND_RDWR_PKT_CNT	0x20108
#define XRNIC_OUTG_ACK_MAD_PKT_CNT	0x2010c
#define XRNIC_LST_INCG_PKT		0x20110
#define XRNIC_LST_OUTG_PKT		0x20114
#define XRNIC_INCG_INV_DUP_PKT_CNT	0x20118
#define XRNIC_INCG_NAK_PKT_STAT		0x2011c
#define XRNIC_OUTG_RNR_PKT_STAT		0x20120
#define XRNIC_WQE_PROC_STAT		0x20124
#define XRNIC_QP_MGR_STAT		0x2012c
#define XRNIC_INCG_ALL_DRP_PKT_CNT	0x20130
#define XRNIC_INCG_NAK_PKT_CNT		0x20134
#define XRNIC_OUTG_NAK_PKT_CNT		0x20138
#define XRNIC_RESP_HDLR_STAT		0x2013c
#define XRNIC_RETRY_CNT_STAT		0x20140

#define XRNIC_INT_EN			0x20180
#define XRNIC_INT_STAT			0x20184
  #define XRNIC_INT_PKT_VALID_ERR	BIT(0)
  #define XRNIC_INT_INC_MAD_PKT		BIT(1)
  #define XRNIC_INT_BYPASS_PKT		BIT(2)
  #define XRNIC_INT_RNR_NACK		BIT(3)
  #define XRNIC_INT_WQE_COMP		BIT(4)
  #define XRNIC_INT_ILLEG_OP_SQ		BIT(5)
  #define XRNIC_INT_RQ_PKT_RCVD		BIT(6)
  #define XRNIC_INT_FATAL_ERR_RCVD	BIT(7)
  #define XRNIC_INT_CNP			BIT(8)
#define XRNIC_RCVQ_INT_STAT_0_31	0x20190
#define XRNIC_RCVQ_INT_STAT_31_63	0x20194
#define XRNIC_RCVQ_INT_STAT_64_95	0x20198
#define XRNIC_RCVQ_INT_STAT_96_127	0x2019c
#define XRNIC_RCVQ_INT_STAT_128_159	0x201a0
#define XRNIC_RCVQ_INT_STAT_160_191	0x201a4
#define XRNIC_RCVQ_INT_STAT_192_223	0x201a8
#define XRNIC_RCVQ_INT_STAT_224_255	0x201ac

#define XRNIC_COMPQ_INT_STAT_0_31	0x201b0
#define XRNIC_COMPQ_INT_STAT_31_63	0x201b4
#define XRNIC_COMPQ_INT_STAT_64_95	0x201b8
#define XRNIC_COMPQ_INT_STAT_96_127	0x201bc
#define XRNIC_COMPQ_INT_STAT_128_159	0x201c0
#define XRNIC_COMPQ_INT_STAT_160_191	0x201c4
#define XRNIC_COMPQ_INT_STAT_192_223	0x201c8
#define XRNIC_COMPQ_INT_STAT_224_255	0x201cc
#define XRNIC_CNP_INT_STAT_0_31		0x201D0

#define XRNIC_SQ_PICI_DB_CHECK_EN	(1 << 16)

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
#define XRNIC_QP_ADV_CONF(q)		(0x20204 + (q) * 0x100)
  #define QP_ADV_TC_MASK		0x1f
  #define QP_ADV_TTL_MASK		0xff
  #define QP_ADV_TTL_SHIFT		8
  #define QP_ADV_PKEY_MASK		0xffff
  #define QP_ADV_PKEY_SHIFT		16
#define XRNIC_RCVQ_BUF_BASE_LSB(q)	(0x20208 + (q) * 0x100)
#define XRNIC_RCVQ_BUF_BASE_MSB(q)	(0x202C0 + (q) * 0x100)
#define XRNIC_SNDQ_BUF_BASE_LSB(q)	(0x20210 + (q) * 0x100)
#define XRNIC_CQ_BUF_BASE_LSB(q)	(0x20218 + (q) * 0x100)
#define XRNIC_RCVQ_WP_DB_ADDR_LSB(q)	(0x20220 + (q) * 0x100)
#define XRNIC_CQ_DB_ADDR_LSB(q)		(0x20228 + (q) * 0x100)
#define XRNIC_CQ_DB_ADDR_MSB(q)		(0x2022C + (q) * 0x100)
#define XRNIC_CQ_HEAD_PTR(q)		(0x20230 + (q) * 0x100)
  #define XRNIC_CQ_HEAD_PTR_MASK	0xffff
#define XRNIC_RQ_CONS_IDX(q)		(0x20234 + (q) * 0x100)
#define XRNIC_SQ_PROD_IDX(q)		(0x20238 + (q) * 0x100)
#define XRNIC_QUEUE_DEPTH(q)		(0x2023c + (q) * 0x100)
#define XRNIC_SNDQ_PSN(q)		(0x20240 + (q) * 0x100)
#define XRNIC_LAST_RQ_PSN(q)		(0x20244 + (q) * 0x100)
#define XRNIC_DEST_QP_CONF(q)		(0x20248 + (q) * 0x100)
#define XRNIC_MAC_DEST_ADDR_LO(q)	(0x20250 + (q) * 0x100)
#define XRNIC_MAC_DEST_ADDR_HI(q)	(0x20254 + (q) * 0x100)

#define XRNIC_IP_DEST_ADDR_1(q)		(0x20260 + (q) * 0x100)
#define XRNIC_IP_DEST_ADDR_2(q)		(0x20264 + (q) * 0x100)
#define XRNIC_IP_DEST_ADDR_3(q)		(0x20268 + (q) * 0x100)
#define XRNIC_IP_DEST_ADDR_4(q)		(0x2026c + (q) * 0x100)
#define XRNIC_TIMEOUT_CONF(q)		(0x2024c + (q) * 0x100)
#define XRNIC_STAT_SND_SQN(q)		(0x20280 + (q) * 0x100)
#define XRNIC_STAT_MSG_SQN(q)		(0x20284 + (q) * 0x100)
#define XRNIC_STAT_QP(q)		(0x20288 + (q) * 0x100)
#define XRNIC_STAT_CUR_SQ_PTR(q)	(0x2028c + (q) * 0x100)
#define XRNIC_STAT_RESP_PSN(q)		(0x20290 + (q) * 0x100)
#define XRNIC_STAT_RQ_BUF_LSB(q)	(0x20294 + (q) * 0x100)
#define XRNIC_STAT_WQE(q)		(0x20298 + (q) * 0x100)
#define XRNIC_STAT_RQ_PROD_IDX(q)	(0x2029c + (q) * 0x100)
#define XRNIC_QP_PD_NUM(q)		(0x202b0 + (q) * 0x100)

#define XRNIC_RSP_ERR_BUF_BA_MSB	(0x200B4)
#define XRNIC_ERR_BUF_BASE_MSB		(0x20064)
#define XRNIC_INCG_PKT_ERRQ_BASE_MSB	(0x2008C)
#define XRNIC_DATA_BUF_BASE_MSB		(0x200a4)
#define XRNIC_RCVQ_WP_DB_ADDR_MSB(q)	(0x20224 + (q) * 0x100)
#define XRNIC_CQ_DB_ADDR_MSB(q)		(0x2022C + (q) * 0x100)
#define XRNIC_OUT_ERR_STAT_BA_LSB(q)	(0x20278 + (q) * 0x100)
#define XRNIC_OUT_ERR_STAT_BA_MSB(q)	(0x2027C + (q) * 0x100)
#define XRNIC_SNDQ_BUF_BASE_MSB(q)	(0x202C8 + (q) * 0x100)
#define XRNIC_CQ_BUF_BASE_MSB(q)	(0x202D0 + (q) * 0x100)
#define XRNIC_STAT_RQ_BUF_MSB(q)	(0x202D8 + (q) * 0x100)

#define XRNIC_SEND_SGL_SIZE		4096
#define XRNIC_GSI_SQ_DEPTH		128
#define XRNIC_NUM_OF_ERROR_BUF		64
#define XRNIC_SIZE_OF_ERROR_BUF		256
#define XRNIC_RESP_ERR_BUF_SIZE		64
#define XRNIC_RESP_ERR_BUF_DEPTH	256
#define XRNIC_NUM_OF_DATA_BUF		16
#define XRNIC_SIZE_OF_DATA_BUF		4096
#define XRNIC_OUT_ERRST_Q_NUM_ENTRIES	64
#define XRNIC_IN_ERRST_Q_NUM_ENTRIES	64

#define XRNIC_SW_OVER_RIDE_EN		1
#define XRNIC_SW_OVER_RIDE_BIT		(0)

#define PFC_XON_XOFF_MIN		0
#define PFC_XON_XOFF_MAX		512



#define XRNIC_INVALID_OPC -1


#define XRNIC_SQ_WQE_SIZE	64
#define XRNIC_GSI_RECV_PKT_SIZE 512
#define XRNIC_GSI_RQ_DEPTH	64
#define XRNIC_IN_PKT_ERRQ_DEPTH 64

#define XRNIC_RQ_BUF_SGE_SIZE	256
#define XRNIC_DEF_RQ_DEPTH	16
#define XRNIC_DEF_SQ_DEPTH	32

struct xrnic_local {
	struct xilinx_ib_dev		*xib;
	struct pci_dev		*pdev;
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
/* Work request 64Byte size */
#define XRNIC_WR_ID_MASK	0xffff
#define XRNIC_OPCODE_MASK	0xff
struct xrnic_wr
{
	u32	wrid;
	u64	l_addr;
	u32	length;
	u32	opcode;
	u64	r_offset;
	u32	r_tag;
#define XRNIC_MAX_SDATA		16
	u8	sdata[XRNIC_MAX_SDATA];
	u32	imm_data;
	u8	resvd[12];
}__attribute__((packed));

static inline void xrnic_iow32(u8 __iomem *base, off_t offset, u32 value)
{
#ifdef WRITE_REG
    iowrite32(value, base + offset);
	wmb();
    #ifdef PRINT_REG_WR
        xt_printk("write reg addr\t0x%lx\tval 0x%x\n", offset, value);
    #endif
#else
    return;
#endif//WRITE_REG
}
static inline u32 xrnic_ior32(u8 __iomem *base, off_t offset)
{
#ifdef WRITE_REG
    u32 val;
    val = ioread32(base + offset);
    rmb();
    #ifdef PRINT_REG_WR
        xt_printk("read reg addr\t0x%lx\tval 0x%x\n", offset, val);
    #endif
    return val;
#else
    return 0;
#endif//WRITE_REG
}

static inline void xrnic_iow(struct xrnic_local *xl, off_t offset, u32 value)
{
#ifdef WRITE_REG
    iowrite32(value, (xl->reg_base + offset));
    #ifdef PRINT_REG_WR
        xt_printk("write reg addr\t0x%lx\tval 0x%x\n", offset, value);
    #endif
#else
    return;
#endif//WRITE_REG
}

static inline u32 xrnic_ior(struct xrnic_local *xl, off_t offset)
{
#ifdef WRITE_REG
    int val;
    val = ioread32( (xl->reg_base + offset));
    #ifdef PRINT_REG_WR
        xt_printk("read reg addr\t0x%lx\tval 0x%x\n", offset, val);
    #endif
    return val;
#else
    return 0;
#endif//WRITE_REG
}




int xrnic_reg_mr(struct xilinx_ib_dev *xib, u64 va, u64 len,
		u64 *pbl_tbl, int umem_pgs, int pdn, u32 mr_idx, u8 rkey);

u64 xrnic_get_sq_db_addr(struct xrnic_local *xl, int hw_qpn);
void config_raw_ip(struct xrnic_local *xl, u32 base, u32 *ip, bool is_ipv6);
u64 xrnic_get_rq_db_addr(struct xrnic_local *xl, int hw_qpn);
int xrnic_qp_set_pd(struct xilinx_ib_dev *xib, int qpn, int pdn);

struct xrnic_local *xrnic_hw_init(struct xib_dev_info *dev_info, struct xilinx_ib_dev *xib);
void xrnic_hw_deinit(struct xilinx_ib_dev *xib);
void xrnic_set_mac(struct xrnic_local *xl, u8 *mac);
int xrnic_start(struct xrnic_local *xl);




#endif /* _XRNIC_H_ */