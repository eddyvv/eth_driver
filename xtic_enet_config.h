#ifndef XTIC_ENET_CONFIG_H
#define XTIC_ENET_CONFIG_H

#define XTIC_ENABLE_TSN     true
#define XTIC_DISABLE_TSN    false

/* Descriptors defines for Tx and Rx DMA */
#define TX_BD_NUM_DEFAULT		64
#define RX_BD_NUM_DEFAULT		128
#define TX_BD_NUM_MAX			4096
#define RX_BD_NUM_MAX			4096


#if defined(CONFIG_XTIC_TSN)
#define XTIC_MAX_QUEUES		    5
#elif defined(CONFIG_AXIENET_HAS_MCDMA)
#define XTIC_MAX_QUEUES		    16
#else
#define XTIC_MAX_QUEUES		    1
#endif






#endif /* XTIC_ENET_CONFIG_H */
