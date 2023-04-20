#ifndef XTIC_ENET_CONFIG_H
#define XTIC_ENET_CONFIG_H

#define XTIC_ENABLE_TSN     true
#define XTIC_DISABLE_TSN    false

/* Descriptors defines for Tx and Rx DMA */
#define TX_BD_NUM_DEFAULT       64
#define RX_BD_NUM_DEFAULT       128
#define TX_BD_NUM_MAX           4096
#define RX_BD_NUM_MAX           4096

#define RX_MEM					0x2580

#define XTIC_MAX_QUEUES         16







#endif /* XTIC_ENET_CONFIG_H */
