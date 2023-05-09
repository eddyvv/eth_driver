DEPENDS += " umm"
FILESEXTRAPATHS_prepend := "${THISDIR}/:" 
SRC_URI += " file://0001-xilinx-RNIC-providers-code.patch \
	     file://0000-rping-modifications.patch \
	     file://0001-new-verb-reg-mr-ex.patch \
	     file://0001-ERNIC-v3.0-patch.patch \
	     file://0001-ERNIC-v3.1-patch.patch \
	     file://0001-Changes-for-versal.patch \
	     file://0002-ERNIC-SQD-support.patch \
	     file://0001-Corrected-xib_u_reg_mr-function-prototype.patch \
           "

