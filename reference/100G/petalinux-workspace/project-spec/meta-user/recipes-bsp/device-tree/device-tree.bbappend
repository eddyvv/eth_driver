
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SYSTEM_USER_DTSI ?= "system-user.dtsi"
SRC_URI_append = " file://system-user.dtsi \
		   file://device_tree_ernic_tcl.patch"

do_configure_append() {
	echo "#include \"system-user.dtsi\"" >> "${DT_FILES_PATH}/system-top.dts"
}
