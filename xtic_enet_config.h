#ifndef XTIC_ENET_CONFIG_H
#define XTIC_ENET_CONFIG_H

#define XTIC_DEBUG

#if defined(LINUX_5_15)

// #define PCI_VENDOR_ID_XTIC 0x1057
// #define PCI_DEVICE_ID_XTIC 0x0004
#define PCI_VENDOR_ID_XTIC 0x8086
#define PCI_DEVICE_ID_XTIC 0x100f
#elif defined(LINUX_5_4)
/* VENDOR_ID 0x10ee DEVICE_ID 0x903f */
#define PCI_VENDOR_ID_XTIC 0x10ee
#define PCI_DEVICE_ID_XTIC 0x9038
#define WRITE_REG
#endif

#define XDMA0_CTRL_BASE       0x00000000
#define XDMA0_B_BASE          0x00000000
#define AXIDMA_1_BASE         0x00200000
#define XXV_ETHERNET_0_BASE   0x00100000

#define XTIC_ENABLE_TSN     true
#define XTIC_DISABLE_TSN    false

/* Descriptors defines for Tx and Rx DMA */
#define TX_BD_NUM_DEFAULT       64
#define RX_BD_NUM_DEFAULT       128
#define TX_BD_NUM_MAX           4096
#define RX_BD_NUM_MAX           4096

#define RX_MEM					0x2580

#define XTIC_MAX_QUEUES         1

#define XTIC_PCIE_MAX_IRQ       2





#endif /* XTIC_ENET_CONFIG_H */
