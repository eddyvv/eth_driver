#ifndef ETH_SMART_NIC_250SOC_H
#define ETH_SMART_NIC_250SOC_H

#include <linux/types.h>
#include <linux/device.h>
#include <linux/netdevice.h>

char xtenet_driver_name[] = "xtenet_eth";
/* VENDOR_ID 0x10ee DEVICE_ID 0x903f */

#define XTIC_DEBUG

#ifndef XTIC_DEBUG
#define xt_printk
#else
#define xt_printk printk
#endif /* XTIC_DEBUG */


#define xtenet_core_err(__dev, format, ...)			\
	dev_err((__dev)->device, "%s:%d:(pid %d): " format,	\
		__func__, __LINE__, current->pid,		\
	       ##__VA_ARGS__)

#define PCI_VENDOR_ID_XTIC 0x1057
#define PCI_DEVICE_ID_XTIC 0x0004












/* Packet size info */
#define XTIC_HDR_SIZE			    14 /* Size of Ethernet header */
#define XTIC_TRL_SIZE			    4 /* Size of Ethernet trailer (FCS) */
#define XTIC_NET_MTU			   1500 /* Max MTU of an Ethernet frame */
#define XTIC_NET_JUMBO_MTU		      9000 /* Max MTU of a jumbo Eth. frame */

/* Axi Ethernet Synthesis features */
#define XAE_FEATURE_PARTIAL_RX_CSUM	BIT(0)
#define XAE_FEATURE_PARTIAL_TX_CSUM	BIT(1)
#define XAE_FEATURE_FULL_RX_CSUM	BIT(2)
#define XAE_FEATURE_FULL_TX_CSUM	BIT(3)

#define XAE_NO_CSUM_OFFLOAD		0

/* Enable recognition of flow control frames on Rx. Default: enabled (set) */
#define XTIC_OPTION_FLOW_CONTROL			BIT(4)

/* Enable the transmitter. Default: enabled (set) */
#define XTIC_OPTION_TXEN				BIT(11)

/*  Enable the receiver. Default: enabled (set) */
#define XTIC_OPTION_RXEN				BIT(12)

/*  Default options set when device is initialized or reset */
#define XTIC_OPTION_DEFAULTS				   \
				(XTIC_OPTION_TXEN |	   \
				 XTIC_OPTION_FLOW_CONTROL | \
				 XTIC_OPTION_RXEN)

#define BAR_0 0

enum xtenet_pci_status {
	XTNET_PCI_STATUS_DISABLED,
	XTNET_PCI_STATUS_ENABLED,
};

enum xtenet_device_state {
	XTNET_DEVICE_STATE_UP = 1,
	XTNET_DEVICE_STATE_INTERNAL_ERROR,
};

/**
 * struct axienet_local - axienet private per device data
 * @ndev:	Pointer for net_device to which it will be attached.
 * @dev:	Pointer to device structure
 * @phy_node:	Pointer to device node structure
 * @clk:	AXI bus clock
 * @mii_bus:	Pointer to MII bus structure
 * @mii_clk_div: MII bus clock divider value
 * @regs_start: Resource start for axienet device addresses
 * @regs:	Base address for the axienet_local device address space
 * @mcdma_regs:	Base address for the aximcdma device address space
 * @napi:	Napi Structure array for all dma queues
 * @num_tx_queues: Total number of Tx DMA queues
 * @num_rx_queues: Total number of Rx DMA queues
 * @dq:		DMA queues data
 * @phy_mode:	Phy type to identify between MII/GMII/RGMII/SGMII/1000 Base-X
 * @is_tsn:	Denotes a tsn port
 * @num_tc:	Total number of TSN Traffic classes
 * @timer_priv: PTP timer private data pointer
 * @ptp_tx_irq: PTP tx irq
 * @ptp_rx_irq: PTP rx irq
 * @rtc_irq:	PTP RTC irq
 * @qbv_irq:	QBV shed irq
 * @ptp_ts_type: ptp time stamp type - 1 or 2 step mode
 * @ptp_rx_hw_pointer: ptp rx hw pointer
 * @ptp_rx_sw_pointer: ptp rx sw pointer
 * @ptp_txq:	PTP tx queue header
 * @tx_tstamp_work: PTP timestamping work queue
 * @ptp_tx_lock: PTP tx lock
 * @dma_err_tasklet: Tasklet structure to process Axi DMA errors
 * @eth_irq:	Axi Ethernet IRQ number
 * @options:	AxiEthernet option word
 * @last_link:	Phy link state in which the PHY was negotiated earlier
 * @features:	Stores the extended features supported by the axienet hw
 * @tx_bd_num:	Number of TX buffer descriptors.
 * @rx_bd_num:	Number of RX buffer descriptors.
 * @max_frm_size: Stores the maximum size of the frame that can be that
 *		  Txed/Rxed in the existing hardware. If jumbo option is
 *		  supported, the maximum frame size would be 9k. Else it is
 *		  1522 bytes (assuming support for basic VLAN)
 * @rxmem:	Stores rx memory size for jumbo frame handling.
 * @csum_offload_on_tx_path:	Stores the checksum selection on TX side.
 * @csum_offload_on_rx_path:	Stores the checksum selection on RX side.
 * @coalesce_count_rx:	Store the irq coalesce on RX side.
 * @coalesce_count_tx:	Store the irq coalesce on TX side.
 * @phy_interface: Phy interface type.
 * @phy_flags:	Phy interface flags.
 * @eth_hasnobuf: Ethernet is configured in Non buf mode.
 * @eth_hasptp: Ethernet is configured for ptp.
 * @axienet_config: Ethernet config structure
 * @tx_ts_regs:	  Base address for the axififo device address space.
 * @rx_ts_regs:	  Base address for the rx axififo device address space.
 * @tstamp_config: Hardware timestamp config structure.
 * @tx_ptpheader: Stores the tx ptp header.
 * @aclk: AXI4-Lite clock for ethernet and dma.
 * @eth_sclk: AXI4-Stream interface clock.
 * @eth_refclk: Stable clock used by signal delay primitives and transceivers.
 * @eth_dclk: Dynamic Reconfiguration Port(DRP) clock.
 * @dma_sg_clk: DMA Scatter Gather Clock.
 * @dma_rx_clk: DMA S2MM Primary Clock.
 * @dma_tx_clk: DMA MM2S Primary Clock.
 * @qnum:     Axi Ethernet queue number to be operate on.
 * @chan_num: MCDMA Channel number to be operate on.
 * @chan_id:  MCMDA Channel id used in conjunction with weight parameter.
 * @weight:   MCDMA Channel weight value to be configured for.
 * @dma_mask: Specify the width of the DMA address space.
 * @usxgmii_rate: USXGMII PHY speed.
 * @mrmac_rate: MRMAC speed.
 * @gt_pll: Common GT PLL mask control register space.
 * @gt_ctrl: GT speed and reset control register space.
 * @phc_index: Index to corresponding PTP clock used.
 * @gt_lane: MRMAC GT lane index used.
 * @ptp_os_cf: CF TS of PTP PDelay req for one step usage.
 */
struct xtenet_core_dev {

    u16    num_tx_queues;	/* Number of TX DMA queues */
    u16    num_rx_queues;   /* Number of RX DMA queues */
    bool    is_tsn;
    u32     options;        /* Current options word */

    u32 features;

    u16 tx_bd_num;
	u32 rx_bd_num;

	int csum_offload_on_tx_path;
	int csum_offload_on_rx_path;
    /* bar地址 */
    phys_addr_t     bar_addr;
    /* 映射后的bar地址 */
    u8 __iomem      *hw_addr;
    /* 长度 */
    int         range;
    /* xtenet设备状态 */
    enum xtenet_device_state     state;
    /* 绑定的PCI设备 */
    struct pci_dev      *pdev;
    /* PCI设备状态 */
    enum xtenet_pci_status       pci_status;
    /* 设备对象 */
    struct device       *device;
    /* 网络设备 */
    struct net_device   *ndev;
};

/*
 * register write
 */
static inline void xtenet_iow(struct xtenet_core_dev *lp, off_t offset,
			       u32 value)
{
	iowrite32(value, lp->hw_addr + offset);
}

/*
 * register read
 */
static inline u32 xtenet_ior(struct xtenet_core_dev *lp, off_t offset)
{
	return ioread32(lp->hw_addr + offset);
}
















#endif /* ETH_SMART_NIC_250SOC_H */