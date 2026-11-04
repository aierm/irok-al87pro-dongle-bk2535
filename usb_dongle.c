/*
 * usb_dongle.c
 * Clean-room USB 2.0 Full-Speed HID Keyboard implementation for BK2535.
 * Developed independently based on USB 2.0 / HID 1.11 specifications and
 * BK2535 hardware register definitions. Compiled with open-source SDCC.
 *
 * SPDX-License-Identifier: MIT
 */
#include "usb_dongle.h"
#include <string.h>

/* =========================================================================
 * USB Descriptors (Explicit Little-Endian Byte Arrays for SDCC)
 * ========================================================================= */
#define HID_REPORT_DESC_SIZE     160
#define CONFIG_DESC_TOTAL_SIZE   34

/* 18-byte USB Device Descriptor */
__code const uint8 DEVICEDESC[18] = {
    18,             /* bLength */
    0x01,           /* bDescriptorType (Device) */
    0x00, 0x02,     /* bcdUSB = 0x0200 (USB 2.0) */
    0x00,           /* bDeviceClass (Defined at interface level) */
    0x00,           /* bDeviceSubClass */
    0x00,           /* bDeviceProtocol */
    EP0_PACKET_SIZE,/* bMaxPacketSize0 (64) */
    0xA7, 0x24,     /* idVendor = 0x24A7 (Beken / Wireless HID) */
    0x35, 0x25,     /* idProduct = 0x2535 (BK2535 2.4G Dongle) */
    0x00, 0x01,     /* bcdDevice = 0x0100 */
    0x01,           /* iManufacturer */
    0x02,           /* iProduct */
    0x00,           /* iSerialNumber */
    0x01            /* bNumConfigurations */
};

/* 34-byte Configuration + Interface 0 (Composite HID: Keyboard + Consumer) + HID + EP1 IN Descriptor */
__code const uint8 HIDCONFIGDESC[CONFIG_DESC_TOTAL_SIZE] = {
    /* Configuration Descriptor (9 bytes) */
    0x09,                           /* bLength */
    0x02,                           /* bDescriptorType (Configuration) */
    CONFIG_DESC_TOTAL_SIZE, 0x00,   /* wTotalLength = 34 */
    0x01,                           /* bNumInterfaces = 1 */
    0x01,                           /* bConfigurationValue = 1 */
    0x00,                           /* iConfiguration */
    0xA0,                           /* bmAttributes (Bus Powered, Remote Wakeup) */
    0x32,                           /* bMaxPower = 100mA */

    /* Interface 0 Descriptor: Composite HID (9 bytes) */
    0x09,                           /* bLength */
    0x04,                           /* bDescriptorType (Interface) */
    0x00,                           /* bInterfaceNumber = 0 */
    0x00,                           /* bAlternateSetting = 0 */
    0x01,                           /* bNumEndpoints = 1 (EP1 IN) */
    0x03,                           /* bInterfaceClass = 0x03 (HID) */
    0x00,                           /* bInterfaceSubClass = 0x00 (No Subclass - Full Report Protocol) */
    0x00,                           /* bInterfaceProtocol = 0x00 (None - Multi-Report Device) */
    0x00,                           /* iInterface */

    /* HID Class Descriptor (9 bytes at offset 18) */
    0x09,                           /* bLength */
    0x21,                           /* bDescriptorType (HID) */
    0x11, 0x01,                     /* bcdHID = 0x0111 (1.11) */
    0x00,                           /* bCountryCode */
    0x01,                           /* bNumDescriptors = 1 */
    0x22,                           /* bDescriptorType (Report) */
    HID_REPORT_DESC_SIZE, 0x00,     /* wItemLength = 160 (0x00A0) */

    /* Endpoint 1 IN Descriptor (7 bytes at offset 27) */
    0x07,                           /* bLength */
    0x05,                           /* bDescriptorType (Endpoint) */
    0x81,                           /* bEndpointAddress = 0x81 (EP1 IN) */
    0x03,                           /* bmAttributes = 0x03 (Interrupt) */
    EP1_PACKET_SIZE, 0x00,          /* wMaxPacketSize = 64 */
    0x01                            /* bInterval = 1ms (1000Hz polling / High Report Rate) */
};

/* 160-byte Combined HID Report Descriptor:
 * - Report ID 1: 15-byte Compact NKRO Keyboard (1 Mod + 14 Bitmap Bytes = 112 keys) + 1-byte Output LED
 * - Report ID 2: 4-byte Consumer Control (30 media usages + 2 padding bits) matching BK3632 bitmask
 */
__code const uint8 HIDREPORTDESC[HID_REPORT_DESC_SIZE] = {
    /* -------------------------------------------------------------
     * Report ID 1: Keyboard (Generic Desktop / Keyboard)
     * ------------------------------------------------------------- */
    0x05, 0x01,     /* Usage Page (Generic Desktop) */
    0x09, 0x06,     /* Usage (Keyboard) */
    0xA1, 0x01,     /* Collection (Application) */
    0x85, 0x01,     /*   Report ID (1) */
    0x05, 0x07,     /*   Usage Page (Key Codes) */
    0x19, 0xE0,     /*   Usage Minimum (224 - Left Control) */
    0x29, 0xE7,     /*   Usage Maximum (231 - Right GUI) */
    0x15, 0x00,     /*   Logical Minimum (0) */
    0x25, 0x01,     /*   Logical Maximum (1) */
    0x75, 0x01,     /*   Report Size (1) */
    0x95, 0x08,     /*   Report Count (8) */
    0x81, 0x02,     /*   Input (Data, Variable, Absolute) ; Modifier byte (Byte 0) */
    0x05, 0x08,     /*   Usage Page (LEDs) */
    0x19, 0x01,     /*   Usage Minimum (1 - Num Lock) */
    0x29, 0x05,     /*   Usage Maximum (5 - Kana) */
    0x95, 0x05,     /*   Report Count (5) */
    0x75, 0x01,     /*   Report Size (1) */
    0x91, 0x02,     /*   Output (Data, Variable, Absolute) ; LED report (5 bits) */
    0x95, 0x01,     /*   Report Count (1) */
    0x75, 0x03,     /*   Report Size (3) */
    0x91, 0x03,     /*   Output (Constant, Variable, Absolute) ; LED padding (3 bits) */
    0x05, 0x07,     /*   Usage Page (Key Codes) */
    0x19, 0x04,     /*   Usage Minimum (0x04 - 'a') */
    0x29, 0x73,     /*   Usage Maximum (0x73 - 'F24') */
    0x15, 0x00,     /*   Logical Minimum (0) */
    0x25, 0x01,     /*   Logical Maximum (1) */
    0x75, 0x01,     /*   Report Size (1) */
    0x95, 0x70,     /*   Report Count (112 bits = 14 bytes) */
    0x81, 0x02,     /*   Input (Data, Variable, Absolute) ; Key bitmap (Bytes 1..14) */
    0xC0,           /* End Collection */

    /* -------------------------------------------------------------
     * Report ID 2: Consumer Control (Media Keys)
     * ------------------------------------------------------------- */
    0x05, 0x0C,     /* Usage Page (Consumer Devices) */
    0x09, 0x01,     /* Usage (Consumer Control) */
    0xA1, 0x01,     /* Collection (Application) */
    0x85, 0x02,     /*   Report ID (2) */
    0x15, 0x00,     /*   Logical Minimum (0) */
    0x25, 0x01,     /*   Logical Maximum (1) */
    0x75, 0x01,     /*   Report Size (1) */
    0x95, 0x1E,     /*   Report Count (30) */
    0x0A, 0x24, 0x02, /* WWW Back (0) */
    0x0A, 0x25, 0x02, /* WWW Forward (1) */
    0x0A, 0x26, 0x02, /* WWW Stop (2) */
    0x0A, 0x27, 0x02, /* WWW Refresh (3) */
    0x0A, 0x21, 0x02, /* WWW Search (4) */
    0x0A, 0x2A, 0x02, /* WWW Favorites (5) */
    0x0A, 0x23, 0x02, /* WWW Home (6) */
    0x0A, 0x8A, 0x01, /* Mail (7) */
    0x09, 0xE2,       /* Mute (8) */
    0x09, 0xEA,       /* Volume- (9) */
    0x09, 0xE9,       /* Volume+ (10) */
    0x09, 0xCD,       /* Play/Pause (11) */
    0x09, 0xB7,       /* Stop (12) */
    0x09, 0xB6,       /* Prev Track (13) */
    0x09, 0xB5,       /* Next Track (14) */
    0x0A, 0x83, 0x01, /* Media Select (15) */
    0x0A, 0x94, 0x01, /* My Computer (16) */
    0x0A, 0x92, 0x01, /* Calculator (17) */
    0x0A, 0x09, 0x02, /* More Info (18) */
    0x09, 0xB2,       /* Record (19) */
    0x09, 0xB3,       /* Forward (20) */
    0x09, 0xB4,       /* Rewind (21) */
    0x09, 0x8D,       /* Guide (22) */
    0x09, 0x04,       /* Reserved (23) */
    0x09, 0x30,       /* Eject (24) */
    0x0A, 0x07, 0x03, /* Touchkey H7 (25) */
    0x0A, 0x0A, 0x03, /* Brightness+ H10 (26) */
    0x0A, 0x0B, 0x03, /* Brightness- H11 (27) */
    0x0A, 0xB1, 0x01, /* Photo (28) */
    0x09, 0xB8,       /* Touchkey (29) */
    0x81, 0x02,       /* Input (Data, Variable, Absolute) ; 30 bits */
    0x95, 0x01,       /* Report Count (1) */
    0x75, 0x02,       /* Report Size (2) */
    0x81, 0x03,       /* Input (Constant, Variable, Absolute) ; 2 bits padding */
    0xC0              /* End Collection */
};

__code const uint8 String0Desc[4] = {
    4, 0x03, 0x09, 0x04
};

__code const uint8 String1Desc[12] = {
    12, 0x03,
    'B', 0, 'E', 0, 'K', 0, 'E', 0, 'N', 0
};

__code const uint8 String2Desc[38] = {
    38, 0x03,
    'B', 0, 'K', 0, '2', 0, '5', 0, '3', 0, '5', 0, ' ', 0,
    '2', 0, '.', 0, '4', 0, 'G', 0, ' ', 0,
    'D', 0, 'o', 0, 'n', 0, 'g', 0, 'l', 0, 'e', 0
};

/* =========================================================================
 * USB State Variables in XDATA
 * ========================================================================= */
volatile uint8 USB0_STATE = DEV_DEFAULT;
volatile uint8 EP_STATE[3] = {EP_IDLE, EP_IDLE, EP_IDLE};
volatile uint8 b_ep1_send_finish = 1;
volatile uint8 g_usb_led_status = 0;
volatile uint8 g_usb_led_updated = 0;

static __xdata uint8 setup_buf[8];
static __xdata uint8 ep0_out_buf[16];
static __xdata uint8 last_kb_report[16] = {0x01, 0};
static __xdata uint8 last_consumer_report[5] = {0x02, 0};
static __xdata uint8 status_reply[2] = {0, 0};
static __xdata uint8 idle_rate = 0;
static __xdata uint8 protocol_val = 1; /* 1 = Report Protocol, 0 = Boot Protocol */
static __xdata uint8 remote_wakeup = 0;

static const uint8 *DATAPTR;
static uint16 DATASIZE = 0;
static uint16 DATASENT = 0;
static uint8 setup_bmRequestType;
static uint8 setup_bRequest;
static uint8 setup_wValueL;
static uint8 setup_wValueH;
static uint8 setup_wIndexL;
static uint8 setup_wIndexH;
static uint16 setup_wLength;

/* =========================================================================
 * Low-level FIFO & Endpoint Helpers
 * ========================================================================= */
static void set_ep_rdy(uint8 ep_index)
{
    EP_RDY = (uint8)(0x01 << ep_index);
}

static void set_ep0_dir(uint8 dir_in)
{
    if (dir_in)
        CFG_EP0_1 |= 0x80;
    else
        CFG_EP0_1 &= ~0x80;
}

static void set_Wait_Status(void)
{
    EP_STATE[0] = EP_WAIT_STATUS;
    set_ep0_dir(!(CFG_EP0_1 & 0x80));
    CNT0 = 0;
}

static void set_Wait_Address(void)
{
    EP_STATE[0] = EP_SET_ADDRESS;
    set_ep0_dir(!(CFG_EP0_1 & 0x80));
    CNT0 = 0;
}

static void force_Stall(void)
{
    EP_STATE[0] = EP_STALL;
    EP_HALT |= 0x01;
}

static void fifo_Read(uint8 fifo_index, uint8 count, uint8 *dst)
{
    __xdata volatile uint8 *fifo_p;
    uint8 i;

    if (fifo_index == 0)
        fifo_p = &EP0_ADDR_DEF;
    else if (fifo_index == 1)
        fifo_p = &EP1_ADDR_DEF;
    else
        fifo_p = &EP2_ADDR_DEF;

    for (i = 0; i < count; i++)
    {
        dst[i] = fifo_p[i];
    }
}

static void fifo_Write(uint8 fifo_index, uint8 count, const uint8 *src)
{
    __xdata volatile uint8 *fifo_p;
    __xdata volatile uint8 *cnt_p;
    uint8 i;

    if (fifo_index == 0)
    {
        fifo_p = &EP0_ADDR_DEF;
        cnt_p  = &CNT0;
    }
    else if (fifo_index == 1)
    {
        fifo_p = &EP1_ADDR_DEF;
        cnt_p  = &CNT1;
    }
    else
    {
        fifo_p = &EP2_ADDR_DEF;
        cnt_p  = &CNT2;
    }

    for (i = 0; i < count; i++)
    {
        fifo_p[i] = src[i];
    }
    *cnt_p = count;
}

/* =========================================================================
 * USB Initialization & Keyboard Report Upload
 * ========================================================================= */
void usb_ini(void)
{
    USB_PWR_CN &= ~0x02; /* Clear USB suspend */
    while (CFG_EP0_1 != 0xC0)
    {
        CFG_EP0_1 = 0xC0; /* dir = OUT, ep0_en = 1 */
    }

    Delay(300);
    EP_ADDR_MSB = 0x3F;

    CFG_EP0_1 = 0xC0;
    CFG_EP0_0 = 0x00; /* FIFO0 @ 0x0400 */

    CFG_EP1_1 = 0xEC; /* dir = IN, mod = INT, ep_len = 64 */
    CFG_EP1_0 = 0x40; /* FIFO1 @ 0x0440 */

    CFG_EP2_1 = 0x6C; /* dir = OUT, mod = INT, ep_len = 64 */
    CFG_EP2_0 = 0x80; /* FIFO2 @ 0x0480 */

    USBINT0 = 0xFF;
    USBINT1 = 0xFF;

    USB_EN0 = 0xF8; /* Enable EP0 RX/TX, EP1-7 RX/TX, USB Reset */
    USB_EN1 = 0x00;

    USB0_STATE = DEV_DEFAULT;
    EP_STATE[0] = EP_IDLE;
    EP_STATE[1] = EP_IDLE;
    EP_STATE[2] = EP_IDLE;
    b_ep1_send_finish = 1;

    set_ep_rdy(EP_2);
    USB_PWR_CN |= 0x80; /* Enable internal D+ pull-up */
    EX4 = 1;            /* Enable USB interrupt (interrupt 10) */
}

#define HID_QUEUE_CAP 16
#define HID_REPORT_MAX_LEN 16

typedef struct {
    uint8 len;
    uint8 buf[HID_REPORT_MAX_LEN];
} hid_report_pkt_t;

static __xdata hid_report_pkt_t s_hid_queue[HID_QUEUE_CAP];
static __xdata uint8 s_hid_q_head = 0;
static __xdata uint8 s_hid_q_tail = 0;

void usb_dongle_process_kb_queue(void)
{
    if (b_ep1_send_finish && (s_hid_q_head != s_hid_q_tail) && (USB0_STATE == DEV_CONFIGURED))
    {
        uint8 len = s_hid_queue[s_hid_q_tail].len;
        b_ep1_send_finish = 0;
        fifo_Write(EP_1, len, s_hid_queue[s_hid_q_tail].buf);
        s_hid_q_tail = (s_hid_q_tail + 1) % HID_QUEUE_CAP;
        set_ep_rdy(EP_1);
    }
}

uint8 usb_send_keyboard_report(const uint8 *report, uint8 len)
{
    uint8 next_head;
    uint8 i;
    __xdata uint8 buf[15];

    if (USB0_STATE != DEV_CONFIGURED)
    {
        return FALSE;
    }

    if (len >= 15)
    {
        for (i = 0; i < 15; i++) buf[i] = report[i];
    }
    else if (len == 7 || len == 8)
    {
        /* Legacy 6KRO conversion -> 15-byte NKRO bitmap */
        for (i = 0; i < 15; i++) buf[i] = 0;
        buf[0] = report[0];
        if (len == 7)
        {
            /* [mod, k0..k5] without reserved byte */
            for (i = 1; i < 7; i++)
            {
                uint8 k = report[i];
                if (k >= 4 && k <= 0x73)
                {
                    uint8 n = k - 4;
                    buf[1 + (n >> 3)] |= (uint8)(1 << (n & 7));
                }
            }
        }
        else
        {
            /* Standard [mod, rsvd, k0..k5] */
            for (i = 2; i < 8; i++)
            {
                uint8 k = report[i];
                if (k >= 4 && k <= 0x73)
                {
                    uint8 n = k - 4;
                    buf[1 + (n >> 3)] |= (uint8)(1 << (n & 7));
                }
            }
        }
    }
    else
    {
        for (i = 0; i < 15; i++) buf[i] = 0;
    }

    EX4 = 0;
    next_head = (s_hid_q_head + 1) % HID_QUEUE_CAP;
    if (next_head != s_hid_q_tail)
    {
        s_hid_queue[s_hid_q_head].buf[0] = 0x01; /* Report ID 1 (Keyboard) */
        for (i = 0; i < 15; i++)
        {
            s_hid_queue[s_hid_q_head].buf[1 + i] = buf[i];
            last_kb_report[1 + i] = buf[i];
        }
        last_kb_report[0] = 0x01;
        s_hid_queue[s_hid_q_head].len = 16;
        s_hid_q_head = next_head;
    }
    else
    {
        uint8 last_idx = (s_hid_q_head + HID_QUEUE_CAP - 1) % HID_QUEUE_CAP;
        s_hid_queue[last_idx].buf[0] = 0x01;
        for (i = 0; i < 15; i++)
        {
            s_hid_queue[last_idx].buf[1 + i] = buf[i];
            last_kb_report[1 + i] = buf[i];
        }
        last_kb_report[0] = 0x01;
        s_hid_queue[last_idx].len = 16;
    }
    usb_dongle_process_kb_queue();
    EX4 = 1;
    return TRUE;
}

uint8 usb_send_consumer_report(const uint8 *report, uint8 len)
{
    uint8 next_head;
    uint8 i;

    if (USB0_STATE != DEV_CONFIGURED)
    {
        return FALSE;
    }

    EX4 = 0;
    next_head = (s_hid_q_head + 1) % HID_QUEUE_CAP;
    if (next_head != s_hid_q_tail)
    {
        s_hid_queue[s_hid_q_head].buf[0] = 0x02; /* Report ID 2 (Consumer) */
        for (i = 0; i < 4; i++)
        {
            uint8 b = (i < len) ? report[i] : 0;
            s_hid_queue[s_hid_q_head].buf[1 + i] = b;
            last_consumer_report[1 + i] = b;
        }
        last_consumer_report[0] = 0x02;
        s_hid_queue[s_hid_q_head].len = 5;
        s_hid_q_head = next_head;
    }
    else
    {
        uint8 last_idx = (s_hid_q_head + HID_QUEUE_CAP - 1) % HID_QUEUE_CAP;
        s_hid_queue[last_idx].buf[0] = 0x02;
        for (i = 0; i < 4; i++)
        {
            uint8 b = (i < len) ? report[i] : 0;
            s_hid_queue[last_idx].buf[1 + i] = b;
            last_consumer_report[1 + i] = b;
        }
        last_consumer_report[0] = 0x02;
        s_hid_queue[last_idx].len = 5;
    }
    usb_dongle_process_kb_queue();
    EX4 = 1;
    return TRUE;
}

/* =========================================================================
 * Control Endpoint (EP0) Standard & HID Class Request Handlers
 * ========================================================================= */
static void handle_Get_Descriptor(void)
{
    switch (setup_wValueH)
    {
    case DSC_DEVICE:
        DATAPTR  = DEVICEDESC;
        DATASIZE = sizeof(DEVICEDESC);
        break;

    case DSC_CONFIG:
        DATAPTR  = HIDCONFIGDESC;
        DATASIZE = CONFIG_DESC_TOTAL_SIZE;
        break;

    case DSC_STRING:
        if (setup_wValueL == 0)
        {
            DATAPTR  = String0Desc;
            DATASIZE = String0Desc[0];
        }
        else if (setup_wValueL == 1)
        {
            DATAPTR  = String1Desc;
            DATASIZE = String1Desc[0];
        }
        else if (setup_wValueL == 2)
        {
            DATAPTR  = String2Desc;
            DATASIZE = String2Desc[0];
        }
        else
        {
            force_Stall();
            return;
        }
        break;

    case DSC_INTERFACE:
        DATAPTR  = &HIDCONFIGDESC[9];
        DATASIZE = 9;
        break;

    case DSC_HID:
        DATAPTR  = &HIDCONFIGDESC[18];
        DATASIZE = 9;
        break;

    case DSC_ENDPOINT:
        DATAPTR  = &HIDCONFIGDESC[27];
        DATASIZE = 7;
        break;

    case DSC_HID_REPORT:
        DATAPTR  = HIDREPORTDESC;
        DATASIZE = HID_REPORT_DESC_SIZE;
        break;

    default:
        force_Stall();
        return;
    }

    if (setup_wLength < DATASIZE)
    {
        DATASIZE = setup_wLength;
    }
    EP_STATE[0] = EP_TX;
    DATASENT = 0;
}

static void handle_Control(void)
{
    if ((EP_STATUS_OUT & 0x01) || (EP_STATUS_IN & 0x01))
    {
        EP_STATUS_OUT = 0x01;
        EP_STATUS_IN  = 0x01;
        EP_STATE[0]   = EP_IDLE;

        if (CNT0 == 8)
        {
            fifo_Read(EP_0, 8, setup_buf);
            setup_bmRequestType = setup_buf[0];
            setup_bRequest      = setup_buf[1];
            setup_wValueL       = setup_buf[2];
            setup_wValueH       = setup_buf[3];
            setup_wIndexL       = setup_buf[4];
            setup_wIndexH       = setup_buf[5];
            setup_wLength       = (uint16)setup_buf[6] | ((uint16)setup_buf[7] << 8);

            if ((setup_bmRequestType & 0x60) == 0x20)
            {
                /* HID Class-Specific Request */
                switch (setup_bRequest)
                {
                case GET_REPORT:
                    if (setup_wValueL == 0x02)
                    {
                        DATAPTR  = last_consumer_report;
                        DATASIZE = (setup_wLength < 5) ? setup_wLength : 5;
                    }
                    else
                    {
                        DATAPTR  = last_kb_report;
                        DATASIZE = (setup_wLength < 16) ? setup_wLength : 16;
                    }
                    EP_STATE[0] = EP_TX;
                    DATASENT = 0;
                    break;

                case SET_REPORT:
                    DATAPTR  = ep0_out_buf;
                    DATASIZE = (setup_wLength < sizeof(ep0_out_buf)) ? setup_wLength : sizeof(ep0_out_buf);
                    EP_STATE[0] = EP_RX;
                    DATASENT = 0;
                    break;

                case GET_IDLE:
                    DATAPTR  = &idle_rate;
                    DATASIZE = 1;
                    EP_STATE[0] = EP_TX;
                    DATASENT = 0;
                    break;

                case SET_IDLE:
                    idle_rate = setup_wValueH;
                    set_Wait_Status();
                    break;

                case GET_PROTOCOL:
                    DATAPTR  = &protocol_val;
                    DATASIZE = 1;
                    EP_STATE[0] = EP_TX;
                    DATASENT = 0;
                    break;

                case SET_PROTOCOL:
                    protocol_val = setup_wValueL;
                    set_Wait_Status();
                    break;

                default:
                    force_Stall();
                    break;
                }
            }
            else if ((setup_bmRequestType & 0x60) == 0x00)
            {
                /* Standard USB Request */
                switch (setup_bRequest)
                {
                case GET_STATUS:
                    status_reply[0] = 0x00;
                    status_reply[1] = 0x00;
                    if (setup_bmRequestType == 0x80 && remote_wakeup)
                    {
                        status_reply[0] = 0x02;
                    }
                    else if (setup_bmRequestType == 0x82 && setup_wIndexL == 0x81 && EP_STATE[1] == EP_STALL)
                    {
                        status_reply[0] = 0x01;
                    }
                    DATAPTR  = status_reply;
                    DATASIZE = 2;
                    EP_STATE[0] = EP_TX;
                    DATASENT = 0;
                    break;

                case CLEAR_FEATURE:
                    if (setup_bmRequestType == 0x00 && setup_wValueL == 0x01)
                    {
                        remote_wakeup = 0;
                        set_Wait_Status();
                    }
                    else if (setup_bmRequestType == 0x02 && setup_wValueL == 0x00)
                    {
                        EP_HALT &= ~0x02;
                        EP_STATE[1] = EP_IDLE;
                        set_Wait_Status();
                    }
                    else
                    {
                        force_Stall();
                    }
                    break;

                case SET_FEATURE:
                    if (setup_bmRequestType == 0x00 && setup_wValueL == 0x01)
                    {
                        remote_wakeup = 1;
                        set_Wait_Status();
                    }
                    else if (setup_bmRequestType == 0x02 && setup_wValueL == 0x00)
                    {
                        EP_HALT |= 0x02;
                        EP_STATE[1] = EP_STALL;
                        set_Wait_Status();
                    }
                    else
                    {
                        force_Stall();
                    }
                    break;

                case SET_ADDRESS:
                    if (setup_wValueL != 0)
                        USB0_STATE = DEV_ADDRESS;
                    else
                        USB0_STATE = DEV_DEFAULT;
                    set_Wait_Address();
                    break;

                case GET_DESCRIPTOR:
                    handle_Get_Descriptor();
                    break;

                case GET_CONFIGURATION:
                    status_reply[0] = (USB0_STATE == DEV_CONFIGURED) ? 0x01 : 0x00;
                    DATAPTR  = status_reply;
                    DATASIZE = 1;
                    EP_STATE[0] = EP_TX;
                    DATASENT = 0;
                    break;

                case SET_CONFIGURATION:
                    if (setup_wValueL > 0)
                    {
                        USB0_STATE = DEV_CONFIGURED;
                        EP_STATE[1] = EP_IDLE;
                        EP_STATE[2] = EP_IDLE;
                        s_hid_q_head = 0;
                        s_hid_q_tail = 0;
                        b_ep1_send_finish = 1;
                    }
                    else
                    {
                        USB0_STATE = DEV_ADDRESS;
                        EP_STATE[1] = EP_STALL;
                    }
                    set_Wait_Status();
                    break;

                case GET_INTERFACE:
                    status_reply[0] = 0x00;
                    DATAPTR  = status_reply;
                    DATASIZE = 1;
                    EP_STATE[0] = EP_TX;
                    DATASENT = 0;
                    break;

                case SET_INTERFACE:
                    set_Wait_Status();
                    break;

                default:
                    force_Stall();
                    break;
                }
            }
            else
            {
                force_Stall();
            }
        }
        else
        {
            force_Stall();
        }
    }
    else if (EP_STATE[0] == EP_SET_ADDRESS)
    {
        FUNCT_ADR = setup_wValueL;
        EP_STATE[0] = EP_IDLE;
    }
    else if (EP_STATE[0] == EP_WAIT_STATUS)
    {
        EP_STATE[0] = EP_IDLE;
    }
    else if (EP_STATE[0] == EP_RX)
    {
        uint8 rx_cnt = (DATASIZE >= EP0_PACKET_SIZE) ? EP0_PACKET_SIZE : (uint8)DATASIZE;
        fifo_Read(EP_0, rx_cnt, ep0_out_buf);
        DATASIZE -= rx_cnt;
        DATASENT += rx_cnt;
        if (DATASIZE == 0 || DATASENT >= setup_wLength)
        {
            set_Wait_Status();
            if (setup_bRequest == SET_REPORT)
            {
                if (setup_wLength >= 2 && ep0_out_buf[0] == 0x01)
                    g_usb_led_status  = ep0_out_buf[1];
                else
                    g_usb_led_status  = ep0_out_buf[0];
                g_usb_led_updated = 1;
            }
        }
    }

    if (EP_STATE[0] == EP_TX)
    {
        set_ep0_dir(1);
        if (DATASIZE == 0 || DATASENT == setup_wLength)
        {
            set_Wait_Status();
        }
        else if (DATASIZE >= EP0_PACKET_SIZE)
        {
            fifo_Write(EP_0, EP0_PACKET_SIZE, DATAPTR);
            DATAPTR  += EP0_PACKET_SIZE;
            DATASIZE -= EP0_PACKET_SIZE;
            DATASENT += EP0_PACKET_SIZE;
        }
        else
        {
            fifo_Write(EP_0, (uint8)DATASIZE, DATAPTR);
            DATASENT += DATASIZE;
            DATASIZE = 0;
        }
    }

    USBINT0 = 0xC0;
    set_ep_rdy(EP_0);
}

void Usb_ISR(void) __interrupt(10)
{
    uint8 int0 = USBINT0 & USB_EN0;

    if (int0 & 0xC0) /* EP0 RX or TX interrupt */
    {
        handle_Control();
    }
    if (int0 & 0x20) /* EP1-7 OUT interrupt */
    {
        if (EP_STATUS_OUT & 0x04) /* EP2 OUT */
        {
            uint8 len = CNT2;
            if (len > 0 && len <= sizeof(ep0_out_buf))
            {
                fifo_Read(EP_2, len, ep0_out_buf);
                g_usb_led_status  = ep0_out_buf[0];
                g_usb_led_updated = 1;
            }
            EP_STATUS_OUT = 0x04;
            set_ep_rdy(EP_2);
        }
        else
        {
            EP_STATUS_OUT = 0xFE;
        }
        USBINT0 = 0x20;
    }
    if (int0 & 0x10) /* EP1-7 IN interrupt */
    {
        if (EP_STATUS_IN & 0x02) /* EP1 IN finished */
        {
            EP_STATUS_IN = 0x02;
            b_ep1_send_finish = 1;
            usb_dongle_process_kb_queue();
        }
        else
        {
            EP_STATUS_IN = 0xFE;
        }
        USBINT0 = 0x10;
    }
    if (int0 & 0x08) /* USB Reset interrupt */
    {
        PCON2 |= 0x40;
        USB_PWR_CN &= ~0x02;
        USB_PWR_CN |= 0x04;
        FUNCT_ADR = 0;
        USB0_STATE = DEV_DEFAULT;
        EP_STATE[0] = EP_IDLE;
        EP_STATE[1] = EP_IDLE;
        EP_STATE[2] = EP_IDLE;
        s_hid_q_head = 0;
        s_hid_q_tail = 0;
        b_ep1_send_finish = 1;
        USBINT0 = 0x08;
    }

    AIF &= ~0x04; /* Clear MCU USB interrupt flag */
}
