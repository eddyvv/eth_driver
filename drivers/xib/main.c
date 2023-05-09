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




static const struct ib_device_ops xib_dev_ops = {
    .owner	= THIS_MODULE,
	// .driver_id = RDMA_DRIVER_XLNX,
    .uverbs_abi_ver	= 1,

};





static void xib_add(void)
{

}


static struct xib_driver xib_driver = {
    .name = "xib driver",
    .add = xib_add,
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


