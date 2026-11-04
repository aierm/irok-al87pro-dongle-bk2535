/*
 * rf_dongle.h
 * Native BK2535 2.4G RF Receiver Driver (Synced with BK3632 Keyboard TX)
 */
#ifndef __RF_DONGLE_H__
#define __RF_DONGLE_H__

#include "bk2535_reg.h"

#define MAX_RF_PACKET_LEN  32

#define FLUSH_TX           { BK2401_CMD = 0xA0; }
#define FLUSH_RX           { BK2401_CMD = 0x80; }
#define REUSE_TX_PL        { BK2401_CMD = 0x10; }
#define RF_NOP             { BK2401_CMD = 0x00; }

extern __xdata uint8 g_rf_rx_buf[MAX_RF_PACKET_LEN];
extern volatile uint16 g_rf_rx_pkt_count;

void rf_init(uint8 data_rate);
void SwitchToRxMode(void);
void SwitchToTxMode(void);
void W_ACK_PAYLOAD(const uint8 *pBuf, uint8 bytes, uint8 pipe);
void rf_hop_next_channel(void);
void rf_poll_receive(void);

#endif /* __RF_DONGLE_H__ */
