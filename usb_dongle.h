/*
 * usb_dongle.h
 * USB 2.0 Full-Speed HID Keyboard interface for BK2535 Dongle
 */
#ifndef __USB_DONGLE_H__
#define __USB_DONGLE_H__

#include "bk2535_reg.h"

#define EP0_PACKET_SIZE    64
#define EP1_PACKET_SIZE    64

/* Endpoint indices */
#define EP_0               0
#define EP_1               1
#define EP_2               2

/* USB Device States */
#define DEV_ATTACHED       0x00
#define DEV_POWERED        0x01
#define DEV_DEFAULT        0x02
#define DEV_ADDRESS        0x03
#define DEV_CONFIGURED     0x04
#define DEV_SUSPENDED      0x05

/* Endpoint States */
#define EP_IDLE            0x00
#define EP_TX              0x01
#define EP_RX              0x02
#define EP_HALT_STATE      0x03
#define EP_STALL           0x04
#define EP_WAIT_STATUS     0x05
#define EP_SET_ADDRESS     0x06

/* Standard USB Requests */
#define GET_STATUS         0x00
#define CLEAR_FEATURE      0x01
#define SET_FEATURE        0x03
#define SET_ADDRESS        0x05
#define GET_DESCRIPTOR     0x06
#define SET_DESCRIPTOR     0x07
#define GET_CONFIGURATION  0x08
#define SET_CONFIGURATION  0x09
#define GET_INTERFACE      0x0A
#define SET_INTERFACE      0x0B

/* HID Class Requests */
#define GET_REPORT         0x01
#define GET_IDLE           0x02
#define GET_PROTOCOL       0x03
#define SET_REPORT         0x09
#define SET_IDLE           0x0A
#define SET_PROTOCOL       0x0B

/* Descriptor Types */
#define DSC_DEVICE         0x01
#define DSC_CONFIG         0x02
#define DSC_STRING         0x03
#define DSC_INTERFACE      0x04
#define DSC_ENDPOINT       0x05
#define DSC_HID            0x21
#define DSC_HID_REPORT     0x22

#define KB_REPORT_SIZE     15  /* 15-byte Compact NKRO Report (1 mod + 14 bitmap bytes) */

extern volatile uint8 USB0_STATE;
extern volatile uint8 b_ep1_send_finish;
extern volatile uint8 g_usb_led_status;
extern volatile uint8 g_usb_led_updated;

void usb_ini(void);
void Usb_ISR(void) __interrupt(10);
void usb_dongle_process_kb_queue(void);
uint8 usb_send_keyboard_report(const uint8 *report, uint8 len);
uint8 usb_send_consumer_report(const uint8 *report, uint8 len);

#endif /* __USB_DONGLE_H__ */
