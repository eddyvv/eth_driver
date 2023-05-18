#include "xt_roce.h"
#include "xtic_enet.h"

static struct xib_driver *xib_drv;
static LIST_HEAD(xt_roce_list);
static DEFINE_MUTEX(xt_adapter_list_lock);

static int _xt_roce_dev_add(struct axienet_local *adapter)
{
    struct xib_dev_info dev_info;
    struct pci_dev *pdev = adapter->pdev;
    int err;

    xib_printfunc("%s start\n", __func__);
    if(!xib_drv)
        return -ENOMEM;

    if(xib_drv->xt_abi_version != XT_ROCE_ABI_VERSION) {
        dev_warn(&pdev->dev,"Cannot initialize RoCE due to xib ABI mismatch\n");
        return -EINVAL;
    }

    dev_info.pdev = adapter->pdev;
    dev_info.xib = adapter->xib;
    dev_info.xib_irq = adapter->xib_irq;
    dev_info.netdev = adapter->ndev;
    dev_info.ethdev = adapter;
    memcpy(dev_info.mac_addr, adapter->mac_addr, ETH_ALEN);

    err = xib_drv->add(&dev_info, adapter->xib_dev);
    if (err)
    {
        dev_err(&pdev->dev, "xib_drv->add fail (err = %d)\n", err);
        return err;
    }

    xib_printfunc("%s start\n", __func__);
    return 0;
}

void xt_roce_dev_add(struct axienet_local *adapter)
{
    int ret;
    xib_printfunc("%s start\n", __func__);

    INIT_LIST_HEAD(&adapter->entry);
    mutex_lock(&xt_adapter_list_lock);
    list_add_tail(&adapter->entry, &xt_roce_list);
    ret = _xt_roce_dev_add(adapter);
    if (ret)
        dev_err(&adapter->pdev->dev,
            "match and instantiation failed for port, ret = %d\n",
            ret);
    mutex_unlock(&xt_adapter_list_lock);
    xib_printfunc("%s end\n", __func__);
}

static void _xt_roce_dev_remove(struct axienet_local *adapter)
{
    if(xib_drv && xib_drv->remove && adapter->xib_dev)
        xib_drv->remove(adapter->xib_dev);
    adapter->flags &= XTIC_FLAGS_RDMA_ENABLED;
    adapter->xib_dev = NULL;
}

void xt_roce_dev_remove(struct axienet_local *adapter)
{
    xib_printfunc("%s start\n", __func__);

    mutex_lock(&xt_adapter_list_lock);
    _xt_roce_dev_remove(adapter);
    list_del(&adapter->entry);
    mutex_unlock(&xt_adapter_list_lock);

    xib_printfunc("%s end\n", __func__);
}

void xt_roce_dev_shutdown(struct axienet_local *adapter)
{
    xib_printfunc("%s start\n", __func__);
    if (xt_roce_supported(adapter)) {
		mutex_lock(&xt_adapter_list_lock);
		if (xib_drv && adapter->xib_dev &&
		    xib_drv->state_change_handler)
			xib_drv->state_change_handler(adapter->xib_dev,
							 XT_DEV_SHUTDOWN);
		mutex_unlock(&xt_adapter_list_lock);
	}
    xib_printfunc("%s end\n", __func__);
}

int xt_roce_register_driver(struct xib_driver *drv)
{
    struct axienet_local *lp;
    int ret;

    xib_printfunc("%s start\n", __func__);
    mutex_lock(&xt_adapter_list_lock);
    if (xib_drv) {
		mutex_unlock(&xt_adapter_list_lock);
		return -EINVAL;
	}
	xib_drv = drv;

    list_for_each_entry(lp, &xt_roce_list, entry) {
		ret = _xt_roce_dev_add(lp);
        if (ret)
			dev_err(&lp->pdev->dev,
				"match and instantiation failed for port, ret = %d\n",
				ret);
	}
    mutex_unlock(&xt_adapter_list_lock);

    xib_printfunc("%s end\n", __func__);
    return 0;
}
EXPORT_SYMBOL(xt_roce_register_driver);



void xt_roce_unregister_driver(struct xib_driver *drv)
{
	struct axienet_local *lp;

    xib_printfunc("%s start\n", __func__);
	mutex_lock(&xt_adapter_list_lock);
	list_for_each_entry(lp, &xt_roce_list, entry) {
		if (lp->xib_dev)
			_xt_roce_dev_remove(lp);
	}
	xib_drv = NULL;
	mutex_unlock(&xt_adapter_list_lock);

    xib_printfunc("%s end\n", __func__);
}
EXPORT_SYMBOL(xt_roce_unregister_driver);



