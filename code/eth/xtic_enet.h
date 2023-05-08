#ifndef ETH_SMART_NIC_250SOC_H
#define ETH_SMART_NIC_250SOC_H

#include <linux/types.h>
#include <linux/device.h>
#include <linux/netdevice.h>
#include <linux/phy.h>
#include <linux/if_vlan.h>
#include <linux/cdev.h>
#include <linux/version.h>
#include "xtic_enet_config.h"
#include "../inc/xtic_common.h"



#define XILINX_IOC_MAGIC                              'D'
#define XILINX_IOC_READ_REG                       _IOR(XILINX_IOC_MAGIC, 0xc0, unsigned long)
#define XILINX_IOC_WRITE_REG                      _IOW(XILINX_IOC_MAGIC, 0xc1, unsigned long)
#define XILINX_IOC_READ_REG_ALL                   _IOR(XILINX_IOC_MAGIC, 0xc2, unsigned long)

/* Packet size info */
#define XAE_HDR_SIZE            14 /* Size of Ethernet header */
#define XAE_TRL_SIZE             4 /* Size of Ethernet trailer (FCS) */
#define XAE_MTU                  1500 /* Max MTU of an Ethernet frame */
#define XAE_JUMBO_MTU              9000 /* Max MTU of a jumbo Eth. frame */

#define XAE_MAX_FRAME_SIZE     (XAE_MTU + XAE_HDR_SIZE + XAE_TRL_SIZE)
#define XAE_MAX_VLAN_FRAME_SIZE  (XAE_MTU + VLAN_ETH_HLEN + XAE_TRL_SIZE)
#define XAE_MAX_JUMBO_FRAME_SIZE (XAE_JUMBO_MTU + XAE_HDR_SIZE + XAE_TRL_SIZE)

#define AXIENET_ETHTOOLS_SSTATS_LEN 6
#define AXIENET_TX_SSTATS_LEN(lp) ((lp)->num_tx_queues * 2)
#define AXIENET_RX_SSTATS_LEN(lp) ((lp)->num_rx_queues * 2)

/* DMA address width min and max range */
#define XAE_DMA_MASK_MIN    32
#define XAE_DMA_MASK_MAX    64

/* In AXI DMA Tx and Rx queue count is same */
#define for_each_tx_dma_queue(lp, var) \
    for ((var) = 0; (var) < (lp)->num_tx_queues; (var)++)

#define for_each_rx_dma_queue(lp, var) \
    for ((var) = 0; (var) < (lp)->num_rx_queues; (var)++)
/* Configuration options */


/* Accept all incoming packets. Default: disabled (cleared) */
#define XAE_OPTION_PROMISC            BIT(0)

/* Jumbo frame support for Tx & Rx. Default: disabled (cleared) */
#define XAE_OPTION_JUMBO            BIT(1)

/* VLAN Rx & Tx frame support. Default: disabled (cleared) */
#define XAE_OPTION_VLAN                BIT(2)

/* Enable recognition of flow control frames on Rx. Default: enabled (set) */
#define XAE_OPTION_FLOW_CONTROL            BIT(4)

/* Strip FCS and PAD from incoming frames. Note: PAD from VLAN frames is not
 * stripped. Default: disabled (set)
 */
#define XAE_OPTION_FCS_STRIP            BIT(5)

/* Generate FCS field and add PAD automatically for outgoing frames.
 * Default: enabled (set)
 */
#define XAE_OPTION_FCS_INSERT            BIT(6)

/* Enable Length/Type error checking for incoming frames. When this option is
 * set, the MAC will filter frames that have a mismatched type/length field
 * and if XAE_OPTION_REPORT_RXERR is set, the user is notified when these
 * types of frames are encountered. When this option is cleared, the MAC will
 * allow these types of frames to be received. Default: enabled (set)
 */
#define XAE_OPTION_LENTYPE_ERR            BIT(7)

/* Enable the transmitter. Default: enabled (set) */
#define XAE_OPTION_TXEN                BIT(11)

/*  Enable the receiver. Default: enabled (set) */
#define XAE_OPTION_RXEN                BIT(12)

/*  Default options set when device is initialized or reset */
#define XAE_OPTION_DEFAULTS                   \
                (XAE_OPTION_TXEN |       \
                 XAE_OPTION_FLOW_CONTROL | \
                 XAE_OPTION_RXEN)

/* XXV MAC Register Definitions */
#define XXV_GT_RESET_OFFSET         0x00000000
#define XXV_TC_OFFSET            0x0000000C
#define XXV_RCW1_OFFSET           0x00000014
#define XXV_JUM_OFFSET            0x00000018
#define XXV_TICKREG_OFFSET          0x00000020
#define XXV_STATRX_BLKLCK_OFFSET       0x0000040C
#define XXV_USXGMII_AN_OFFSET        0x000000C8
#define XXV_USXGMII_AN_STS_OFFSET      0x00000458

#define XAE_IE_OFFSET        0x00000014 /* Interrupt enable */

/* XXV MAC Register Mask Definitions */
#define XXV_GT_RESET_MASK         BIT(0)
#define XXV_TC_TX_MASK          BIT(0)
#define XXV_RCW1_RX_MASK         BIT(0)
#define XXV_RCW1_FCS_MASK         BIT(1)
#define XXV_TC_FCS_MASK          BIT(1)
#define XXV_MIN_JUM_MASK         GENMASK(7, 0)
#define XXV_MAX_JUM_MASK         GENMASK(10, 8)
#define XXV_RX_BLKLCK_MASK        BIT(0)
#define XXV_TICKREG_STATEN_MASK      BIT(0)
#define XXV_MAC_MIN_PKT_LEN        64


#define XAXIDMA_DFT_TX_THRESHOLD    24
#define XAXIDMA_DFT_TX_WAITBOUND    254
#define XAXIDMA_DFT_RX_THRESHOLD    1
#define XAXIDMA_DFT_RX_WAITBOUND    254

#define DELAY_OF_ONE_MILLISEC        1000

#define XAXIENET_NAPI_WEIGHT        64

/* Packet size info */
#define XTIC_HDR_SIZE               14 /* Size of Ethernet header */
#define XTIC_TRL_SIZE               4 /* Size of Ethernet trailer (FCS) */
#define XTIC_NET_MTU               1500 /* Max MTU of an Ethernet frame */
#define XTIC_NET_JUMBO_MTU            9000 /* Max MTU of a jumbo Eth. frame */

/* Axi Ethernet Synthesis features */
#define XAE_FEATURE_PARTIAL_RX_CSUM BIT(0)
#define XAE_FEATURE_PARTIAL_TX_CSUM BIT(1)
#define XAE_FEATURE_FULL_RX_CSUM    BIT(2)
#define XAE_FEATURE_FULL_TX_CSUM    BIT(3)

#define XAE_NO_CSUM_OFFLOAD     0

/* Enable recognition of flow control frames on Rx. Default: enabled (set) */
#define XTIC_OPTION_FLOW_CONTROL            BIT(4)

/* Enable the transmitter. Default: enabled (set) */
#define XTIC_OPTION_TXEN                BIT(11)

/*  Enable the receiver. Default: enabled (set) */
#define XTIC_OPTION_RXEN                BIT(12)

/*  Default options set when device is initialized or reset */
#define XTIC_OPTION_DEFAULTS                   \
                (XTIC_OPTION_TXEN |    \
                 XTIC_OPTION_FLOW_CONTROL | \
                 XTIC_OPTION_RXEN)


/* USXGMII Register Mask Definitions  */
#define USXGMII_AN_EN        BIT(5)
#define USXGMII_AN_RESET    BIT(6)
#define USXGMII_AN_RESTART    BIT(7)
#define USXGMII_EN        BIT(16)
#define USXGMII_RATE_MASK    0x0E000700
#define USXGMII_RATE_1G        0x04000200
#define USXGMII_RATE_2G5    0x08000400
#define USXGMII_RATE_10M    0x0
#define USXGMII_RATE_100M    0x02000100
#define USXGMII_RATE_5G        0x0A000500
#define USXGMII_RATE_10G    0x06000300
#define USXGMII_FD        BIT(28)
#define USXGMII_LINK_STS    BIT(31)

/* USXGMII AN STS register mask definitions */
#define USXGMII_AN_STS_COMP_MASK    BIT(16)


/* Axi DMA Register definitions */

#define XAXIDMA_TX_CR_OFFSET    0x00000000 /* Channel control */
#define XAXIDMA_TX_SR_OFFSET    0x00000004 /* Status */
#define XAXIDMA_TX_CDESC_OFFSET    0x00000008 /* Current descriptor pointer */
#define XAXIDMA_TX_TDESC_OFFSET    0x00000010 /* Tail descriptor pointer */

#define XAXIDMA_RX_CR_OFFSET    0x00000030 /* Channel control */
#define XAXIDMA_RX_SR_OFFSET    0x00000034 /* Status */
#define XAXIDMA_RX_CDESC_OFFSET    0x00000038 /* Current descriptor pointer */
#define XAXIDMA_RX_TDESC_OFFSET    0x00000040 /* Tail descriptor pointer */

#define XAXIDMA_CR_RUNSTOP_MASK    0x00000001 /* Start/stop DMA channel */
#define XAXIDMA_CR_RESET_MASK    0x00000004 /* Reset DMA engine */

#define XAXIDMA_SR_HALT_MASK    0x00000001 /* Indicates DMA channel halted */

#define XAXIDMA_BD_NDESC_OFFSET        0x00 /* Next descriptor pointer */
#define XAXIDMA_BD_BUFA_OFFSET        0x08 /* Buffer address */
#define XAXIDMA_BD_CTRL_LEN_OFFSET    0x18 /* Control/buffer length */
#define XAXIDMA_BD_STS_OFFSET        0x1C /* Status */
#define XAXIDMA_BD_USR0_OFFSET        0x20 /* User IP specific word0 */
#define XAXIDMA_BD_USR1_OFFSET        0x24 /* User IP specific word1 */
#define XAXIDMA_BD_USR2_OFFSET        0x28 /* User IP specific word2 */
#define XAXIDMA_BD_USR3_OFFSET        0x2C /* User IP specific word3 */
#define XAXIDMA_BD_USR4_OFFSET        0x30 /* User IP specific word4 */
#define XAXIDMA_BD_ID_OFFSET        0x34 /* Sw ID */
#define XAXIDMA_BD_HAS_STSCNTRL_OFFSET    0x38 /* Whether has stscntrl strm */
#define XAXIDMA_BD_HAS_DRE_OFFSET    0x3C /* Whether has DRE */

#define XAXIDMA_BD_HAS_DRE_SHIFT    8 /* Whether has DRE shift */
#define XAXIDMA_BD_HAS_DRE_MASK        0xF00 /* Whether has DRE mask */
#define XAXIDMA_BD_WORDLEN_MASK        0xFF /* Whether has DRE mask */

#define XAXIDMA_BD_CTRL_LENGTH_MASK    0x007FFFFF /* Requested len */
#define XAXIDMA_BD_CTRL_TXSOF_MASK    0x08000000 /* First tx packet */
#define XAXIDMA_BD_CTRL_TXEOF_MASK    0x04000000 /* Last tx packet */
#define XAXIDMA_BD_CTRL_ALL_MASK    0x0C000000 /* All control bits */

#define XAXIDMA_DELAY_MASK        0xFF000000 /* Delay timeout counter */
#define XAXIDMA_COALESCE_MASK        0x00FF0000 /* Coalesce counter */

#define XAXIDMA_DELAY_SHIFT        24
#define XAXIDMA_COALESCE_SHIFT        16

#define XAXIDMA_IRQ_IOC_MASK        0x00001000 /* Completion intr */
#define XAXIDMA_IRQ_DELAY_MASK        0x00002000 /* Delay interrupt */
#define XAXIDMA_IRQ_ERROR_MASK        0x00004000 /* Error interrupt */
#define XAXIDMA_IRQ_ALL_MASK        0x00007000 /* All interrupts */

/* Default TX/RX Threshold and waitbound values for SGDMA mode */
#define XAXIDMA_DFT_TX_THRESHOLD    24
#define XAXIDMA_DFT_TX_WAITBOUND    254
#define XAXIDMA_DFT_RX_THRESHOLD    1
#define XAXIDMA_DFT_RX_WAITBOUND    254

#define XAXIDMA_BD_CTRL_TXSOF_MASK    0x08000000 /* First tx packet */
#define XAXIDMA_BD_CTRL_TXEOF_MASK    0x04000000 /* Last tx packet */
#define XAXIDMA_BD_CTRL_ALL_MASK    0x0C000000 /* All control bits */

#define XAXIDMA_BD_STS_ACTUAL_LEN_MASK    0x007FFFFF /* Actual len */
#define XAXIDMA_BD_STS_COMPLETE_MASK    0x80000000 /* Completed */
#define XAXIDMA_BD_STS_DEC_ERR_MASK    0x40000000 /* Decode error */
#define XAXIDMA_BD_STS_SLV_ERR_MASK    0x20000000 /* Slave error */
#define XAXIDMA_BD_STS_INT_ERR_MASK    0x10000000 /* Internal err */
#define XAXIDMA_BD_STS_ALL_ERR_MASK    0x70000000 /* All errors */
#define XAXIDMA_BD_STS_RXSOF_MASK    0x08000000 /* First rx pkt */
#define XAXIDMA_BD_STS_RXEOF_MASK    0x04000000 /* Last rx pkt */
#define XAXIDMA_BD_STS_ALL_MASK        0xFC000000 /* All status bits */

#define XAXIDMA_BD_MINIMUM_ALIGNMENT    0x40

#define DESC_DMA_MAP_SINGLE 0
#define DESC_DMA_MAP_PAGE 1

#define BAR_0 0

/* Macros used when AXI DMA h/w is configured without DRE */
#define XAE_TX_BUFFERS        64
#define XAE_MAX_PKT_LEN        8192

#define XTNET_MAX_IRQ 256
#define NODE_ADDRESS_SIZE 6
enum xtenet_pci_status {
    XTNET_PCI_STATUS_DISABLED,
    XTNET_PCI_STATUS_ENABLED,
};

enum xtenet_device_state {
    XTNET_DEVICE_STATE_UP = 1,
    XTNET_DEVICE_STATE_INTERNAL_ERROR,
};

/**
 * enum axienet_ip_type - AXIENET IP/MAC type.
 *
 * @XAXIENET_1G:     IP is 1G MAC
 * @XAXIENET_2_5G:     IP type is 2.5G MAC.
 * @XAXIENET_LEGACY_10G: IP type is legacy 10G MAC.
 * @XAXIENET_10G_25G:     IP type is 10G/25G MAC(XXV MAC).
 * @XAXIENET_MRMAC:     IP type is hardened Multi Rate MAC (MRMAC).
 *
 */
enum axienet_ip_type {
    XAXIENET_1G = 0,
    XAXIENET_2_5G,
    XAXIENET_LEGACY_10G,
    XAXIENET_10G_25G,
    XAXIENET_MRMAC,
};

struct axienet_config {
    enum axienet_ip_type mactype;
    void (*setoptions)(struct net_device *ndev, u32 options);
    // int (*clk_init)(struct platform_device *pdev, struct clk **axi_aclk,
    //         struct clk **axis_clk, struct clk **ref_clk,
    //         struct clk **dclk);
    u32 tx_ptplen;
    u8 ts_header_len;
};

struct xxvenet_option {
    u32 opt;
    u32 reg;
    u32 m_or;
};

/**
 * struct axidma_bd - Axi Dma buffer descriptor layout
 * @next:         MM2S/S2MM Next Descriptor Pointer
 * @reserved1:    Reserved and not used for 32-bit
 * @phys:         MM2S/S2MM Buffer Address
 * @reserved2:    Reserved and not used for 32-bit
 * @reserved3:    Reserved and not used
 * @reserved4:    Reserved and not used
 * @cntrl:        MM2S/S2MM Control value
 * @status:       MM2S/S2MM Status value
 * @app0:         MM2S/S2MM User Application Field 0.
 * @app1:         MM2S/S2MM User Application Field 1.
 * @app2:         MM2S/S2MM User Application Field 2.
 * @app3:         MM2S/S2MM User Application Field 3.
 * @app4:         MM2S/S2MM User Application Field 4.
 * @sw_id_offset: MM2S/S2MM Sw ID
 * @ptp_tx_skb:   If timestamping is enabled used for timestamping skb
 *          Otherwise reserved.
 * @ptp_tx_ts_tag: Tag value of 2 step timestamping if timestamping is enabled
 *           Otherwise reserved.
 * @tx_skb:      Transmit skb address
 * @tx_desc_mapping: Tx Descriptor DMA mapping type.
 */
struct axidma_bd {
    phys_addr_t next;    /* Physical address of next buffer descriptor */
#ifndef CONFIG_PHYS_ADDR_T_64BIT
    u32 reserved1;
#endif
    phys_addr_t phys;
#ifndef CONFIG_PHYS_ADDR_T_64BIT
    u32 reserved2;
#endif
    u32 reserved3;
    u32 reserved4;
    u32 cntrl;
    u32 status;
    u32 app0;
    u32 app1;    /* TX start << 16 | insert */
    u32 app2;    /* TX csum seed */
    u32 app3;
    u32 app4;
    phys_addr_t sw_id_offset; /* first unused field by h/w */
    phys_addr_t ptp_tx_skb;
    u32 ptp_tx_ts_tag;
    phys_addr_t tx_skb;
    u32 tx_desc_mapping;
} __aligned(XAXIDMA_BD_MINIMUM_ALIGNMENT);

/**
 * struct axienet_dma_q - axienet private per dma queue data
 * @lp:        Parent pointer
 * @dma_regs:    Base address for the axidma device address space
 * @tx_irq:    Axidma TX IRQ number
 * @rx_irq:    Axidma RX IRQ number
 * @tx_lock:    Spin lock for tx path
 * @rx_lock:    Spin lock for tx path
 * @tx_bd_v:    Virtual address of the TX buffer descriptor ring
 * @tx_bd_p:    Physical address(start address) of the TX buffer descr. ring
 * @rx_bd_v:    Virtual address of the RX buffer descriptor ring
 * @rx_bd_p:    Physical address(start address) of the RX buffer descr. ring
 * @tx_buf:    Virtual address of the Tx buffer pool used by the driver when
 *        DMA h/w is configured without DRE.
 * @tx_bufs:    Virutal address of the Tx buffer address.
 * @tx_bufs_dma: Physical address of the Tx buffer address used by the driver
 *         when DMA h/w is configured without DRE.
 * @eth_hasdre: Tells whether DMA h/w is configured with dre or not.
 * @tx_bd_ci:    Stores the index of the Tx buffer descriptor in the ring being
 *        accessed currently. Used while alloc. BDs before a TX starts
 * @tx_bd_tail:    Stores the index of the Tx buffer descriptor in the ring being
 *        accessed currently. Used while processing BDs after the TX
 *        completed.
 * @rx_bd_ci:    Stores the index of the Rx buffer descriptor in the ring being
 *        accessed currently.
 * @tx_packets: Number of transmit packets processed by the dma queue.
 * @tx_bytes:   Number of transmit bytes processed by the dma queue.
 * @rx_packets: Number of receive packets processed by the dma queue.
 * @rx_bytes:    Number of receive bytes processed by the dma queue.
 */
struct axienet_dma_q {
    struct axienet_local    *lp; /* parent */
    void __iomem *dma_regs;

    int tx_irq;
    int rx_irq;

    spinlock_t tx_lock;        /* tx lock */
    spinlock_t rx_lock;        /* rx lock */

    /* Buffer descriptors */
    struct axidma_bd *tx_bd_v;
    struct axidma_bd *rx_bd_v;
    dma_addr_t rx_bd_p;
    dma_addr_t tx_bd_p;

    unsigned char *tx_buf[XAE_TX_BUFFERS];
    unsigned char *tx_bufs;
    dma_addr_t tx_bufs_dma;
    bool eth_hasdre;

    u32 tx_bd_ci;   /* 正在填充的发送环 */
    u32 rx_bd_ci;   /* 正在处理的接受环 */
    u32 tx_bd_tail; /* 正在DMA处理的发送环 */

    unsigned long tx_packets;
    unsigned long tx_bytes;
    unsigned long rx_packets;
    unsigned long rx_bytes;
};

struct xtic_degug_reg_wr{
    unsigned int addr;
    unsigned int data;
};

struct xtic_cdev {
    dev_t devid;            /* 设备号      */
    struct cdev cdev;        /* cdev     */
    struct class *class;        /* 类         */
    struct device *device;    /* 设备      */
    struct axienet_local *axidev;
    int major;                /* 主设备号      */
    int minor;                /* 次设备号   */
    spinlock_t lock;
};

/**
 * struct axienet_local - axienet private per device data
 * @ndev:   Pointer for net_device to which it will be attached.
 * @dev:    Pointer to device structure
 * @phy_node:   Pointer to device node structure
 * @clk:    AXI bus clock
 * @mii_bus:    Pointer to MII bus structure
 * @mii_clk_div: MII bus clock divider value
 * @regs_start: Resource start for axienet device addresses
 * @regs:   Base address for the axienet_local device address space
 * @napi:   Napi Structure array for all dma queues
 * @num_tx_queues: Total number of Tx DMA queues
 * @num_rx_queues: Total number of Rx DMA queues
 * @dq:     DMA queues data
 * @phy_mode:   Phy type to identify between MII/GMII/RGMII/SGMII/1000 Base-X
 * @is_tsn: Denotes a tsn port
 * @num_tc: Total number of TSN Traffic classes
 * @timer_priv: PTP timer private data pointer
 * @ptp_tx_irq: PTP tx irq
 * @ptp_rx_irq: PTP rx irq
 * @rtc_irq:    PTP RTC irq
 * @qbv_irq:    QBV shed irq
 * @ptp_ts_type: ptp time stamp type - 1 or 2 step mode
 * @ptp_rx_hw_pointer: ptp rx hw pointer
 * @ptp_rx_sw_pointer: ptp rx sw pointer
 * @ptp_txq:    PTP tx queue header
 * @tx_tstamp_work: PTP timestamping work queue
 * @ptp_tx_lock: PTP tx lock
 * @dma_err_tasklet: Tasklet structure to process Axi DMA errors
 * @eth_irq:    Axi Ethernet IRQ number
 * @options:    AxiEthernet option word
 * @last_link:  Phy link state in which the PHY was negotiated earlier
 * @features:   Stores the extended features supported by the axienet hw
 * @tx_bd_num:  Number of TX buffer descriptors.
 * @rx_bd_num:  Number of RX buffer descriptors.
 * @max_frm_size: Stores the maximum size of the frame that can be that
 *        Txed/Rxed in the existing hardware. If jumbo option is
 *        supported, the maximum frame size would be 9k. Else it is
 *        1522 bytes (assuming support for basic VLAN)
 * @rxmem:  Stores rx memory size for jumbo frame handling.
 * @csum_offload_on_tx_path:    Stores the checksum selection on TX side.
 * @csum_offload_on_rx_path:    Stores the checksum selection on RX side.
 * @coalesce_count_rx:  Store the irq coalesce on RX side.
 * @coalesce_count_tx:  Store the irq coalesce on TX side.
 * @phy_interface: Phy interface type.
 * @phy_flags:  Phy interface flags.
 * @eth_hasnobuf: Ethernet is configured in Non buf mode.
 * @eth_hasptp: Ethernet is configured for ptp.
 * @axienet_config: Ethernet config structure
 * @tx_ts_regs:   Base address for the axififo device address space.
 * @rx_ts_regs:   Base address for the rx axififo device address space.
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
 * @dma_mask: Specify the width of the DMA address space.
 * @usxgmii_rate: USXGMII PHY speed.
 * @mrmac_rate: MRMAC speed.
 * @gt_pll: Common GT PLL mask control register space.
 * @gt_ctrl: GT speed and reset control register space.
 * @phc_index: Index to corresponding PTP clock used.
 * @gt_lane: MRMAC GT lane index used.
 * @ptp_os_cf: CF TS of PTP PDelay req for one step usage.
 */
struct axienet_local {

    u16    num_tx_queues;   /* Number of TX DMA queues */
    u16    num_rx_queues;   /* Number of RX DMA queues */
    struct axienet_dma_q *dq[XTIC_MAX_QUEUES];    /* DMA queue data*/
    bool    is_tsn;
    u32     options;        /* Current options word */
    char name[16];
    phy_interface_t phy_mode;

    u32 features;

    u16 tx_bd_num;  /* TX描述符数量 */
    u32 rx_bd_num;  /* RX描述符数量 */

    bool eth_hasnobuf;

    u32 rxmem;
    u32 max_frm_size;

    int eth_irq;
    int irqn[XTIC_PCIE_MAX_IRQ];

    u8 mac_addr[NODE_ADDRESS_SIZE];
    u32 coalesce_count_rx;
    u32 coalesce_count_tx;

    u32 phy_interface;

    u32 usxgmii_rate;
    u32 mrmac_rate;        /* MRMAC speed */

    u8 dma_mask;

    struct tasklet_struct dma_err_tasklet[XTIC_MAX_QUEUES];
    struct napi_struct napi[XTIC_MAX_QUEUES];    /* NAPI Structure */

    int csum_offload_on_tx_path;
    int csum_offload_on_rx_path;
    /* bar地址 */
    phys_addr_t     bar_addr;
    phys_addr_t     axidma_addr;
    phys_addr_t     xdma_addr;
    phys_addr_t     xxv_addr;

    /* 映射后的bar地址 */
    u8 __iomem      *regs;
    u8 __iomem      *axidma_regs;
    u8 __iomem      *xdma_regs;
    u8 __iomem      *xxv_regs;
    /* 长度 */
    int         bar_size;
    /* xtenet设备状态 */
    enum xtenet_device_state     state;
    /* 绑定的PCI设备 */
    struct pci_dev      *pdev;
    /* PCI设备状态 */
    enum xtenet_pci_status       pci_status;
    /* 设备对象 */
    struct device       *dev;
    struct xtic_cdev    *xcdev;
    /* 网络设备 */
    struct net_device   *ndev;
    const struct axienet_config *axienet_config;
};

int __maybe_unused axienet_dma_q_init(struct net_device *ndev,
                      struct axienet_dma_q *q);
void axienet_dma_err_handler(unsigned long data);
irqreturn_t __maybe_unused axienet_tx_irq(int irq, void *_ndev);
irqreturn_t __maybe_unused axienet_rx_irq(int irq, void *_ndev);
void axienet_start_xmit_done(struct net_device *ndev, struct axienet_dma_q *q);
void __axienet_device_reset(struct axienet_dma_q *q);
void axienet_dma_bd_release(struct net_device *ndev);

void axienet_set_mac_address(struct net_device *ndev, const void *address);
void axienet_set_multicast_list(struct net_device *ndev);

void __maybe_unused axienet_bd_free(struct net_device *ndev,
                    struct axienet_dma_q *q);
int __maybe_unused axienet_dma_q_init(struct net_device *ndev,
                      struct axienet_dma_q *q);
int xtic_cdev_create_interfaces(struct xtic_cdev *xcdev);
void xtic_cdev_destroy_interfaces(struct xtic_cdev *xcdev);
int xtic_cdev_init(void);

/**
 * axienet_dma_bdout - Memory mapped Axi DMA register Buffer Descriptor write.
 * @q:        Pointer to DMA queue structure
 * @reg:    Address offset from the base address of the Axi DMA core
 * @value:    Value to be written into the Axi DMA register
 *
 * This function writes the desired value into the corresponding Axi DMA
 * register.
 */
static inline void axienet_dma_bdout(struct axienet_dma_q *q,
                     off_t reg, dma_addr_t value)
{
#ifdef WRITE_REG
    #if defined(CONFIG_PHYS_ADDR_T_64BIT)
        #ifdef PRINT_REG_WR
            xt_printk("write axidma reg addr\t0x%lx\tval 0x%llx\n", reg, value);
        #endif
        writeq(value, (q->dma_regs + reg));
    #else //CONFIG_PHYS_ADDR_T_64BIT

        writeq(value, (q->dma_regs + reg));
        #ifdef PRINT_REG_WR
            xt_printk("write axidma reg addr\t0x%lx\tval 0x%llx\n", reg, value);
        #endif

    #endif // CONFIG_PHYS_ADDR_T_64BIT
#else
    return;
#endif // WRITE_REG
}

/**
 * axienet_dma_in32 - Memory mapped Axi DMA register read
 * @q:        Pointer to DMA queue structure
 * @reg:    Address offset from the base address of the Axi DMA core
 *
 * Return: The contents of the Axi DMA register
 *
 * This function returns the contents of the corresponding Axi DMA register.
 */
static inline u32 axienet_dma_in32(struct axienet_dma_q *q, off_t reg)
{
#ifdef WRITE_REG
    int val;
    val = ioread32(q->dma_regs + reg);
    #ifdef PRINT_REG_WR
        xt_printk("read axidma reg addr\t0x%lx\tval 0x%x\n", reg, val);
    #endif
    return val;
#else
    return 0;
#endif // WRITE_REG
}

/**
 * axienet_dma_out32 - Memory mapped Axi DMA register write.
 * @q:        Pointer to DMA queue structure
 * @reg:    Address offset from the base address of the Axi DMA core
 * @value:    Value to be written into the Axi DMA register
 *
 * This function writes the desired value into the corresponding Axi DMA
 * register.
 */
static inline void axienet_dma_out32(struct axienet_dma_q *q,
                     off_t reg, u32 value)
{
#ifdef WRITE_REG
    iowrite32(value, q->dma_regs + reg);
    #ifdef PRINT_REG_WR
        xt_printk("write axidma reg addr\t0x%lx\tval 0x%x\n", reg, value);
    #endif
#else
    return;
#endif//WRITE_REG
}

/*
 * xxv register write
 */
static inline void axienet_xxv_iow(struct axienet_local *lp, off_t offset,
                   u32 value)
{
#ifdef WRITE_REG
    iowrite32(value, lp->xxv_regs + offset);
    #ifdef PRINT_REG_WR
        xt_printk("write xxv reg addr\t0x%lx\tval 0x%x\n", offset, value);
    #endif
#else
    return;
#endif//WRITE_REG
}

/*
 * xxv register read
 */
static inline u32 axienet_xxv_ior(struct axienet_local *lp, off_t offset)
{
#ifdef WRITE_REG
    int val;
    val = ioread32(lp->xxv_regs + offset);
    #ifdef PRINT_REG_WR
        xt_printk("read xxv reg addr\t0x%lx\tval 0x%x\n", offset, val);
    #endif
    return val;
#else
    return 0;
#endif//WRITE_REG
}

/*
 * register write
 */
static inline void axienet_iow(struct axienet_local *lp, off_t offset,
                   u32 value)
{
#ifdef WRITE_REG
    iowrite32(value, lp->xxv_regs + offset);
    #ifdef PRINT_REG_WR
        xt_printk("write reg addr\t0x%lx\tval 0x%x\n", offset, value);
    #endif
#else
    return;
#endif//WRITE_REG
}

/*
 * register read
 */
static inline u32 axienet_ior(struct axienet_local *lp, off_t offset)
{
#ifdef WRITE_REG
    int val;
    val = ioread32(lp->xxv_regs + offset);
    #ifdef PRINT_REG_WR
        xt_printk("read reg addr\t0x%lx\tval 0x%x\n", offset, val);
    #endif
    return val;
#else
    return 0;
#endif//WRITE_REG
}














#endif /* ETH_SMART_NIC_250SOC_H */