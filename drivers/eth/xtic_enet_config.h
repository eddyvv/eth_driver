#ifndef XTIC_ENET_CONFIG_H
#define XTIC_ENET_CONFIG_H


#define XDMA0_CTRL_BASE       0x00000000
#define XDMA0_B_BASE          0x00000000
#define AXIDMA_1_BASE         0x00200000
#define XXV_ETHERNET_0_BASE   0x00100000
#define XIB_BASE              0x00300000

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
