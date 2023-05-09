inherit kernel

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
RM_WORK_EXCLUDE += "linux-xlnx"

SRC_URI += " file://002-Adding-xib-abi-header.patch \
	     file://0001-xib-nvmf-addons.patch \
	     file://0001-removed-warn_on-on-disconnect.patch \
	     file://0001-Support-to-enable-hw-accl.patch \
	     file://0001-Add-RDMA-driver-ID-for-ERNIC.patch \
	     file://0001-updated-create-qp-data-structure.patch \
	     file://0001-updated-create-qp-response.patch \
	     file://0001-arm64-zone-dev-support.patch \
	     file://0001-Kernel-patch-for-separated-RQ-PI-CQ-CI-DB-memory.patch \
	     file://0001-Add-new-rdma-core-verb-ibv_reg_mr_ex.patch \
	     file://0001-imm-data-alloc-in-user-space.patch \
	     file://0001-support-for-hw-hs-qp-attr.patch \
             "

SRC_URI += " file://ernic.cfg"
