/*
 * rf_dongle.c
 * Clean-room BK2535 2.4GHz RF Receiver Driver and Protocol Parser.
 * Developed independently based on BK2535 hardware register specifications
 * and paired with BK3632 frequency-hopping RF protocol.
 *
 * SPDX-License-Identifier: MIT
 */
#include "rf_dongle.h"
#include "usb_dongle.h"
#include <string.h>

/* Pre-shared fallback 5-byte Pipe 0 Address matching BK3632 P0_Address */
__code const uint8 RX0_Default_Address[5] = {0x10, 0x11, 0x36, 0x25, 0x35};
/* Public 2.4G Pairing Address matching BK3632 rf_address_tx_page */
__code const uint8 RX_Pair_Address[5]    = {0x20, 0x34, 0x56, 0x42, 0x46};

__xdata uint8 s_rx0_addr[5];

/* 16-Channel Hopping Table matching BK3632 RF_CHANNEL_TABLE[4][4] */
__code const uint8 RF_CHANNEL_TABLE[4][4] = {
    {7, 8, 10, 14},
    {21, 28, 35, 37},
    {40, 41, 42, 49},
    {55, 67, 68, 77}
};

/* Analog Bank1 Registers for 1Mbps and 2Mbps on BK2535 */
__code const uint32 Bank1_Reg0_1M[9] = {
    0x53C0239C,
    0x80FC3DC8,
    0x00009A40,
    0x8086E419,
    0x49DCEDB4,
    0x81780480,
    0x0002FEFF,
    0x262848A1,
    0x1111130C
};

__code const uint32 Bank1_Reg0_2M[9] = {
    0x53C0239C,
    0x80FC3DC8,
    0x00009A40,
    0x8086E419,
    0x49DCEDB4,
    0x81780780,
    0x0002FEFF,
    0x262808E1,
    0x1111130C
};

/* Digital Bank1 RegC / RegD configured for Auto-ACK mode (from Beken 0203 SDK) */
__code const uint8 Bank1_RegC_Ack[4] = {0x00, 0x03, 0x33, 0x10};
__code const uint8 Bank1_RegD_Ack[4] = {0x14, 0xB7, 0x80, 0x01};
__code const uint8 Bank1_Ramp_Table[11] = {
    0x1A, 0x28, 0x92, 0x26, 0xCA, 0xC2, 0xB4, 0x8D, 0xEB, 0xFC, 0x0F
};

__xdata uint8 g_rf_rx_buf[MAX_RF_PACKET_LEN];
volatile uint16 g_rf_rx_pkt_count = 0;

static uint8 s_rf_array = 0;
static uint8 s_rf_channel_cfg = 0x1A; /* Derived from {0x11, 0x36, 0x25, 0x35} */

static uint8 nvr_read_byte(uint8 addr)
{
    uint8 val;
    FLASH_CTL = 0x01;
    NVR_WP0 = 0xA5;
    NVR_WP1 = 0xC3;
    FLASH_KEY = 0xA5;
    FLASH_KEY = 0x49;
    FLASH_ADR = addr;
    FLASH_CTL |= 0x40;
    while (FLASH_CTL & 0x40);
    val = FLASH_DAT;
    NVR_WP0 = 0x00;
    NVR_WP1 = 0x00;
    FLASH_CTL = 0x00;
    return val;
}

static void nvr_write_byte(uint8 addr, uint8 val)
{
    FLASH_CTL = 0x01;
    NVR_WP0 = 0xA5;
    NVR_WP1 = 0xC3;
    FLASH_KEY = 0xA5;
    FLASH_KEY = 0x49;
    FLASH_ADR = addr;
    FLASH_DAT = val;
    FLASH_CTL |= 0x80;
    while (FLASH_CTL & 0x80);
    NVR_WP0 = 0x00;
    NVR_WP1 = 0x00;
    FLASH_CTL = 0x00;
}

static void rf_load_nvr_address(void)
{
    uint8 b[8];
    uint8 i;
    for (i = 0; i < 8; i++)
    {
        b[i] = nvr_read_byte((uint8)(0x10 + i));
    }
    if (((uint8)(b[0] ^ b[4]) == 0xFF) &&
        ((uint8)(b[1] ^ b[5]) == 0xFF) &&
        ((uint8)(b[2] ^ b[6]) == 0xFF) &&
        ((uint8)(b[3] ^ b[7]) == 0xFF) &&
        (b[0] != 0x00 && b[0] != 0xFF))
    {
        s_rx0_addr[0] = 0x10;
        for (i = 0; i < 4; i++)
        {
            s_rx0_addr[i + 1] = b[i];
        }
    }
    else
    {
        for (i = 0; i < 5; i++)
        {
            s_rx0_addr[i] = RX0_Default_Address[i];
        }
    }
}

static void rf_save_nvr_address(const uint8 *addr4)
{
    uint8 i;
    for (i = 0; i < 4; i++)
    {
        nvr_write_byte((uint8)(0x10 + i), addr4[i]);
        nvr_write_byte((uint8)(0x14 + i), (uint8)(addr4[i] ^ 0xFF));
    }
    s_rx0_addr[0] = 0x10;
    for (i = 0; i < 4; i++)
    {
        s_rx0_addr[i + 1] = addr4[i];
    }
}

static void PowerUp_RF(void)
{
    BK2401_CONFIG |= 0x02;
}

static void PowerDown_RF(void)
{
    BK2401_CONFIG &= 0xFD;
}

static void W_BANK1_ANALOG_REG(uint8 addr, const uint8 *pvalue)
{
    uint8 i;
    while (!(BK2401_SCTRL & 0x80))
        ;
    for (i = 0; i < 4; i++)
    {
        BK2401_SDATA[i] = pvalue[i];
    }
    BK2401_SCTRL = addr;
}

static void W_BANK1_DIGITAL_REG(__xdata volatile uint8 *paddr, const uint8 *pvalue, uint8 len)
{
    uint8 i;
    for (i = 0; i < len; i++)
    {
        paddr[i] = pvalue[i];
    }
}

void SwitchToRxMode(void)
{
    PowerUp_RF();
    FLUSH_RX;
    BK2401_CE = 0;
    BK2401_CONFIG |= 0x01; /* PRX */
    BK2401_CE = 1;
}

void SwitchToTxMode(void)
{
    PowerUp_RF();
    FLUSH_TX;
    BK2401_CE = 0;
    BK2401_CONFIG &= 0xFE; /* PTX */
    BK2401_CE = 1;
}

static void R_RX_PAYLOAD(uint8 *pBuf, uint8 bytes)
{
    uint8 i;
    BK2401_CMD = 0x40;
    for (i = 0; i < bytes; i++)
    {
        pBuf[i] = BK2401_FIFO;
    }
    BK2401_CMD = 0x00;
}

void W_ACK_PAYLOAD(const uint8 *pBuf, uint8 bytes, uint8 pipe)
{
    uint8 i;
    BK2401_CMD = (uint8)(0x68 + (pipe & 0x07));
    for (i = 0; i < bytes; i++)
    {
        BK2401_FIFO = pBuf[i];
    }
    BK2401_CMD = 0x00;
}

void W_TX_PAYLOAD(const uint8 *pBuf, uint8 bytes)
{
    uint8 i;
    BK2401_CMD = 0x60;
    for (i = 0; i < bytes; i++)
    {
        BK2401_FIFO = pBuf[i];
    }
    BK2401_CMD = 0x00;
}

static void rf_tx_page_packet(const uint8 *buf, uint8 len)
{
    uint16 wait_loops = 0;
    uint8 i;
    BK2401_CE = 0;
    for (i = 0; i < 5; i++)
    {
        BK2401_TX_ADDR[i] = RX_Pair_Address[i];
    }
    BK2401_CONFIG &= 0xFE; /* Switch to PTX mode */
    FLUSH_TX;
    BK2401_STATUS = 0x70;
    W_TX_PAYLOAD(buf, len);
    BK2401_CE = 1;
    while (!(BK2401_STATUS & (STATUS_TX_DS | STATUS_MAX_RT)) && ++wait_loops < 4000)
    {
    }
    BK2401_CE = 0;
    FLUSH_TX;
    BK2401_STATUS = 0x70;
    for (i = 0; i < 5; i++)
    {
        BK2401_TX_ADDR[i] = s_rx0_addr[i];
    }
    BK2401_CONFIG |= 0x01; /* Switch back to PRX mode */
    BK2401_CE = 1;
}

static void vco_freq_cal(void)
{
    uint8 WriteArr[4];
    uint8 j;
    for (j = 0; j < 4; j++)
    {
        WriteArr[j] = (uint8)((Bank1_Reg0_1M[1] >> (8 * (3 - j))) & 0xFF);
    }
    WriteArr[1] |= 0x02;
    W_BANK1_ANALOG_REG(1, WriteArr);
    delay_us(500);
    WriteArr[1] &= ~0x02;
    W_BANK1_ANALOG_REG(1, WriteArr);
}

static void vco_amp_cal(void)
{
    uint8 WriteArr[4];
    uint8 j;
    for (j = 0; j < 4; j++)
    {
        WriteArr[j] = (uint8)((Bank1_Reg0_1M[1] >> (8 * (3 - j))) & 0xFF);
    }
    WriteArr[2] |= 0x80;
    W_BANK1_ANALOG_REG(1, WriteArr);

    for (j = 0; j < 4; j++)
    {
        WriteArr[j] = (uint8)((Bank1_Reg0_1M[2] >> (8 * (3 - j))) & 0xFF);
    }
    WriteArr[0] |= 0x02;
    W_BANK1_ANALOG_REG(2, WriteArr);
    delay_ms(1);
    WriteArr[1] = 0x04;
    W_BANK1_ANALOG_REG(2, WriteArr);
    delay_us(10);
    WriteArr[1] &= ~0x04;
    W_BANK1_ANALOG_REG(2, WriteArr);
    delay_us(100);
    WriteArr[0] &= ~0x02;
    W_BANK1_ANALOG_REG(2, WriteArr);
}

static void rf_compute_freq_config(const uint8 *addr5)
{
    uint8 t0 = addr5[1];
    uint8 t1 = addr5[2];
    uint8 t2 = addr5[3];
    uint8 t3 = addr5[4];

    if (t0 == 0)
    {
        t0 = 0xFF;
        if (t1 == 0)
        {
            t1 = 0xFF;
            if (t2 == 0)
            {
                t2 = 0xFF;
                t3--;
            }
            else
                t2--;
        }
        else
            t1--;
    }
    else
        t0--;

    s_rf_channel_cfg = t0;
    s_rf_channel_cfg += (uint8)(t1 >> 3);
    s_rf_channel_cfg += (uint8)(t2 >> 4);
    s_rf_channel_cfg += (uint8)(t3 >> 6);
    s_rf_channel_cfg ^= (uint8)(t0 >> 3);
}

void rf_hop_next_channel(void)
{
#if RF_FIXED_CHANNEL_MODE
    BK2401_RFCH = RF_DEFAULT_CHANNEL;
#else
    uint8 uTmp;
    s_rf_array++;
    if (s_rf_array > 3)
        s_rf_array = 0;

    uTmp = s_rf_channel_cfg;
    if (s_rf_array == 1)
        uTmp >>= 2;
    else if (s_rf_array == 2)
        uTmp >>= 4;
    else if (s_rf_array == 3)
        uTmp >>= 6;
    uTmp &= 0x03;

    BK2401_CE = 0;
    BK2401_RFCH = RF_CHANNEL_TABLE[s_rf_array][uTmp];
    BK2401_CE = 1;
#endif
}

void rf_init(uint8 data_rate)
{
    uint8 i, j;
    uint8 WriteArr[4];

    for (i = 0; i <= 8; i++)
    {
        for (j = 0; j < 4; j++)
        {
            if (data_rate == 2)
                WriteArr[j] = (uint8)((Bank1_Reg0_2M[i] >> (8 * (3 - j))) & 0xFF);
            else
                WriteArr[j] = (uint8)((Bank1_Reg0_1M[i] >> (8 * (3 - j))) & 0xFF);
        }
        W_BANK1_ANALOG_REG(i, WriteArr);
    }

    W_BANK1_DIGITAL_REG(BK2401_CFG_0C_ARRAY, Bank1_RegC_Ack, 4);
    W_BANK1_DIGITAL_REG(BK2401_CFG_0D_ARRAY, Bank1_RegD_Ack, 4);
    W_BANK1_DIGITAL_REG(BK2401_RAMP_TAB_ARRAY, Bank1_Ramp_Table, 11);

    TX_FREQ_OFFSET[0] = 0x00;
    TX_FREQ_OFFSET[1] = 0x00;
    TX_FREQ_OFFSET[2] = 0x00;
    TX_FREQ_OFFSET[3] = 0x96;
    RX_FREQ_OFFSET[0] = 0x00;
    RX_FREQ_OFFSET[1] = 0x00;
    RX_FREQ_OFFSET[2] = 0x00;
    RX_FREQ_OFFSET[3] = 0xA0;

    MOD_DELAY = 0x0A;
    PLL_SDM = 0x00;
    FM_GAIN_H = 0x00;
    FM_GAIN_L = 0xB0;
    FM_KMOD_SET_H = 0x01;
    FM_KMOD_SET_L = 0x00;
    if (data_rate == 2)
    {
        MOD_COEFFICIENT_H = 0x13;
        MOD_COEFFICIENT_L = 0x55;
    }
    else
    {
        MOD_COEFFICIENT_H = 0x09;
        MOD_COEFFICIENT_L = 0xAA;
    }
    ANA_CTRL10 = 0x82;
    ANA_CTRL11 = 0xFF;
    ANA_CTRL12 = 0x07;
    ANA_CTRL13 = 0x81;

    BK2401_CONFIG  = 0x0F; /* PRX, Power Up, 16-bit CRC enabled */
    BK2401_ENAA    = 0x3F;
    BK2401_ENRX    = 0x3F;
    BK2401_AW      = 0x03; /* 5-byte RF address width */
    BK2401_RETR    = 0x15;
    BK2401_SETUP   = (data_rate == 2) ? 0x0F : 0x07;

    rf_load_nvr_address();

    for (i = 0; i < 5; i++)
    {
        BK2401_R0_ADDR[i] = s_rx0_addr[i];
        BK2401_TX_ADDR[i] = s_rx0_addr[i];
        BK2401_R1_ADDR[i] = RX_Pair_Address[i];
    }
    BK2401_R2_ADDR = 0x11;
    BK2401_R3_ADDR = 0x12;
    BK2401_R4_ADDR = 0x13;
    BK2401_R5_ADDR = 0x14;

    BK2401_R0_PW   = 0x20;
    BK2401_R1_PW   = 0x20;
    BK2401_R2_PW   = 0x20;
    BK2401_R3_PW   = 0x20;
    BK2401_R4_PW   = 0x20;
    BK2401_R5_PW   = 0x20;
    BK2401_DYNPD   = 0x3F;
    BK2401_FEATURE = 0x07;

    rf_compute_freq_config(s_rx0_addr);
#if RF_FIXED_CHANNEL_MODE
    BK2401_RFCH = RF_DEFAULT_CHANNEL;
#else
    s_rf_array = 3;
    rf_hop_next_channel();
#endif

    PowerDown_RF();
    PowerUp_RF();

    vco_freq_cal();
    vco_amp_cal();

    FLUSH_RX;
    FLUSH_TX;
    BK2401_STATUS = 0x70;
    SwitchToRxMode();
}

void rf_poll_receive(void)
{
    uint8 status;
    uint8 rx_len;
    uint8 hdr;
    uint8 i;

    /* If host PC updated keyboard LEDs (CapsLock/NumLock), queue ACK payload */
    if (g_usb_led_updated)
    {
        __xdata uint8 ack_pld[2];
        g_usb_led_updated = 0;
        ack_pld[0] = KEYBOARD_LED_STATUS;
        ack_pld[1] = g_usb_led_status;
        FLUSH_TX;
        W_ACK_PAYLOAD(ack_pld, 2, 0);
    }

    status = BK2401_STATUS;
    if (!(status & STATUS_RX_DR))
    {
        return;
    }

    do
    {
        rx_len = BK2401_RPL_WIDTH;
        if (rx_len > 0 && rx_len <= MAX_RF_PACKET_LEN)
        {
            memset(g_rf_rx_buf, 0, sizeof(g_rf_rx_buf));
            R_RX_PAYLOAD(g_rf_rx_buf, rx_len);
            g_rf_rx_pkt_count++;

            /* Toggle GPIO indicators on P0.2, P1.0, and P3.6 for visual/scope feedback */
            P0 ^= 0x04;
            P1 ^= 0x01;
            P3 ^= 0x40;

            hdr = g_rf_rx_buf[0];

            /* 1. 2.4G Pairing / Code-Matching Packets */
            if (hdr == DATATYPE_PAGE_START && rx_len >= 5 && g_rf_rx_buf[1] == DATATYPE_KEYBOARD)
            {
                /* Step 0: Keyboard initiates pairing handshake with its BD_ADDR */
                __xdata uint8 page_ack[6];
                __xdata uint8 new_addr[4];
                new_addr[0] = 0x36;
                new_addr[1] = 0x25;
                new_addr[2] = g_rf_rx_buf[2];
                new_addr[3] = g_rf_rx_buf[3];

                page_ack[0] = DATATYPE_PAGE_START;
                page_ack[1] = DATATYPE_KEYBOARD;
                page_ack[2] = new_addr[0];
                page_ack[3] = new_addr[1];
                page_ack[4] = new_addr[2];
                page_ack[5] = new_addr[3];

                FLUSH_TX;
                W_ACK_PAYLOAD(page_ack, 6, 1);
                W_ACK_PAYLOAD(page_ack, 6, 0);
                rf_tx_page_packet(page_ack, 6);
            }
            else if ((hdr == DATATYPE_PAGE_END_MOUSE || hdr == DATATYPE_PAGE_END_KB) &&
                     rx_len >= 9 && g_rf_rx_buf[1] == DATATYPE_KEYBOARD)
            {
                /* Step 1: Keyboard confirms pairing with negotiated 4-byte address */
                __xdata uint8 confirm_ack[9];
                rf_save_nvr_address(&g_rf_rx_buf[5]);
                for (i = 0; i < 5; i++)
                {
                    BK2401_R0_ADDR[i] = s_rx0_addr[i];
                    BK2401_TX_ADDR[i] = s_rx0_addr[i];
                }
                rf_compute_freq_config(s_rx0_addr);

                for (i = 0; i < 9; i++)
                {
                    confirm_ack[i] = g_rf_rx_buf[i];
                }
                FLUSH_TX;
                W_ACK_PAYLOAD(confirm_ack, 9, 1);
                W_ACK_PAYLOAD(confirm_ack, 9, 0);
                rf_tx_page_packet(confirm_ack, 9);
            }
            /* 2. Normal Keyboard Packets (NKRO 16-byte or 6KRO 8-byte) */
            else if ((hdr & (DATATYPE_KEYBOARD | DATATYPE_STAND)) == (DATATYPE_KEYBOARD | DATATYPE_STAND))
            {
                /* Handle transmitter channel hop notification */
                if (hdr & SYSTEM_NEXT_CH)
                {
                    rf_hop_next_channel();
                }

                if (rx_len >= 16)
                {
                    /* Full 15-byte Compact NKRO report (Byte 1: Modifier, Bytes 2..15: 112-bit key bitmap) */
                    usb_send_keyboard_report(&g_rf_rx_buf[1], 15);
                }
                else if (rx_len == 8)
                {
                    /* 7-byte boot report [mod, k0..k5] */
                    usb_send_keyboard_report(&g_rf_rx_buf[1], 7);
                }
                else if (rx_len >= 9)
                {
                    /* 8-byte standard report [mod, rsvd, k0..k5] */
                    usb_send_keyboard_report(&g_rf_rx_buf[1], 8);
                }
            }
            /* 3. Media / Consumer Control Packets (Volume, Play/Pause, Mute, etc.) */
            else if (hdr & DATATYPE_MEDIA)
            {
                if (hdr & SYSTEM_NEXT_CH)
                {
                    rf_hop_next_channel();
                }

                if (rx_len >= 5)
                {
                    usb_send_consumer_report(&g_rf_rx_buf[1], 4);
                }
                else if (rx_len > 1)
                {
                    usb_send_consumer_report(&g_rf_rx_buf[1], (uint8)(rx_len - 1));
                }
                else
                {
                    static __code const uint8 zero_media[4] = {0, 0, 0, 0};
                    usb_send_consumer_report(zero_media, 4);
                }
            }
#if ENABLE_UART_DEBUG
            else
            {
                UartPrintf("[2.4G RX] sync hdr=");
                UartPrintHex8(hdr);
                UartPrintf(" len=");
                UartPrintHex8(rx_len);
                UartPrintf("\r\n");
            }
#endif
        }
        else
        {
            FLUSH_RX;
            break;
        }
    } while ((BK2401_FIFOSTATUS & FIFO_STATUS_RX_EMPTY) == 0);

    BK2401_STATUS = 0x70;
}
