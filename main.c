/*
 * main.c
 * BK2535 2.4G USB HID Dongle Entry Point (Paired with BK3632 Wireless Keyboard).
 * Developed independently for IROK AL87 Pro 2.4G wireless receiver.
 *
 * SPDX-License-Identifier: MIT
 */
#include "bk2535_reg.h"
#include "usb_dongle.h"
#include "rf_dongle.h"

/*
 * Beken BK2535 hardware errata reservation:
 * Reserve 0x2000..0x5FFF in Flash so code/constants stay in 0x0000..0x1FFF.
 */
__code __at(0x2000) const uint8 flash_bug_reserve[0x4000] = {0xFF};

void Delay(int num)
{
    int x, y;
    for (y = 0; y < num; y++)
    {
        for (x = 0; x < 500; x++)
        {
            ;
        }
    }
}

void delay_us(uint16 us)
{
    while (us--)
    {
        __asm nop __endasm;
        __asm nop __endasm;
    }
}

void delay_ms(uint16 ms)
{
    uint16 i;
    while (ms--)
    {
        for (i = 0; i < 1200; i++)
        {
            __asm nop __endasm;
        }
    }
}

#if ENABLE_UART_DEBUG
void serial_Init(void)
{
    uint16 reload;

    CLK_EN_CFG = 0xFF;  /* Enable UART and Timer clocks */
    PALT1 = 0x04;       /* Enable UART0 alternate function on P3.0/P3.1 */
    SCON0 = 0x50;       /* Mode 1 (8-bit UART), enable receiver */
    T2CON = 0x30;       /* Timer 2 baud rate generator mode */

    reload = (uint16)(65536UL - (16000000UL / 32UL) / 9600UL);
    RCAP2H = (uint8)(reload >> 8);
    RCAP2L = (uint8)(reload & 0xFF);
    TH2 = RCAP2H;
    TL2 = RCAP2L;
    TI = 0;
    TR2 = 1;
}

void UartSendByte(uint8 val)
{
    uint16 timeout = 8000;
    SBUF0 = val;
    while (!TI && timeout > 0)
    {
        timeout--;
    }
    TI = 0;
}

void UartPrintf(const char *str)
{
    while (*str != '\0')
    {
        UartSendByte((uint8)(*str));
        str++;
    }
}

void UartPrintHex8(uint8 val)
{
    static __code const char hex_lut[] = "0123456789ABCDEF";
    UartSendByte((uint8)hex_lut[(val >> 4) & 0x0F]);
    UartSendByte((uint8)hex_lut[val & 0x0F]);
}
#else
void serial_Init(void) {}
void UartSendByte(uint8 val) { (void)val; }
void UartPrintf(const char *str) { (void)str; }
void UartPrintHex8(uint8 val) { (void)val; }
#endif

static void driver_gpio_initial(void)
{
    /* Configure P0.2, P1.0, and P3.6 as push-pull outputs for LED/debug indication */
    P0IN_EN  = 0xFB;
    P0OUT_EN = 0xFB; /* Bit 0 = output enabled on BK2535 */
    P0_OPDR  = 0x00;
    P0_PU    = 0xFB;
    P0_PD    = 0x00;
    P0       = 0xFF;

    P1IN_EN  = 0xF2;
    P1OUT_EN = 0xFE;
    P1_OPDR  = 0x00;
    P1_PU    = 0xF2;
    P1_PD    = 0x00;
    P1       = 0xFF;

    P2IN_EN  = 0xFF;
    P2OUT_EN = 0xFF;
    P2_OPDR  = 0x00;
    P2_PU    = 0xFF;
    P2_PD    = 0x00;

    P3IN_EN  = 0x00;
    P3OUT_EN = 0x00;
    P3_OPDR  = 0x00;
    P3_PU    = 0x00;
    P3_PD    = 0x00;
    P3       = 0x00;

    P4IN_EN  = 0x00;
    P4OUT_EN = 0x00;
    P4_OPDR  = 0x00;
    P4_PU    = 0x00;
    P4_PD    = 0x00;
}

void main(void)
{
    EA = 0;
    Delay(1);
    PCON2 = 0x00;
    Delay(3);

    CKCON = 0x00;      /* 16MHz MCU clock */
    PCON2 = 0x40;      /* Clear stop, open 48MHz USB PLL */
    AIP |= 0x04;       /* High priority for USB interrupt */
    CLK_EN_CFG = 0xFF; /* Enable all peripheral clocks */

    driver_gpio_initial();
    serial_Init();
    usb_ini();
    rf_init(RF_AIR_DATA_RATE);

    EA = 1;

    UartPrintf("\r\n=== BK2535 2.4G USB Dongle Ready ===\r\n");
    UartPrintf("Addr: 10 11 36 25 35, Ch: ");
    UartPrintHex8(BK2401_RFCH);
    UartPrintf("\r\n");

    while (1)
    {
        rf_poll_receive();
        usb_dongle_process_kb_queue();
    }
}
