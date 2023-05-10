#include <linux/module.h>
// #include <rdma/ib_umem.h>
#include <rdma/ib_cache.h>
#include <rdma/ib_pack.h>
#include <rdma/ib_mad.h>
#include <linux/etherdevice.h>
#include <asm/page.h>
#include "xib.h"
#include "ib_verbs.h"
#include "xib-abi.h"

/* In existing code, instead of using the user space buffers
 * for RDMA_READ, the code uses SGL memory configured when create qp is
 * called, and thus the data rx in READ response is being filled in the
 * SGL memory but not in the user space buffer directly. The fix, that's
 * being made here works only for 32Bit machines */

int xib_bmap_alloc_id(struct xib_bmap *bmap, u32 *id_num)
{
	*id_num = find_first_zero_bit(bmap->bitmap, bmap->max_count);
	if (*id_num >= bmap->max_count)
		return -EINVAL;

	__set_bit(*id_num, bmap->bitmap);

	return 0;
}
