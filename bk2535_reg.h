/*
 * bk2535_reg.h
 * Register definitions for Beken BK2535 (8051 core + USB 2.0 FS + BK2401 2.4G RF)
 * Compatible with SDCC (Small Device C Compiler)
 */
#ifndef __BK2535_REG_H__
#define __BK2535_REG_H__

typedef unsigned char  uint8;
typedef signed   char  int8;
typedef unsigned short uint16;
typedef signed   short int16;
typedef unsigned long  uint32;
typedef signed   long  int32;

#define TRUE  1
#define FALSE 0

/* =========================================================================
 * Unified 2.4G RF Configuration (Synced with BK3632 Keyboard TX)
 * ========================================================================= */
#define RF_FIXED_CHANNEL_MODE     1   /* 1: Fixed channel 40 (2440MHz); 0: 16-ch hopping */
#define RF_DEFAULT_CHANNEL        40  /* 2440 MHz */
#define RF_PAIRING_CHANNEL        100 /* 2450 MHz */
#define RF_AIR_DATA_RATE          2   /* 1: 1Mbps, 2: 2Mbps (High Report Rate) */
#define RF_ADDRESS_LEN            5
#define ENABLE_UART_DEBUG         0   /* 0: Disable blocking UART debug for max speed */

/* 2.4G Packet Header Definitions (matching BK3632 define_system.h) */
#define SYSTEM_SYNCHRONIZE        0x02
#define SYSTEM_END                0x04
#define SYSTEM_NEXT_CH            0x08
#define SYSTEM_CONFIG             0x80
#define SYSTEM_SLEEP              0x8A

#define DATATYPE_MOUSE            0x01
#define DATATYPE_KEYBOARD         0x10
#define DATATYPE_STAND            0x20
#define DATATYPE_MEDIA            0x40
#define DATATYPE_PAGE_START       0xA5
#define DATATYPE_PAGE_END_MOUSE   0x34
#define DATATYPE_PAGE_END_KB      0x36
#define KEYBOARD_LED_STATUS       0x41

/* =========================================================================
 * 8051 Special Function Registers (SFR)
 * ========================================================================= */
__sfr __at(0x80) P0;
__sfr __at(0x81) SP;
__sfr __at(0x82) DPL;
__sfr __at(0x83) DPH;
__sfr __at(0x84) CKCON;
__sfr __at(0x85) CLK_EN_CFG;
__sfr __at(0x86) PCON2;
__sfr __at(0x87) PCON;
__sfr __at(0x88) TCON;
__sfr __at(0x89) TMOD;
__sfr __at(0x8A) TL0;
__sfr __at(0x8B) TL1;
__sfr __at(0x8C) TH0;
__sfr __at(0x8D) TH1;
__sfr __at(0x8E) CCMCON;
__sfr __at(0x8F) CCMVAL;
__sfr __at(0x90) P1;
__sfr __at(0x91) P0IN_EN;
__sfr __at(0x92) DPSEL;
__sfr __at(0x93) P1IN_EN;
__sfr __at(0x94) P2IN_EN;
__sfr __at(0x95) P3IN_EN;
__sfr __at(0x96) P4IN_EN;
__sfr __at(0x97) MMS;
__sfr __at(0x98) SCON0;
__sfr __at(0x99) SBUF0;
__sfr __at(0x9A) P0OUT_EN;
__sfr __at(0x9B) PAGE_A;
__sfr __at(0x9C) PAGE_B;
__sfr __at(0x9D) PAGE_C;
__sfr __at(0x9E) P1OUT_EN;
__sfr __at(0x9F) P2OUT_EN;
__sfr __at(0xA0) P2;
__sfr __at(0xA1) MPAGE;
__sfr __at(0xA2) PWM0_C;
__sfr __at(0xA3) PWM0_DCLSB;
__sfr __at(0xA4) PWM0_DCMSB;
__sfr __at(0xA5) P3OUT_EN;
__sfr __at(0xA6) WDT;
__sfr __at(0xA7) P4OUT_EN;
__sfr __at(0xA8) IE;
__sfr __at(0xA9) P0_PU;
__sfr __at(0xAA) P1_PU;
__sfr __at(0xAB) P2_PU;
__sfr __at(0xAC) P3_PU;
__sfr __at(0xAD) P4_PU;
__sfr __at(0xAE) P0_PD;
__sfr __at(0xAF) P1_PD;
__sfr __at(0xB0) P3;
__sfr __at(0xB1) SPCR;
__sfr __at(0xB2) SPDR;
__sfr __at(0xB3) SPSR;
__sfr __at(0xB4) P2_PD;
__sfr __at(0xB5) P3_PD;
__sfr __at(0xB6) P4_PD;
__sfr __at(0xB7) P4;
__sfr __at(0xB8) IP;
__sfr __at(0xC0) AIF;
__sfr __at(0xC8) T2CON;
__sfr __at(0xC9) NMI_EN;
__sfr __at(0xCA) RCAP2L;
__sfr __at(0xCB) RCAP2H;
__sfr __at(0xCC) TL2;
__sfr __at(0xCD) TH2;
__sfr __at(0xD0) PSW;
__sfr __at(0xD8) P0_OPDR;
__sfr __at(0xD9) P1_OPDR;
__sfr __at(0xDA) P2_OPDR;
__sfr __at(0xDB) P3_OPDR;
__sfr __at(0xDC) P4_OPDR;
__sfr __at(0xE0) ACC;
__sfr __at(0xE8) AIE;
__sfr __at(0xE9) PALT0;
__sfr __at(0xEA) EXSLEEP;
__sfr __at(0xEB) P0_WUEN;
__sfr __at(0xEC) P1_WUEN;
__sfr __at(0xED) P2_WUEN;
__sfr __at(0xEE) P3_WUEN;
__sfr __at(0xEF) P4_WUEN;
__sfr __at(0xF0) B;
__sfr __at(0xF7) PALT1;
__sfr __at(0xF8) AIP;
__sfr __at(0xF9) P0_WUMOD;
__sfr __at(0xFA) P1_WUMOD;
__sfr __at(0xFB) P2_WUMOD;
__sfr __at(0xFC) P3_WUMOD;
__sfr __at(0xFD) P4_WUMOD;

/* =========================================================================
 * Bit-Addressable SFR Bits (__sbit)
 * ========================================================================= */
/* IE (0xA8) */
__sbit __at(0xAF) EA;
__sbit __at(0xAE) ES1;
__sbit __at(0xAD) ET2;
__sbit __at(0xAC) ES0;
__sbit __at(0xAB) ET1;
__sbit __at(0xAA) EX1;
__sbit __at(0xA9) ET0;
__sbit __at(0xA8) EX0;

/* TCON (0x88) */
__sbit __at(0x8F) TF1;
__sbit __at(0x8E) TR1;
__sbit __at(0x8D) TF0;
__sbit __at(0x8C) TR0;
__sbit __at(0x8B) IE1;
__sbit __at(0x8A) IT1;
__sbit __at(0x89) IE0;
__sbit __at(0x88) IT0;

/* SCON0 (0x98) */
__sbit __at(0x9F) SM0;
__sbit __at(0x9E) SM1;
__sbit __at(0x9D) SM2;
__sbit __at(0x9C) REN;
__sbit __at(0x9B) TB8;
__sbit __at(0x9A) RB8;
__sbit __at(0x99) TI;
__sbit __at(0x98) RI;

/* T2CON (0xC8) */
__sbit __at(0xCF) TF2;
__sbit __at(0xCE) EXF2;
__sbit __at(0xCD) RCLK;
__sbit __at(0xCC) TCLK;
__sbit __at(0xCB) EXEN2;
__sbit __at(0xCA) TR2;
__sbit __at(0xC9) C_T2;
__sbit __at(0xC8) CP_RL2;

/* AIE (0xE8) */
__sbit __at(0xEF) EX9;
__sbit __at(0xEE) EX8;
__sbit __at(0xED) EX7;
__sbit __at(0xEC) EX6;
__sbit __at(0xEB) EX5;
__sbit __at(0xEA) EX4;
__sbit __at(0xE9) EX3;
__sbit __at(0xE8) EX2;

/* AIF (0xC0) */
__sbit __at(0xC7) INT_AES;
__sbit __at(0xC6) INT_ADC;
__sbit __at(0xC5) INT_PCA;
__sbit __at(0xC4) INT_EXT;
__sbit __at(0xC3) INT_XVR;
__sbit __at(0xC2) INT_USB;
__sbit __at(0xC1) INT_LBD;
__sbit __at(0xC0) INT_SPI;

/* =========================================================================
 * XDATA Memory-Mapped USB & BK2401 RF Registers
 * ========================================================================= */
#define XBYTE ((__xdata volatile unsigned char *)0)
#define PCHAR(addr) ((__xdata volatile unsigned char *)(addr))

/* USB FIFO Buffers in XDATA */
#define EP0_ADDR_DEF       XBYTE[0x0400]
#define EP1_ADDR_DEF       XBYTE[0x0440]
#define EP2_ADDR_DEF       XBYTE[0x0480]

/* USB Controller Registers */
#define FRAM_NO_0          XBYTE[0x0808]
#define FRAM_NO_1          XBYTE[0x0809]
#define USBINT0            XBYTE[0x080A]
#define USBINT1            XBYTE[0x080B]
#define USB_EN0            XBYTE[0x080C]
#define USB_EN1            XBYTE[0x080D]
#define EP_STATUS_IN       XBYTE[0x080E]
#define EP_STATUS_OUT      XBYTE[0x080F]

#define CFG_EP0_1          XBYTE[0x0810]
#define CFG_EP0_0          XBYTE[0x0811]
#define CFG_EP1_1          XBYTE[0x0812]
#define CFG_EP1_0          XBYTE[0x0813]
#define CFG_EP2_1          XBYTE[0x0814]
#define CFG_EP2_0          XBYTE[0x0815]
#define CFG_EP3_1          XBYTE[0x0816]
#define CFG_EP3_0          XBYTE[0x0817]
#define CFG_EP4_1          XBYTE[0x0818]
#define CFG_EP4_0          XBYTE[0x0819]
#define CFG_EP5_1          XBYTE[0x081A]
#define CFG_EP5_0          XBYTE[0x081B]

#define EP_HALT            XBYTE[0x0820]
#define EP_RDY             XBYTE[0x0821]
#define FUNCT_ADR          XBYTE[0x0822]
#define CNT0               XBYTE[0x0823]
#define CNT1               XBYTE[0x0824]
#define CNT2               XBYTE[0x0825]
#define CNT3               XBYTE[0x0826]
#define CNT4               XBYTE[0x0827]
#define CNT5               XBYTE[0x0828]

#define EP_ADDR_MSB        XBYTE[0x0840]
#define USB_PWR_CN         XBYTE[0x0841]
#define USB_DEBUG          XBYTE[0x0842]
#define USB_SOFT_RST       XBYTE[0x0843]

/* BK2401 2.4G Baseband & Analog Registers (0x0880 - 0x08FF) */
#define BK2401_CONFIG      XBYTE[0x0880]
#define BK2401_ENAA        XBYTE[0x0881]
#define BK2401_ENRX        XBYTE[0x0882]
#define BK2401_AW          XBYTE[0x0883]
#define BK2401_RETR        XBYTE[0x0884]
#define BK2401_RFCH        XBYTE[0x0885]
#define BK2401_SETUP       XBYTE[0x0886]

#define BK2401_R0_ADDR     PCHAR(0x0887)
#define BK2401_R1_ADDR     PCHAR(0x088C)
#define BK2401_R2_ADDR     XBYTE[0x0891]
#define BK2401_R3_ADDR     XBYTE[0x0892]
#define BK2401_R4_ADDR     XBYTE[0x0893]
#define BK2401_R5_ADDR     XBYTE[0x0894]
#define BK2401_TX_ADDR     PCHAR(0x0895)

#define BK2401_R0_PW       XBYTE[0x089A]
#define BK2401_R1_PW       XBYTE[0x089B]
#define BK2401_R2_PW       XBYTE[0x089C]
#define BK2401_R3_PW       XBYTE[0x089D]
#define BK2401_R4_PW       XBYTE[0x089E]
#define BK2401_R5_PW       XBYTE[0x089F]
#define BK2401_DYNPD       XBYTE[0x08A0]
#define BK2401_FEATURE     XBYTE[0x08A1]

#define BK2401_CFG_0C_0      XBYTE[0x08A2]
#define BK2401_CFG_0C_ARRAY  PCHAR(0x08A2)
#define BK2401_CFG_0D_ARRAY  PCHAR(0x08A6)
#define BK2401_RAMP_TAB_ARRAY PCHAR(0x08AA)

#define BK2401_CE          XBYTE[0x08B5]
#define BK2401_CMD         XBYTE[0x08B6]
#define BK2401_FIFO        XBYTE[0x08B7]
#define BK2401_SDATA       PCHAR(0x08B8)
#define BK2401_SCTRL       XBYTE[0x08BC]
#define BK2401_KCALOUT_H   XBYTE[0x08BE]
#define BK2401_KCALOUT_L   XBYTE[0x08BF]
#define BK2401_STATUS      XBYTE[0x08C0]
#define BK2401_OBSERVETX   XBYTE[0x08C1]
#define BK2401_CDSTATUS    XBYTE[0x08C2]
#define BK2401_FIFOSTATUS  XBYTE[0x08C3]
#define BK2401_RPL_WIDTH   XBYTE[0x08C4]
#define BK2401_MBIST_ST    XBYTE[0x08C5]

#define TX_FREQ_OFFSET     PCHAR(0x08D0)
#define RX_FREQ_OFFSET     PCHAR(0x08D4)
#define MOD_DELAY          XBYTE[0x08D8]
#define PLL_SDM            XBYTE[0x08D9]
#define FM_GAIN_H          XBYTE[0x08DA]
#define FM_GAIN_L          XBYTE[0x08DB]
#define FM_KMOD_SET_H      XBYTE[0x08DC]
#define FM_KMOD_SET_L      XBYTE[0x08DD]
#define MOD_COEFFICIENT_H  XBYTE[0x08DE]
#define MOD_COEFFICIENT_L  XBYTE[0x08DF]
#define ANA_CTRL10         XBYTE[0x08E0]
#define ANA_CTRL11         XBYTE[0x08E1]
#define ANA_CTRL12         XBYTE[0x08E2]
#define ANA_CTRL13         XBYTE[0x08E3]
#define ANA_IND            XBYTE[0x08F9]
#define CHIP_ID_H          XBYTE[0x08FA]
#define CHIP_ID_L          XBYTE[0x08FB]

/* BK2535 NVR / Flash Controller Registers (0x0900..0x0906) */
#define FLASH_KEY          XBYTE[0x0900]
#define FLASH_CTL          XBYTE[0x0901]
#define FLASH_ADR          XBYTE[0x0902]
#define FLASH_ADR_H        XBYTE[0x0903]
#define FLASH_DAT          XBYTE[0x0904]
#define NVR_WP0            XBYTE[0x0905]
#define NVR_WP1            XBYTE[0x0906]

/* BK2401 Status & FIFO Bits */
#define STATUS_RX_DR           0x40
#define STATUS_TX_DS           0x20
#define STATUS_MAX_RT          0x10
#define STATUS_TX_FULL         0x01
#define FIFO_STATUS_TX_EMPTY   0x10
#define FIFO_STATUS_RX_FULL    0x02
#define FIFO_STATUS_RX_EMPTY   0x01

/* Common Utility Prototypes */
void Delay(int num);
void delay_us(uint16 us);
void delay_ms(uint16 ms);
void serial_Init(void);
void UartSendByte(uint8 val);
void UartPrintf(const char *str);
void UartPrintHex8(uint8 val);

#endif /* __BK2535_REG_H__ */
