#include <linux/netdevice.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include "eth_smart_nic_250soc.h"


static const struct of_device_id eth_250soc_of_matches[] = {
	{ .compatible = "xtic,eth_250soc", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, eth_250soc_of_matches);


static int eth_250soc_probe(struct platform_device *pdev)
{
    printk("eth_250soc_probe\n");
    return 0;
}

static int eth_250soc_remove(struct platform_device *pdev)
{
    printk("eth_250soc_remove\n");
    return 0;
}


static struct platform_driver eth_250soc_driver = {
	.probe		= eth_250soc_probe,
	.remove		= eth_250soc_remove,
	.driver		= {
		.name		= "eth_250soc_nic",
		.of_match_table	= of_match_ptr(eth_250soc_of_matches),
	},
};

module_platform_driver(eth_250soc_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("XTIC Ethernet driver");
MODULE_AUTHOR("Haavard Skinnemoen (Atmel)");
MODULE_ALIAS("platform:eth_250soc");

