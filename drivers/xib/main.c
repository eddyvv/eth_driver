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
#include "../eth/xtic_enet.h"
#include "xib.h"
#include "ib_verbs.h"

struct xilinx_ib_dev *ibdev;

static int handle_netdev_notifier(struct notifier_block *notifier,
                               unsigned long event, void *ptr);
struct notifier_block cmac_netdev_notifier = {
	.notifier_call = handle_netdev_notifier
};

static int cmac_inetaddr_event(struct notifier_block *notifier,
                               unsigned long event, void *data);
static int cmac_inet6addr_event(struct notifier_block *notifier,
                               unsigned long event, void *data);
struct notifier_block cmac_inetaddr_notifier = {
	.notifier_call = cmac_inetaddr_event
};

struct notifier_block cmac_inet6addr_notifier = {
        .notifier_call = cmac_inet6addr_event
};

static const struct pci_device_id xt_roce_pci_tbl[] = {
    {PCI_DEVICE(PCI_VENDOR_ID_XTIC, PCI_DEVICE_ID_XTIC)},
    {0,}
};
MODULE_DEVICE_TABLE(pci, xt_roce_pci_tbl);
unsigned int app_qp_cnt = 10;

unsigned int app_qp_depth = 16;

unsigned int max_rq_sge = 16;

static ssize_t show_roce_pfc_enable(struct kobject *kobj,
		        struct kobj_attribute *attr, char *buf)
{
	if(xrnic_ior(ibdev->xl, XRNIC_PAUSE_CONF) &
			(1U << XRNIC_ROCE_PFC_EN_BIT))
		return sprintf(buf, "1\n");
	else
		return sprintf(buf, "0\n");
}

static ssize_t show_non_roce_pfc_enable(struct kobject *kobj,
		        struct kobj_attribute *attr, char *buf)
{
	if(xrnic_ior(ibdev->xl, XRNIC_PAUSE_CONF) &
			(1U << XRNIC_NON_ROCE_PFC_EN_BIT))
		return sprintf(buf, "1\n");
	else
		return sprintf(buf, "0\n");
}

static ssize_t store_roce_pfc_enable(struct kobject *kobj,
		        struct kobj_attribute *attr,
			const char *buf, size_t count)
{
	u32 val, en;
	val = xrnic_ior(ibdev->xl, XRNIC_PAUSE_CONF);
	en = simple_strtol(buf, NULL, 10);

	if (!en) {
		val &= ~(1U << XRNIC_ROCE_PFC_EN_BIT);
		xrnic_iow(ibdev->xl, XRNIC_PAUSE_CONF, val);
	} else if (en == 1) {
		val |= (1U << XRNIC_ROCE_PFC_EN_BIT);
		xrnic_iow(ibdev->xl, XRNIC_PAUSE_CONF, val);
	} else {
		pr_err("Error: Write 1 or 0 to enable/disable PFC.\n");
	}
	return count;
}

static ssize_t store_non_roce_pfc_enable(struct kobject *kobj,
		        struct kobj_attribute *attr,
			const char *buf, size_t count)
{
	u32 val, en;
	val = xrnic_ior(ibdev->xl, XRNIC_PAUSE_CONF);
	en = simple_strtol(buf, NULL, 10);

	if (!en) {
		val &= ~(1U << XRNIC_NON_ROCE_PFC_EN_BIT);
		xrnic_iow(ibdev->xl, XRNIC_PAUSE_CONF, val);
	} else if (en == 1) {
		val |= (1U << XRNIC_NON_ROCE_PFC_EN_BIT);
		xrnic_iow(ibdev->xl, XRNIC_PAUSE_CONF, val);
	} else {
		pr_err("Error: Write 1 or 0 to enable/disable PFC.\n");
	}
	return count;
}

static ssize_t show_roce_pfc_priority(struct kobject *kobj,
		        struct kobj_attribute *attr, char *buf)
{
	u32 val;
	val = xrnic_ior(ibdev->xl, XRNIC_PAUSE_CONF);

	return sprintf(buf, "%u\n", (val >> XRNIC_ROCE_PFC_PRIO_BIT) &
				XRNIC_PFC_PRIO_BIT_MASK);
}

static ssize_t show_non_roce_pfc_priority(struct kobject *kobj,
		        struct kobj_attribute *attr, char *buf)
{
	u32 val;
	val = xrnic_ior(ibdev->xl, XRNIC_PAUSE_CONF);

	return sprintf(buf, "%u\n", (val >> XRNIC_NON_ROCE_PFC_PRIO_BIT) &
				XRNIC_PFC_PRIO_BIT_MASK);
}

static ssize_t store_roce_pfc_priority(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf, size_t count)
{
	u32 val, prio;
	val = xrnic_ior(ibdev->xl, XRNIC_PAUSE_CONF);
	prio = simple_strtol(buf, NULL, 10);

	if (prio >= 0 && prio <= XRNIC_PFC_GLOBAL_PRIOIRTY) {
		val &= ~(XRNIC_PFC_PRIO_BIT_MASK << XRNIC_ROCE_PFC_PRIO_BIT);
		val |= (prio << XRNIC_ROCE_PFC_PRIO_BIT);
		xrnic_iow(ibdev->xl, XRNIC_PAUSE_CONF, val);
	} else {
		pr_err("Error: Priority value must be 0 to 8.\n");
	}
	return count;
}


static ssize_t store_non_roce_pfc_priority(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf, size_t count)
{
	u32 val, prio;
	val = xrnic_ior(ibdev->xl, XRNIC_PAUSE_CONF);
	prio = simple_strtol(buf, NULL, 10);

	if (prio >= 0 && prio <= XRNIC_PFC_GLOBAL_PRIOIRTY) {
		val &= ~(XRNIC_PFC_PRIO_BIT_MASK << XRNIC_NON_ROCE_PFC_PRIO_BIT);
		val |= (prio << XRNIC_NON_ROCE_PFC_PRIO_BIT);
		xrnic_iow(ibdev->xl, XRNIC_PAUSE_CONF, val);
	} else {
		pr_err("Error: Priority value must be 0 to 8.\n");
	}
	return count;
}

static ssize_t show_roce_pfc_xon(struct kobject *kobj,
		        struct kobj_attribute *attr, char *buf)
{
	u32 val;
	val = xrnic_ior(ibdev->xl, XRNIC_ROCE_PAUSE_OFFSET);
	return sprintf(buf, "%u\n", val & 0xFFFF);
}

static ssize_t show_non_roce_pfc_xon(struct kobject *kobj,
		        struct kobj_attribute *attr, char *buf)
{
	u32 val;
	val = xrnic_ior(ibdev->xl, XRNIC_NON_ROCE_PAUSE_OFFSET);
	return sprintf(buf, "%u\n", val & 0xFFFF);
}

static ssize_t store_roce_pfc_xon(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf, size_t count)
{
	u32 val, xon;
	val = xrnic_ior(ibdev->xl, XRNIC_ROCE_PAUSE_OFFSET);
	xon = simple_strtol(buf, NULL, 10);

	if (xon >= PFC_XON_XOFF_MIN && xon <= PFC_XON_XOFF_MAX) {
		xrnic_iow(ibdev->xl, XRNIC_ROCE_PAUSE_OFFSET, xon | (val & 0xFFFF0000));
	} else {
		pr_err("Error: XON threshold must be 0 to 512.\n");
	}
	return count;
}

static ssize_t store_non_roce_pfc_xon(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf, size_t count)
{
	u32 val, xon;
	val = xrnic_ior(ibdev->xl, XRNIC_NON_ROCE_PAUSE_OFFSET);
	xon = simple_strtol(buf, NULL, 10);

	if (xon >= PFC_XON_XOFF_MIN && xon <= PFC_XON_XOFF_MAX) {
		xrnic_iow(ibdev->xl, XRNIC_NON_ROCE_PAUSE_OFFSET, xon | (val & 0xFFFF0000));
	} else {
		pr_err("Error: XON threshold must be 0 to 512.\n");
	}
	return count;
}

static ssize_t show_non_roce_pfc_xoff(struct kobject *kobj,
		        struct kobj_attribute *attr, char *buf)
{
	u32 val;
	val = xrnic_ior(ibdev->xl, XRNIC_NON_ROCE_PAUSE_OFFSET);
	return sprintf(buf, "%u\n", val >> 16);
}

static ssize_t show_roce_pfc_xoff(struct kobject *kobj,
		        struct kobj_attribute *attr, char *buf)
{
	u32 val;
	val = xrnic_ior(ibdev->xl, XRNIC_ROCE_PAUSE_OFFSET);
	return sprintf(buf, "%u\n", val >> 16);
}

static ssize_t store_roce_pfc_xoff(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf, size_t count)
{
	u32 val, xoff;
	val = xrnic_ior(ibdev->xl, XRNIC_ROCE_PAUSE_OFFSET);
	xoff = simple_strtol(buf, NULL, 10);
	if (xoff >= PFC_XON_XOFF_MIN && xoff <= PFC_XON_XOFF_MAX) {
		xrnic_iow(ibdev->xl, XRNIC_ROCE_PAUSE_OFFSET, (xoff << 16) | (val & 0xFFFF));
	} else {
		pr_err("Error: XOFF threshold must be 0 to 512.\n");
	}
	return count;
}


static ssize_t store_non_roce_pfc_xoff(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf, size_t count)
{
	u32 val, xoff;
	val = xrnic_ior(ibdev->xl, XRNIC_NON_ROCE_PAUSE_OFFSET);
	xoff = simple_strtol(buf, NULL, 10);
	if (xoff >= PFC_XON_XOFF_MIN && xoff <= PFC_XON_XOFF_MAX) {
		xrnic_iow(ibdev->xl, XRNIC_NON_ROCE_PAUSE_OFFSET, (xoff << 16) | (val & 0xFFFF));
	} else {
		pr_err("Error: XOFF threshold must be 0 to 512.\n");
	}
	return count;
}

static ssize_t show_pfc_priority_check(struct kobject *kobj,
		        struct kobj_attribute *attr, char *buf)
{
	u32 val;

	val = xrnic_ior(ibdev->xl, XRNIC_PAUSE_CONF);
	return sprintf(buf, "%u\n", (val >> XRNIC_DIS_PRIO_CHECK_BIT) & 1);
}

static ssize_t store_pfc_priority_check(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf, size_t count)
{
	u32 val, temp;

	val = xrnic_ior(ibdev->xl, XRNIC_PAUSE_CONF);
	temp = simple_strtol(buf, NULL, 10);

	if (temp == 1) {
		val |= (1 << XRNIC_DIS_PRIO_CHECK_BIT);
		xrnic_iow(ibdev->xl, XRNIC_PAUSE_CONF, val);
	} else if (!temp) {
		val &= ~(1 << XRNIC_DIS_PRIO_CHECK_BIT);
		xrnic_iow(ibdev->xl, XRNIC_PAUSE_CONF, val);
	} else {
		pr_err("Error: value must be either a 1 or 0\n");
	}
	return count;
}


static struct kobj_attribute disable_priory_check_attr =
__ATTR(dis_prioirty_check, 0660, show_pfc_priority_check, store_pfc_priority_check);

/* Non-RoCE PFC configuration */
static struct kobj_attribute non_roce_pfc_enable_attr =
__ATTR(en_non_roce_pfc, 0660, show_non_roce_pfc_enable, store_non_roce_pfc_enable);

static struct kobj_attribute non_roce_pfc_priority_attr =
__ATTR(non_roce_pfc_priority, 0660, show_non_roce_pfc_priority, store_non_roce_pfc_priority);

static struct kobj_attribute non_roce_pfc_xon_attr =
__ATTR(non_roce_xon_threshold, 0660, show_non_roce_pfc_xon, store_non_roce_pfc_xon);

static struct kobj_attribute non_roce_pfc_xoff_attr =
__ATTR(non_roce_xoff_threshold, 0660, show_non_roce_pfc_xoff, store_non_roce_pfc_xoff);


/* RoCE PFC configuration */
static struct kobj_attribute roce_pfc_enable_attr =
__ATTR(en_roce_pfc, 0660, show_roce_pfc_enable, store_roce_pfc_enable);

static struct kobj_attribute roce_pfc_priority_attr =
__ATTR(roce_pfc_priority, 0660, show_roce_pfc_priority, store_roce_pfc_priority);

static struct kobj_attribute roce_pfc_xon_attr =
__ATTR(roce_xon_threshold, 0660, show_roce_pfc_xon, store_roce_pfc_xon);

static struct kobj_attribute roce_pfc_xoff_attr =
__ATTR(roce_xoff_threshold, 0660, show_roce_pfc_xoff, store_roce_pfc_xoff);


static struct attribute *pfc_attrs[] = {
	&roce_pfc_enable_attr.attr,
	&roce_pfc_priority_attr.attr,
	&roce_pfc_xon_attr.attr,
	&roce_pfc_xoff_attr.attr,

	&non_roce_pfc_enable_attr.attr,
	&non_roce_pfc_priority_attr.attr,
	&non_roce_pfc_xon_attr.attr,
	&non_roce_pfc_xoff_attr.attr,
	&disable_priory_check_attr.attr,
	NULL,
};

struct attribute_group pfc_attr_group = {
	.attrs = pfc_attrs,
};

static int pfc_create_sysfs_entries(const char *name,
				struct kobject *parent,
				const struct attribute_group *grp,
				struct kobject **kobj)
{
	int ret = 0;

	*kobj = kobject_create_and_add(name, parent);

	if (!*kobj) {
		pr_err("pfc %s sysfs create failed", name);
		return -ENOMEM;
	}

	ret = sysfs_create_group(*kobj, grp);
	if (ret < 0) {
		pr_err("%s unable to create pfc %s sysfs entries\n",
				__func__, name);
		return ret;
	}

	return ret;
}

int update_mtu(struct net_device *dev)
{
	u32 mtu;

	switch (dev->mtu) {
	case 340:
		mtu = QP_PMTU_256;
		break;
	case 592:
		mtu = QP_PMTU_512;
		break;
	case 1500:
		mtu = QP_PMTU_1024;
		break;
	case 2200:
		mtu = QP_PMTU_2048;
		break;
	case 4200:
		mtu = QP_PMTU_4096;
		break;
	default:
		mtu = QP_PMTU_4096;
		break;
	}

	/* update ib dev structure with mtu */
	pr_debug("Updating MTU to %d\n", mtu);
	ibdev->mtu = mtu;
	return 0;
}

int set_ip_address(struct net_device *dev, u32 is_ipv4)
{
	char ip_addr[16];
	int ret = 0;

	if (!dev) {
		pr_err("Dev is null\n");
		return 0;
	}
	if (is_ipv4) {
		u32 ipv4_addr = 0;
		struct in_device *inet_dev = (struct in_device *)dev->ip_ptr;

		if (!inet_dev) {
			pr_err("inet dev is null\n");
			return -EFAULT;
		}
		if (inet_dev->ifa_list) {
			ipv4_addr = inet_dev->ifa_list->ifa_address;
			if (!ipv4_addr) {
				pr_err("ifa_address not available\n");
				return -EINVAL;
			}

			if (!ibdev)
				return -EFAULT;

			if (!ibdev->xl)
				return -EFAULT;

			config_raw_ip(ibdev->xl, XRNIC_IPV4_ADDR, (u32 *)&ipv4_addr, 0);
			snprintf(ip_addr, 16, "%pI4", &ipv4_addr);
			pr_info("IP address is :%s\n", ip_addr);
		} else {
			pr_info("IP address not available at present\n");
			return -EFAULT;
		}
	} else {
		struct inet6_dev *idev;
		struct inet6_ifaddr *ifp, *tmp;
		u32 i, ip_avail = 0;

		idev = __in6_dev_get(dev);
		if (!idev) {
			pr_err("ipv6 inet device not found\n");
			return -EFAULT;
		}

		list_for_each_entry_safe(ifp, tmp, &idev->addr_list, if_list) {
			pr_info("IP=%pI6, MAC=%pM\n", &ifp->addr, dev->dev_addr);
			for (i = 0; i < 16; i++) {
				pr_info("IP=%x\n", ifp->addr.s6_addr[i]);
				ip_addr[15 - i] = ifp->addr.s6_addr[i];
			}
			ip_avail = 1;
			config_raw_ip(ibdev->xl, XRNIC_IPV6_ADD_1, (u32 *)ifp->addr.s6_addr, 1);
		}

		if (!ip_avail) {
			pr_info("IPv6 address not available at present\n");
			return 0;
		}
	}
	ret = update_mtu(dev);
	return ret;
}

static int handle_netdev_notifier(struct notifier_block *notifier,
				       unsigned long event, void *ptr)
{
	struct net_device *dev;

	dev = netdev_notifier_info_to_dev(ptr);
	if (!dev) {
		pr_err("Failed to get the net device");
		return -EINVAL;
	}

	switch (event) {
		case NETDEV_CHANGEADDR:
			pr_info("%s mac changed\n", ibdev->netdev->name);
			xrnic_set_mac(ibdev->xl, dev->dev_addr);
			break;
		case NETDEV_CHANGEMTU:
			pr_info("MTU changed\n");
			update_mtu(dev);
			break;
	}
	return 0;
}

static int handle_inetaddr_notification(struct notifier_block *notifier,
				       unsigned long event, void *data, u32 is_ipv4)
{
	struct in_ifaddr *ifa = data;
	struct net_device *event_netdev;
	struct net_device *dev = __dev_get_by_name(&init_net, ibdev->netdev->name);
	int ret = 0;

	if (!ifa) {
		pr_err("ifaddr is NULL\n");
		return -EINVAL;
	}

	if (!dev) {
		pr_err("Failed to get the net device");
		return -EINVAL;
	}

	if (is_ipv4) {
		event_netdev = ifa->ifa_dev->dev;

		/* Check whether notification is for ernic-ether or not*/
		if (event_netdev != dev)
			return 0;
	}

	switch (event) {
	case NETDEV_DOWN:
		pr_info("%s link down\n", ibdev->netdev->name);
		break;
	case NETDEV_UP:
		pr_info("%s link up\n", ibdev->netdev->name);
		ret = set_ip_address(dev, is_ipv4);
		break;
	}
	return 0;
}

static int cmac_inetaddr_event(struct notifier_block *notifier,
			       unsigned long event, void *data)
{
	handle_inetaddr_notification(notifier, event, data, 1);
	return 0;
}

static int cmac_inet6addr_event(struct notifier_block *notifier,
			       unsigned long event, void *data)
{
	handle_inetaddr_notification(notifier, event, data, 0);
	return 0;
}

static void xib_get_guid(u8 *dev_addr, u8 *guid)
{
	u8 mac[ETH_ALEN];

	/* MAC-48 to EUI-64 mapping */
	memcpy(mac, dev_addr, ETH_ALEN);
	guid[0] = mac[0] ^ 2;
	guid[1] = mac[1];
	guid[2] = mac[2];
	guid[3] = 0xff;
	guid[4] = 0xfe;
	guid[5] = mac[3];
	guid[6] = mac[4];
	guid[7] = mac[5];
}

int xib_alloc_ucontext(struct ib_ucontext *uctx, struct ib_udata *udata)
{
	struct ib_device *ib_dev = uctx->device;
	struct xilinx_ib_dev *xib = get_xilinx_dev(ib_dev);
	struct xrnic_local *xl = xib->xl;
	struct xib_ib_alloc_ucontext_resp resp;
	size_t min_len;
	int ret = 0;

	dev_dbg(&xib->ib_dev.dev, "%s : <---------- \n", __func__);

	resp.qp_tab_size = xib->dev_attr.max_qp;

	resp.db_pa = (u64)xl->db_pa;
	resp.db_size = xl->db_size;
	resp.cq_ci_db_pa = (u64)xl->qp1_sq_db_p;
	resp.rq_pi_db_pa = (u64)xl->qp1_rq_db_p;

	resp.cq_ci_db_size = PAGE_SIZE;
	resp.rq_pi_db_size = PAGE_SIZE;

	dev_dbg(&xib->ib_dev.dev, "%s: db_pa: %llx db_size: %x\n", __func__, resp.db_pa,
			resp.db_size);

	min_len = min_t(size_t, sizeof(struct xib_ib_alloc_ucontext_resp),
			udata->outlen);
	ret = ib_copy_to_udata(udata, &resp, min_len);
	if (ret)
		return ret;

	return 0;
}

static void xib_dealloc_ucontext(struct ib_ucontext *ibucontext)
{
	struct xib_ucontext *context = get_xib_ucontext(ibucontext);

	dev_dbg(&ibucontext->device->dev, "%s : <---------- \n", __func__);

}

static int xib_mmap(struct ib_ucontext *ibucontext,
			 struct vm_area_struct *vma)
{
	struct xilinx_ib_dev *xib = get_xilinx_dev(ibucontext->device);
	// struct xrnic_local *xl = xib->xl;
	ssize_t length;

	if (((vma->vm_end - vma->vm_start) % PAGE_SIZE) != 0)
		return -EINVAL;

	length = vma->vm_end - vma->vm_start;

	dev_dbg(&xib->ib_dev.dev, "%s : pg_off: %lx length: %lx \n", __func__, vma->vm_pgoff, length);
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	if (io_remap_pfn_range(vma,
				vma->vm_start,
				vma->vm_pgoff,
			       length,
			       vma->vm_page_prot)) {
		dev_err(&xib->ib_dev.dev, "Failed to map device memory");
		return -EAGAIN;
	}
	return 0;
}

int xib_map_mr_sge(struct ib_mr *ibmr, struct scatterlist *sg, int sg_nents,
			 unsigned int *sg_offset)

{
        struct xilinx_ib_dev *xib = ibdev;
	struct xib_mr *mr = get_xib_mr(ibmr);
	u64 pa = sg->dma_address;
	u64 va = sg->offset;
	u64 vaddr = va;
	u64 length = sg_dma_len(sg);
	int ret;
#ifndef CONFIG_64BIT
	if (pa < 0xFFFFFFFF) {
		vaddr = (u32)(uintptr_t)va;
	}
#endif
	if (sg == NULL) {
		pr_err("%s: scatterlist is NULL\n", __func__);
		goto fail;
	}

	if(sg_nents > 1) {
		pr_err("%s: SGE entries cannot be more than 1\n", __func__);
		goto fail;
	}
	ret = xrnic_reg_mr(xib, vaddr, length, &pa, sg_nents, mr->pd, mr->mr_idx, mr->rkey);
	if (ret) {
		pr_err("%s:Failed to register MR\n", __func__);
		goto fail;
	}
	return ret;
fail:
	spin_lock_bh(&xib->lock);
	xib_bmap_release_id(&xib->mr_map, mr->mr_idx);
	spin_unlock_bh(&xib->lock);
	kfree(mr);
	return -1;
}

struct ib_mr *xib_alloc_mr(struct ib_pd *ibpd,
				enum ib_mr_type mr_type,
				u32 max_num_sg)
{
        struct xilinx_ib_dev *xib = ibdev;
	struct xib_mr *mr = NULL;
	struct xib_pd *pd = get_xib_pd(ibpd);
	u32 mr_idx;
	u8 rkey;
	int ret;

	mr = kzalloc(sizeof(*mr), GFP_KERNEL);
	if (!mr) {
		pr_err("Failed to allocate memory for mr\n");
		return NULL;
	}

	spin_lock_bh(&xib->lock);
	ret = xib_bmap_alloc_id(&xib->mr_map, &mr_idx);
	spin_unlock_bh(&xib->lock);
	if (ret < 0)
		goto fail;

	get_random_bytes(&rkey, sizeof(rkey));
	/* Alloc mr pointer */
	mr->ib_mr.lkey = (mr_idx << 8) | rkey;
	mr->ib_mr.rkey = (mr_idx << 8) | rkey;
	mr->mr_idx = mr_idx;
	mr->rkey = rkey;
	mr->pd = pd->pdn;
	mr->ib_mr.device = &xib->ib_dev;

	mr->type = XIB_MR_USER;
	return &mr->ib_mr;
fail:
	kfree(mr);
	return NULL;
}

static int xib_alloc_pd(struct ib_pd *ibpd,
			struct ib_udata *udata)
{
	struct ib_device *ibdev = ibpd->device;
	struct xilinx_ib_dev *xib = get_xilinx_dev(ibdev);
	struct xib_pd *pd = get_xib_pd(ibpd);
	int ret;
	struct xib_ucontext *context = rdma_udata_to_drv_context(
				udata, struct xib_ucontext, ib_uc);

	/* TODO we need to lock to protect allocations */
	spin_lock_bh(&xib->lock);
	ret = xib_bmap_alloc_id(&xib->pd_map, &pd->pdn);
	spin_unlock_bh(&xib->lock);
	if (ret < 0)
		return ret;

	dev_dbg(&xib->ib_dev.dev, "%s : pd: %d \n", __func__, pd->pdn);
	if (udata && context) {
		struct xib_ib_alloc_pd_resp uresp;

		uresp.pdn = pd->pdn;
		/* TODO check udata->outlen ? */
		ret = ib_copy_to_udata(udata, &uresp, sizeof(uresp));
		if (ret)
			goto err;
	}

	return 0;
err:
	spin_lock_bh(&xib->lock);
	xib_bmap_release_id(&xib->pd_map, pd->pdn);
	spin_unlock_bh(&xib->lock);
	return ret;
}

static int xib_dealloc_pd(struct ib_pd *ibpd, struct ib_udata * udata)
{
	struct xilinx_ib_dev *xib = get_xilinx_dev(ibpd->device);
	struct xib_pd *pd = get_xib_pd(ibpd);

	dev_dbg(&xib->ib_dev.dev, "%s : <---------- \n", __func__);

	xib_bmap_release_id(&xib->pd_map, pd->pdn);

	/* TODO tell hw about dealloc? */
	return 0;
}

/*u8 port 编译不通过 更改为u32,函数定义在netfiliter/x_tables.h*/
static enum rdma_link_layer xib_get_link_layer(struct ib_device *device,
						    u32 port_num)
{
	return IB_LINK_LAYER_ETHERNET;
}

static int xib_query_device(struct ib_device *ibdev,
				struct ib_device_attr *props,
				struct ib_udata *uhw)
{
	struct xilinx_ib_dev *xib = get_xilinx_dev(ibdev);

	dev_dbg(&xib->ib_dev.dev, "%s : <---------- \n", __func__);

	memset(props, 0, sizeof(*props));
#if 1
	props->max_qp		= app_qp_cnt;
#else
	props->max_qp		= xib->dev_attr.max_qp;
#endif
	props->max_send_sge	= xib->dev_attr.max_send_sge;
	props->max_sge_rd	= xib->dev_attr.max_send_sge;
#if 1
	props->max_qp_wr	= app_qp_depth;
	props->max_recv_sge	= max_rq_sge;
#else
	props->max_qp_wr	= 32;
#endif
	props->max_pd		= xib->dev_attr.max_pd;
	/* TODO ernic doesnt support scatter list
	 * in mr, restrict mr to 1 page
	 */
	props->max_mr		= xib->dev_attr.max_mr;
	props->atomic_cap	= IB_ATOMIC_NONE;

	props->device_cap_flags    = IB_DEVICE_CHANGE_PHY_PORT |
				IB_DEVICE_PORT_ACTIVE_EVENT |
				IB_DEVICE_RC_RNR_NAK_GEN;

#if 1
	props->max_cq		= app_qp_depth;
#else
	props->max_cq		= xib->dev_attr.max_qp - 1;
#endif
	props->max_cqe		= xib->dev_attr.max_cq_wqes;
	props->max_pkeys	= 1;
	props->max_qp_rd_atom   = 0x10; /* TODO how to arrive at these */
	props->max_qp_init_rd_atom = 0x10;

	return 0;
}

/*u8 port 编译不通过 更改为u32,函数定义在netfiliter/x_tables.h*/
int xib_query_port(struct ib_device *ibdev, u32 port,
		       struct ib_port_attr *props)
{
	struct xilinx_ib_dev *xib = get_xilinx_dev(ibdev);

	dev_dbg(&ibdev->dev, "%s : port: %d <---------- \n", __func__, port);

	memset(props, 0, sizeof(struct ib_port_attr));

	if (netif_running(xib->netdev) && netif_carrier_ok(xib->netdev)) {
		props->state = IB_PORT_ACTIVE;
		props->phys_state = 5;
	} else {
		props->state = IB_PORT_DOWN;
		props->phys_state = 3;
	}

	props->gid_tbl_len = 128; /* TODO */
	props->max_mtu = IB_MTU_4096;
	props->lid = 0;
	props->lmc = 0;
	props->sm_lid = 0;
	props->sm_sl = 0;
	props->active_mtu = iboe_get_mtu(xib->netdev->mtu);
	props->port_cap_flags = IB_PORT_CM_SUP | IB_PORT_REINIT_SUP |
				IB_PORT_DEVICE_MGMT_SUP |
				IB_PORT_VENDOR_CLASS_SUP;

	props->active_speed = xib->active_speed;
	props->active_width = xib->active_width;
	props->max_msg_sz = 0x80000000;
	props->bad_pkey_cntr = 0;
	props->qkey_viol_cntr = 0;
	props->subnet_timeout = 0;
	props->init_type_reply = 0;
	props->pkey_tbl_len = 1; /* TODO is it 1? */

	return 0;
}

/*u8 port 编译不通过 更改为u32,函数定义在netfiliter/x_tables.h*/
#define PKEY_ID	0xffff
static int xib_query_pkey(struct ib_device *ibdev, u32 port, u16 index,
			      u16 *pkey)
{
	dev_dbg(&ibdev->dev, "%s : <---------- \n", __func__);
	*pkey = PKEY_ID; /* TODO */
	return 0;
}

#define XIB_MAX_PORT	1
int xib_add_gid(const struct ib_gid_attr *attr, void **context)
{
	if (!rdma_cap_roce_gid_table(attr->device, attr->port_num))
		return -EINVAL;

	if (attr->port_num > XIB_MAX_PORT)
		return -EINVAL;

	if (!context)
		return -EINVAL;

	return 0;
}

int xib_del_gid(const struct ib_gid_attr *attr, void **context)
{
	if (!rdma_cap_roce_gid_table(attr->device, attr->port_num))
		return -EINVAL;

	if (attr->port_num > XIB_MAX_PORT)
		return -EINVAL;

	if (!context)
		return -EINVAL;

	return 0;
}


static const struct ib_device_ops xib_dev_ops = {
    .owner	= THIS_MODULE,
	// .driver_id = RDMA_DRIVER_XLNX,
    .uverbs_abi_ver	= 1,

    .query_device	= xib_query_device,
    .query_port	= xib_query_port,
    .query_pkey	= xib_query_pkey,
    .alloc_ucontext	= xib_alloc_ucontext,
    .dealloc_ucontext = xib_dealloc_ucontext,
    .mmap	= xib_mmap,

    .add_gid	= xib_add_gid,
	.del_gid	= xib_del_gid,
    .alloc_pd	= xib_alloc_pd,
	.alloc_mr	= xib_alloc_mr,
	.map_mr_sg	= xib_map_mr_sge,
	.dealloc_pd	= xib_dealloc_pd,
	.get_link_layer	= xib_get_link_layer,
};




void xib_set_dev_caps(struct ib_device *ibdev)
{
	ibdev->phys_port_cnt		= 1;
	ibdev->num_comp_vectors		= 1;
	ibdev->local_dma_lkey		= 0;
	ibdev->uverbs_cmd_mask		=
		(1ULL << IB_USER_VERBS_CMD_GET_CONTEXT) |
		(1ULL << IB_USER_VERBS_CMD_QUERY_DEVICE) |
		(1ULL << IB_USER_VERBS_CMD_QUERY_PORT) |
		(1ULL << IB_USER_VERBS_CMD_ALLOC_PD) |
		(1ULL << IB_USER_VERBS_CMD_DEALLOC_PD) |
		(1ULL << IB_USER_VERBS_CMD_REG_MR) |
		// (1ULL << IB_USER_VERBS_CMD_REG_MR_EX) |  /* 需在/include/uapi/rdma/ib_user_verbs.h添加枚举变量 */
		(1ULL << IB_USER_VERBS_CMD_DEREG_MR) |
		(1ULL << IB_USER_VERBS_CMD_CREATE_COMP_CHANNEL) |
		(1ULL << IB_USER_VERBS_CMD_CREATE_CQ) |
		(1ULL << IB_USER_VERBS_CMD_DESTROY_CQ) |
		(1ULL << IB_USER_VERBS_CMD_CREATE_QP) |
		(1ULL << IB_USER_VERBS_CMD_MODIFY_QP) |
		(1ULL << IB_USER_VERBS_CMD_QUERY_QP) |
		(1ULL << IB_USER_VERBS_CMD_DESTROY_QP);
}

static struct xilinx_ib_dev *xib_init_instance(struct xib_dev_info *dev_info)
{
    int err;
    struct pci_dev *pdev = dev_info->pdev;
    struct net_device *netdev;
    struct device *dev = &dev_info->pdev->dev;
    struct xrnic_local *xl;
    u32 qpn, rtr_count;
    u64 rtr_addr = 0;

    ibdev = (struct xilinx_ib_dev *)ib_alloc_device(xilinx_ib_dev, ib_dev);
	if(!ibdev) {
		dev_err(&pdev->dev, "cant alloc ibdev\n");
		return -ENOMEM;
	}

    ibdev->mtu = QP_PMTU_4096;
    ibdev->pdev = pdev;

    xl = xrnic_hw_init(pdev, ibdev);
	if (!xl) {
		dev_err(&pdev->dev, "xrnic init failed\n");
		return -ENODEV;
	}

    ibdev->xl = xl;
	xl->xib = ibdev;

    ibdev->netdev = dev_info->netdev;
    if(!ibdev->netdev) {
		dev_err(&pdev->dev, "no netdev found\n");
		return -EINVAL;
	}

    err = update_mtu(ibdev->netdev);
    netdev = ibdev->netdev;
    xib_get_guid(netdev->dev_addr, (u8 *)&ibdev->ib_dev.node_guid);

    ibdev->dev_attr.max_qp = XIB_NUM_QP;
    ibdev->dev_attr.max_pd = XIB_NUM_PD;
    ibdev->dev_attr.max_send_sge = XIB_MAX_SGL_DEPTH;

    dev_dbg(&pdev->dev, "%s: qp:%d pd:%d sgl_depth:%d \n", __func__, ibdev->dev_attr.max_qp,
			ibdev->dev_attr.max_pd,	ibdev->dev_attr.max_send_sge);

    ibdev->dev_attr.max_cq_wqes	= 1024;
	ibdev->dev_attr.max_mr		= ibdev->dev_attr.max_pd;

    /* initialize retry bufs
	* qp 0 doesnt exist
	* qp 1 is UD so no rtr buf needed
	*/
#if 0
	rtr_count = (ibdev->dev_attr.max_qp - 2 )*16;
#else
	rtr_count = 1024;
#endif

    xl->retry_buf_va = dma_alloc_coherent(dev,
                        (rtr_count * XRNIC_SIZE_OF_DATA_BUF),
						 &rtr_addr,
						 GFP_KERNEL);
    if (!rtr_addr) {
		dev_err(&pdev->dev, "Failed to allocate rtr bufs\n");
		return -ENOMEM;
	}
    xl->retry_buf_pa = rtr_addr;

	xrnic_iow(xl, XRNIC_DATA_BUF_BASE_LSB, rtr_addr);
	xrnic_iow(xl, XRNIC_DATA_BUF_BASE_MSB, UPPER_32_BITS(rtr_addr));
	wmb();
	xrnic_iow(xl, XRNIC_DATA_BUF_SZ,
		rtr_count | ( XRNIC_SIZE_OF_DATA_BUF << 16));
	wmb();

    /* initialize in_pkt_errq bufs */
	xl->in_pkt_err_va = dma_alloc_coherent(&pdev->dev,
			(XRNIC_IN_PKT_ERRQ_DEPTH * 8), &xl->in_pkt_err_ba, GFP_KERNEL);
	if (!xl->in_pkt_err_ba) {
		dev_err(&pdev->dev, "Failed to allocate in_pkt_err bufs\n");
		return -ENOMEM;
	}
	xrnic_iow(xl, XRNIC_INCG_PKT_ERRQ_BASE_LSB, xl->in_pkt_err_ba);
	xrnic_iow(xl, XRNIC_INCG_PKT_ERRQ_BASE_MSB,
					UPPER_32_BITS(xl->in_pkt_err_ba));
	wmb();
	/* each entry is 8 bytes, max sz = 64  */
	xrnic_iow(xl, XRNIC_INCG_PKT_ERRQ_SZ, XRNIC_IN_PKT_ERRQ_DEPTH);
	wmb();

    spin_lock_init(&ibdev->lock);

	/* allocate qp list */
	ibdev->qp_list = (struct xib_qp **) kzalloc
			((sizeof(struct xib_qp *) * ibdev->dev_attr.max_qp), GFP_KERNEL);

    /* alloc pd bmap */
	xib_bmap_alloc(&ibdev->pd_map, ibdev->dev_attr.max_pd, "PD");
	/* alloc qp bmap */
	xib_bmap_alloc(&ibdev->qp_map, ibdev->dev_attr.max_qp, "QP");
	/* alloc mr bmap */
	xib_bmap_alloc(&ibdev->mr_map, ibdev->dev_attr.max_mr, "MR");

    /*
	*  TODO do we need to pass PL DDR for infiniband core
	* allocations?
	*/
	ibdev->ib_dev.dev.parent = &pdev->dev;

    strlcpy(ibdev->ib_dev.name, "xib_%d", IB_DEVICE_NAME_MAX);
	ibdev->ib_dev.node_type	= RDMA_NODE_IB_CA;

    xib_set_dev_caps(&ibdev->ib_dev);
    ib_set_device_ops(&ibdev->ib_dev, &xib_dev_ops);

    err = ib_register_device(&ibdev->ib_dev, ibdev->ib_dev.name, &pdev->dev);
	if (err) {
		dev_err(&pdev->dev, "failed to regiser xib device\n");
		goto err_1;
	}

    /* set dma mask */
	/* TODO set relevant mask for 64bit and 32 bit */
	err = dma_set_mask_and_coherent(&ibdev->pdev->dev, DMA_BIT_MASK(32));
	if (err != 0)
		dev_err(&pdev->dev, "unable to set dma mask\n");

	/* the phy attached to 40G is not giving out active speed or width
	 * since we are using 40G in the design default to 40G */ /*TODO */
	#if 0
	ib_get_eth_speed(&ibdev->ib_dev, 1, &ibdev->active_speed,
			&ibdev->active_width);
	#endif
	ibdev->active_speed = IB_SPEED_FDR10;
	ibdev->active_width = IB_WIDTH_4X;

    /* set the mac address */
	xrnic_set_mac(xl, netdev->dev_addr);

    err = set_ip_address(netdev, 1);
	err = set_ip_address(netdev, 0);

    /* pre-reserve QP1
	 * there is no QP0 in ernic HW
	 */
	spin_lock_bh(&ibdev->lock);
    xib_bmap_alloc_id(&ibdev->qp_map, &qpn);
    spin_unlock_bh(&ibdev->lock);

    dev_dbg(&pdev->dev, "gsi qpn: %d\n", qpn);

    if(pfc_create_sysfs_entries("pfc", &ibdev->ib_dev.dev.kobj,
			&pfc_attr_group, &ibdev->pfc_kobj)) {
		dev_err(&pdev->dev, "Failed to create PFC sysfs entry\n");
		goto err_2;
	}

    /* register irq */
	err = request_irq(xl->irq, xib_irq, IRQF_SHARED, "xrnic_intr0",
			(void *)ibdev);
	if (err) {
		dev_err(&pdev->dev, "request irq error!\n");
		goto err_3;
	}

    /* start ernic HW */
	xl->qps_enabled = (ibdev->dev_attr.max_qp - 1);
	xl->udp_sport = 0x8cd1;
	xrnic_start(xl);
    /* register for net dev notifications */
	register_netdevice_notifier(&cmac_netdev_notifier);
	register_inetaddr_notifier(&cmac_inetaddr_notifier);
	register_inet6addr_notifier(&cmac_inet6addr_notifier);

    return NULL;
err_3:
	kobject_put(ibdev->pfc_kobj);
err_2:
	ib_unregister_device(&ibdev->ib_dev);
err_1:
	return err;
}

static struct xilinx_ib_dev *xib_add(struct xib_dev_info *dev_info)
{
    struct pci_dev *pdev = dev_info->pdev;
    const struct pci_device_id *id;

    dev_dbg(&pdev->dev, "%s : <---------- \n", __func__);

    id = pci_match_id(xt_roce_pci_tbl, pdev);
    if(!id)
        return 0;

    return xib_init_instance(dev_info);
}

static void xib_remove(struct xilinx_ib_dev *xdev)
{
	unsigned int rtr_count = 1024;
	struct xrnic_local *xl;
    struct device *dev = &xdev->pdev->dev;

    dev_dbg(dev, "%s : <---------- \n", __func__);

    unregister_netdevice_notifier(&cmac_netdev_notifier);
	unregister_inetaddr_notifier(&cmac_inetaddr_notifier);
	unregister_inet6addr_notifier(&cmac_inet6addr_notifier);

    kobject_put(ibdev->pfc_kobj);
    ib_unregister_device(&xdev->ib_dev);

    /* free rtr buffers */
	xl = xdev->xl;

    if (xl->retry_buf_va)
        dma_free_coherent(dev, (rtr_count * XRNIC_SIZE_OF_DATA_BUF),
                    xl->retry_buf_va, xl->retry_buf_pa);

    /* free incoming error pkt buffer space */
	if (xl->in_pkt_err_va)
		dma_free_coherent(dev, (XRNIC_IN_PKT_ERRQ_DEPTH * 8),
				xl->in_pkt_err_va, xl->in_pkt_err_ba);
	/* free SQ, RQ DB area */

    xrnic_hw_deinit(xdev);

}

static int xib_dispatch_port_error(struct xilinx_ib_dev *dev)
{
	struct ib_event err_event;

	err_event.event = IB_EVENT_PORT_ERR;
	err_event.element.port_num = 1;
	err_event.device = &dev->ib_dev;
	ib_dispatch_event(&err_event);
	return 0;
}

static void xib_shutdown(struct xilinx_ib_dev *dev)
{
    xib_dispatch_port_error(dev);
    xib_remove(dev);
}

/* event handling via NIC driver ensures that all the NIC specific
 * initialization done before RoCE driver notifies
 * event to stack.
 */
static void xib_event_handler(struct xilinx_ib_dev *dev, u32 event)
{
	switch (event) {
	case XT_DEV_SHUTDOWN:
		xib_shutdown(dev);
		break;
	default:
		break;
	}
}

static struct xib_driver xib_driver = {
    .name = "xib driver",
    .add = xib_add,
    .remove = xib_remove,
    .state_change_handler	= xib_event_handler,
    .xt_abi_version = XT_XIB_ROCE_ABI_VERSION,
};

static int __init xtic_ib_init(void)
{
    int status;
    xt_printk("%s\n", __func__);

    status = xt_roce_register_driver(&xib_driver);
    if (status)
		goto err_be_reg;

    return 0;

err_be_reg:

	return status;
}

static void __exit xtic_ib_exit(void)
{
    xt_printk("%s\n", __func__);
    xt_roce_unregister_driver(&xib_driver);
}

module_init(xtic_ib_init);
module_exit(xtic_ib_exit);

MODULE_AUTHOR("XTIC Corporation,<xtic@xtic.com>");
MODULE_DESCRIPTION("XTIC ERNIC IB driver");
MODULE_LICENSE("GPL");


