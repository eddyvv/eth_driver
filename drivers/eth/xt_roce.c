#include "xt_roce.h"
#include "xtic_enet.h"

static struct xib_driver *xib_drv;
static LIST_HEAD(xt_roce_list);
static DEFINE_MUTEX(xt_adapter_list_lock);


void xt_roce_dev_add(struct axienet_local *adapter)
{
    xt_printk("%s start\n", __func__);
    INIT_LIST_HEAD(&adapter->entry);
    mutex_lock(&xt_adapter_list_lock);
    list_add_tail(&adapter->entry, &xt_roce_list);

    mutex_unlock(&xt_adapter_list_lock);
    xt_printk("%s end\n", __func__);
}


int xt_roce_register_driver(struct xib_driver *drv)
{
    struct axienet_local *lp;

    xt_printk("%s start\n", __func__);
    mutex_lock(&xt_adapter_list_lock);
    if (xib_drv) {
		mutex_unlock(&xt_adapter_list_lock);
		return -EINVAL;
	}
	xib_drv = drv;

    list_for_each_entry(lp, &xt_roce_list, entry) {
		// _xt_roce_dev_add(lp);
	}
    mutex_unlock(&xt_adapter_list_lock);

    xt_printk("%s end\n", __func__);
    return 0;
}
EXPORT_SYMBOL(xt_roce_register_driver);



void xt_roce_unregister_driver(struct xib_driver *drv)
{
	struct axienet_local *lp;

    xt_printk("%s start\n", __func__);
	mutex_lock(&xt_adapter_list_lock);
	list_for_each_entry(lp, &xt_roce_list, entry) {
		// if (lp->ocrdma_dev)
		// 	_xt_roce_dev_remove(lp);
	}
	xib_drv = NULL;
	mutex_unlock(&xt_adapter_list_lock);

    xt_printk("%s end\n", __func__);
}
EXPORT_SYMBOL(xt_roce_unregister_driver);



