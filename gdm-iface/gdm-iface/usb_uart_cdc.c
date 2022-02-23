/* This file is the part of the Lightweight USB device Stack for STM32 microcontrollers
 *
 * Copyright ©2016 Dmitry Filimonchuk <dmitrystu[at]gmail[dot]com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *   http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "stm32.h"
#include "usb.h"
#include "usb_cdc.h"

#include "fifo.h"
#include "ub.h"

#include "usb_uart_cdc.h"

#define CDC_EP0_SIZE    0x08

#define CDC1_RXD_EP      0x01
#define CDC1_TXD_EP      0x81
#define CDC1_DATA_SZ     0x40
#define CDC1_NTF_EP      0x82
#define CDC1_NTF_SZ      0x08

#define CDC2_RXD_EP      0x03
#define CDC2_TXD_EP      0x83
#define CDC2_DATA_SZ     0x40
#define CDC2_NTF_EP      0x84
#define CDC2_NTF_SZ      0x08

#define CDC_PROTOCOL USB_PROTO_NONE

/* Declaration of the report descriptor */
struct cdc_config {
    struct usb_config_descriptor        config;
		/* first cdc serial port */
    struct usb_iad_descriptor           comm_iad1;
    struct usb_interface_descriptor     comm1;
    struct usb_cdc_header_desc          cdc_hdr1;
    struct usb_cdc_call_mgmt_desc       cdc_mgmt1;
    struct usb_cdc_acm_desc             cdc_acm1;
    struct usb_cdc_union_desc           cdc_union1;
    struct usb_endpoint_descriptor      comm_ep1;
    struct usb_interface_descriptor     data1;
    struct usb_endpoint_descriptor      data_eprx1;
    struct usb_endpoint_descriptor      data_eptx1;
		/* second cdc serial port */
    struct usb_iad_descriptor           comm_iad2;
    struct usb_interface_descriptor     comm2;
    struct usb_cdc_header_desc          cdc_hdr2;
    struct usb_cdc_call_mgmt_desc       cdc_mgmt2;
    struct usb_cdc_acm_desc             cdc_acm2;
    struct usb_cdc_union_desc           cdc_union2;
    struct usb_endpoint_descriptor      comm_ep2;
    struct usb_interface_descriptor     data2;
    struct usb_endpoint_descriptor      data_eprx2;
    struct usb_endpoint_descriptor      data_eptx2;
} __attribute__((packed));

/* Device descriptor */
static const struct usb_device_descriptor device_desc = {
    .bLength            = sizeof(struct usb_device_descriptor),
    .bDescriptorType    = USB_DTYPE_DEVICE,
    .bcdUSB             = VERSION_BCD(2,0,0),
    .bDeviceClass       = USB_CLASS_IAD,
    .bDeviceSubClass    = USB_SUBCLASS_IAD,
    .bDeviceProtocol    = USB_PROTO_IAD,
    .bMaxPacketSize0    = CDC_EP0_SIZE,
    .idVendor           = 0x1209,
    .idProduct          = 0x2060,
    .bcdDevice          = VERSION_BCD(1,0,0),
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = INTSERIALNO_DESCRIPTOR,
    .bNumConfigurations = 1,
};

/* Device configuration descriptor */
static const struct cdc_config config_desc = {
    .config = {
        .bLength                = sizeof(struct usb_config_descriptor),
        .bDescriptorType        = USB_DTYPE_CONFIGURATION,
        .wTotalLength           = sizeof(struct cdc_config),
        .bNumInterfaces         = 4, // 2* 2per cdc
        .bConfigurationValue    = 1,
        .iConfiguration         = NO_DESCRIPTOR,
        .bmAttributes           = USB_CFG_ATTR_RESERVED | USB_CFG_ATTR_SELFPOWERED,
        .bMaxPower              = USB_CFG_POWER_MA(100),
    },
    .comm_iad1 = {
        .bLength = sizeof(struct usb_iad_descriptor),
        .bDescriptorType        = USB_DTYPE_INTERFASEASSOC,
        .bFirstInterface        = 0,
        .bInterfaceCount        = 2,
        .bFunctionClass         = USB_CLASS_CDC,
        .bFunctionSubClass      = USB_CDC_SUBCLASS_ACM,
        .bFunctionProtocol      = CDC_PROTOCOL,
        .iFunction              = NO_DESCRIPTOR,
    },
    .comm1 = {
        .bLength                = sizeof(struct usb_interface_descriptor),
        .bDescriptorType        = USB_DTYPE_INTERFACE,
        .bInterfaceNumber       = 0,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 1,
        .bInterfaceClass        = USB_CLASS_CDC,
        .bInterfaceSubClass     = USB_CDC_SUBCLASS_ACM,
        .bInterfaceProtocol     = CDC_PROTOCOL,
        .iInterface             = NO_DESCRIPTOR,
    },
    .cdc_hdr1 = {
        .bFunctionLength        = sizeof(struct usb_cdc_header_desc),
        .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType     = USB_DTYPE_CDC_HEADER,
        .bcdCDC                 = VERSION_BCD(1,1,0),
    },
    .cdc_mgmt1 = {
        .bFunctionLength        = sizeof(struct usb_cdc_call_mgmt_desc),
        .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType     = USB_DTYPE_CDC_CALL_MANAGEMENT,
        .bmCapabilities         = 0,
        .bDataInterface         = 1,

    },
    .cdc_acm1 = {
        .bFunctionLength        = sizeof(struct usb_cdc_acm_desc),
        .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType     = USB_DTYPE_CDC_ACM,
        .bmCapabilities         = 0,
    },
    .cdc_union1 = {
        .bFunctionLength        = sizeof(struct usb_cdc_union_desc),
        .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType     = USB_DTYPE_CDC_UNION,
        .bMasterInterface0      = 0,
        .bSlaveInterface0       = 1,
    },
    .comm_ep1 = {
        .bLength                = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType        = USB_DTYPE_ENDPOINT,
        .bEndpointAddress       = CDC1_NTF_EP,
        .bmAttributes           = USB_EPTYPE_INTERRUPT,
        .wMaxPacketSize         = CDC1_NTF_SZ,
        .bInterval              = 0xFF,
    },
    .data1 = {
        .bLength                = sizeof(struct usb_interface_descriptor),
        .bDescriptorType        = USB_DTYPE_INTERFACE,
        .bInterfaceNumber       = 1,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 2,
        .bInterfaceClass        = USB_CLASS_CDC_DATA,
        .bInterfaceSubClass     = USB_SUBCLASS_NONE,
        .bInterfaceProtocol     = USB_PROTO_NONE,
        .iInterface             = NO_DESCRIPTOR,
    },
    .data_eprx1 = {
        .bLength                = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType        = USB_DTYPE_ENDPOINT,
        .bEndpointAddress       = CDC1_RXD_EP,
        .bmAttributes           = USB_EPTYPE_BULK,
        .wMaxPacketSize         = CDC1_DATA_SZ,
        .bInterval              = 0x01,
    },
    .data_eptx1 = {
        .bLength                = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType        = USB_DTYPE_ENDPOINT,
        .bEndpointAddress       = CDC1_TXD_EP,
        .bmAttributes           = USB_EPTYPE_BULK,
        .wMaxPacketSize         = CDC1_DATA_SZ,
        .bInterval              = 0x01,
    },
    .comm_iad2 = {
        .bLength = sizeof(struct usb_iad_descriptor),
        .bDescriptorType        = USB_DTYPE_INTERFASEASSOC,
        .bFirstInterface        = 2,
        .bInterfaceCount        = 2,
        .bFunctionClass         = USB_CLASS_CDC,
        .bFunctionSubClass      = USB_CDC_SUBCLASS_ACM,
        .bFunctionProtocol      = CDC_PROTOCOL,
        .iFunction              = NO_DESCRIPTOR,
    },
    .comm2 = {
        .bLength                = sizeof(struct usb_interface_descriptor),
        .bDescriptorType        = USB_DTYPE_INTERFACE,
        .bInterfaceNumber       = 2,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 1,
        .bInterfaceClass        = USB_CLASS_CDC,
        .bInterfaceSubClass     = USB_CDC_SUBCLASS_ACM,
        .bInterfaceProtocol     = CDC_PROTOCOL,
        .iInterface             = NO_DESCRIPTOR,
    },
    .cdc_hdr2 = {
        .bFunctionLength        = sizeof(struct usb_cdc_header_desc),
        .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType     = USB_DTYPE_CDC_HEADER,
        .bcdCDC                 = VERSION_BCD(1,1,0),
    },
    .cdc_mgmt2 = {
        .bFunctionLength        = sizeof(struct usb_cdc_call_mgmt_desc),
        .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType     = USB_DTYPE_CDC_CALL_MANAGEMENT,
        .bmCapabilities         = 0,
        .bDataInterface         = 2,

    },
    .cdc_acm2 = {
        .bFunctionLength        = sizeof(struct usb_cdc_acm_desc),
        .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType     = USB_DTYPE_CDC_ACM,
        .bmCapabilities         = 0,
    },
    .cdc_union2 = {
        .bFunctionLength        = sizeof(struct usb_cdc_union_desc),
        .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType     = USB_DTYPE_CDC_UNION,
        .bMasterInterface0      = 2,
        .bSlaveInterface0       = 3,
    },
    .comm_ep2 = {
        .bLength                = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType        = USB_DTYPE_ENDPOINT,
        .bEndpointAddress       = CDC2_NTF_EP,
        .bmAttributes           = USB_EPTYPE_INTERRUPT,
        .wMaxPacketSize         = CDC2_NTF_SZ,
        .bInterval              = 0xFF,
    },
    .data2 = {
        .bLength                = sizeof(struct usb_interface_descriptor),
        .bDescriptorType        = USB_DTYPE_INTERFACE,
        .bInterfaceNumber       = 3,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 2,
        .bInterfaceClass        = USB_CLASS_CDC_DATA,
        .bInterfaceSubClass     = USB_SUBCLASS_NONE,
        .bInterfaceProtocol     = USB_PROTO_NONE,
        .iInterface             = NO_DESCRIPTOR,
    },
    .data_eprx2 = {
        .bLength                = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType        = USB_DTYPE_ENDPOINT,
        .bEndpointAddress       = CDC2_RXD_EP,
        .bmAttributes           = USB_EPTYPE_BULK,
        .wMaxPacketSize         = CDC2_DATA_SZ,
        .bInterval              = 0x01,
    },
    .data_eptx2 = {
        .bLength                = sizeof(struct usb_endpoint_descriptor),
        .bDescriptorType        = USB_DTYPE_ENDPOINT,
        .bEndpointAddress       = CDC2_TXD_EP,
        .bmAttributes           = USB_EPTYPE_BULK,
        .wMaxPacketSize         = CDC2_DATA_SZ,
        .bInterval              = 0x01,
    },
};

static const struct usb_string_descriptor lang_desc     = USB_ARRAY_DESC(USB_LANGID_ENG_US);
static const struct usb_string_descriptor manuf_desc_en = USB_STRING_DESC("Geodimeter Iface");
static const struct usb_string_descriptor prod_desc_en  = USB_STRING_DESC("Geodimeter Iface");
static const struct usb_string_descriptor *const dtable[] = {
    &lang_desc,
    &manuf_desc_en,
    &prod_desc_en,
};

usbd_device udev;
uint32_t	ubuf[0x20];
uint8_t rxbuf[CDC1_DATA_SZ];

struct fifo_t cdc1_tx_fifo;
uint8_t fifo_tx1_buf[0x200];
struct fifo_t usb_rx1_fifo;
uint8_t fifo_rx1_buf[0x200];

struct fifo_t cdc2_tx_fifo;
uint8_t fifo_tx2_buf[0x200];
struct fifo_t usb_rx2_fifo;
uint8_t fifo_rx2_buf[0x200];

volatile uint8_t cdc1_tx_state = 0;
volatile uint8_t cdc2_tx_state = 0;


static struct usb_cdc_line_coding cdc_line = {
    .dwDTERate          = 38400,
    .bCharFormat        = USB_CDC_1_STOP_BITS,
    .bParityType        = USB_CDC_NO_PARITY,
    .bDataBits          = 8,
};

static usbd_respond cdc_getdesc (usbd_ctlreq *req, void **address, uint16_t *length) {
    const uint8_t dtype = req->wValue >> 8;
    const uint8_t dnumber = req->wValue & 0xFF;
    const void* desc;
    uint16_t len = 0;
    switch (dtype) {
    case USB_DTYPE_DEVICE:
        desc = &device_desc;
        break;
    case USB_DTYPE_CONFIGURATION:
        desc = &config_desc;
        len = sizeof(config_desc);
        break;
    case USB_DTYPE_STRING:
        if (dnumber < 3) {
            desc = dtable[dnumber];
        } else {
            return usbd_fail;
        }
        break;
    default:
        return usbd_fail;
    }
    if (len == 0) {
        len = ((struct usb_header_descriptor*)desc)->bLength;
    }
    *address = (void*)desc;
    *length = len;
    return usbd_ack;
};


static usbd_respond cdc_control(usbd_device *dev, usbd_ctlreq *req, usbd_rqc_callback *callback) {
	(void) callback;

    if (((USB_REQ_RECIPIENT | USB_REQ_TYPE) & req->bmRequestType) == (USB_REQ_INTERFACE | USB_REQ_CLASS)
        /*&& req->wIndex == 0 */) {
        switch (req->bRequest) {
        case USB_CDC_SET_CONTROL_LINE_STATE:
            return usbd_ack;
        case USB_CDC_SET_LINE_CODING:
            if (req->wIndex == 0) {
							memcpy(&cdc_line, req->data, sizeof(cdc_line));
						} else if (req->wIndex == 2) {
							memcpy(&ub_cdc_line, req->data, sizeof(cdc_line));
							ub_apply_setting();
						}
            return usbd_ack;
        case USB_CDC_GET_LINE_CODING:
            if (req->wIndex == 0) {
							dev->status.data_ptr = &cdc_line;
						} else {
							dev->status.data_ptr = &ub_cdc_line;
						}
            dev->status.data_count = sizeof(cdc_line);
            return usbd_ack;
        default:
            return usbd_fail;
        }
    }
    return usbd_fail;
}

static void cdc_rxtx(usbd_device *dev, uint8_t event, uint8_t ep)
{
	(void)event;
	if (ep == CDC1_RXD_EP) {
		int len = usbd_ep_read(dev, ep, rxbuf, CDC1_DATA_SZ);

		if (len == -1) {
			return;
		}

		fifo_write_buf(&usb_rx1_fifo, rxbuf, len);

	} else if (ep == CDC1_TXD_EP) {
		if (UNLIKELY(cdc1_tx_state == 2)) {
			// ZLP was sent.
			cdc1_tx_state = 0;
			return;
		}

		if (UNLIKELY(FIFO_EMPTY(&cdc1_tx_fifo))) {
			// nothing more, notify host with ZLP
			if (cdc1_tx_state == 1) {
				cdc1_tx_state = 2;
				usbd_ep_write(dev, CDC1_TXD_EP, 0, 0);
			}
			return;
		}

		int rl = MIN(CDC1_DATA_SZ, fifo_get_read_count_cont(&cdc1_tx_fifo));

		rl = usbd_ep_write(dev, ep, fifo_get_read_addr(&cdc1_tx_fifo), rl);
		fifo_read_done_count(&cdc1_tx_fifo, rl);

		cdc1_tx_state = 1;
	}
}

static void cdc_rxtx2(usbd_device *dev, uint8_t event, uint8_t ep)
{
	(void)event;
	if (ep == CDC2_RXD_EP) {
		int len = usbd_ep_read(dev, ep, rxbuf, CDC2_DATA_SZ);

		if (len == -1) {
			return;
		}

		fifo_write_buf(&usb_rx2_fifo, rxbuf, len);

	} else if (ep == CDC2_TXD_EP) {
		if (UNLIKELY(cdc2_tx_state == 2)) {
			// ZLP was sent.
			cdc2_tx_state = 0;
			return;
		}

		if (UNLIKELY(FIFO_EMPTY(&cdc2_tx_fifo))) {
			// nothing more, notify host with ZLP
			if (cdc2_tx_state == 1) {
				cdc2_tx_state = 2;
				usbd_ep_write(dev, CDC2_TXD_EP, 0, 0);
			}
			return;
		}

		int rl = MIN(CDC2_DATA_SZ, fifo_get_read_count_cont(&cdc2_tx_fifo));

		rl = usbd_ep_write(dev, ep, fifo_get_read_addr(&cdc2_tx_fifo), rl);
		fifo_read_done_count(&cdc2_tx_fifo, rl);

		cdc2_tx_state = 1;
	}
}

static usbd_respond cdc_setconf (usbd_device *dev, uint8_t cfg) {
    switch (cfg) {
    case 0:
        /* deconfiguring device */
        usbd_ep_deconfig(dev, CDC1_NTF_EP);
        usbd_ep_deconfig(dev, CDC1_TXD_EP);
        usbd_ep_deconfig(dev, CDC1_RXD_EP);

        usbd_reg_endpoint(dev, CDC1_RXD_EP, 0);
        usbd_reg_endpoint(dev, CDC1_TXD_EP, 0);

        usbd_ep_deconfig(dev, CDC2_NTF_EP);
        usbd_ep_deconfig(dev, CDC2_TXD_EP);
        usbd_ep_deconfig(dev, CDC2_RXD_EP);

        usbd_reg_endpoint(dev, CDC2_RXD_EP, 0);
        usbd_reg_endpoint(dev, CDC2_TXD_EP, 0);
        return usbd_ack;
    case 1:
        /* configuring device */
        usbd_ep_config(dev, CDC1_RXD_EP, USB_EPTYPE_BULK /*| USB_EPTYPE_DBLBUF*/, CDC1_DATA_SZ);
        usbd_ep_config(dev, CDC1_TXD_EP, USB_EPTYPE_BULK /*| USB_EPTYPE_DBLBUF*/, CDC1_DATA_SZ);
        usbd_ep_config(dev, CDC1_NTF_EP, USB_EPTYPE_INTERRUPT, CDC1_NTF_SZ);

        usbd_reg_endpoint(dev, CDC1_RXD_EP, cdc_rxtx);
        usbd_reg_endpoint(dev, CDC1_TXD_EP, cdc_rxtx);

        usbd_ep_write(dev, CDC1_TXD_EP, 0, 0);

        usbd_ep_config(dev, CDC2_RXD_EP, USB_EPTYPE_BULK /*| USB_EPTYPE_DBLBUF*/, CDC2_DATA_SZ);
        usbd_ep_config(dev, CDC2_TXD_EP, USB_EPTYPE_BULK /*| USB_EPTYPE_DBLBUF*/, CDC2_DATA_SZ);
        usbd_ep_config(dev, CDC2_NTF_EP, USB_EPTYPE_INTERRUPT, CDC2_NTF_SZ);

        usbd_reg_endpoint(dev, CDC2_RXD_EP, cdc_rxtx2);
        usbd_reg_endpoint(dev, CDC2_TXD_EP, cdc_rxtx2);

        usbd_ep_write(dev, CDC2_TXD_EP, 0, 0);
        return usbd_ack;
    default:
        return usbd_fail;
    }
}

void usb_uart_cdc1_pkt(struct cmd_pkt_t *pkt)
{
//	uint32_t wl;

//	wl = MIN(len, fifo_get_write_count(&tx_fifo));

	fifo_write_buf(&cdc1_tx_fifo, (void *)&pkt->sof, 1);
	fifo_write_buf(&cdc1_tx_fifo, (void *)&pkt->cmd, 1);
	fifo_write_buf(&cdc1_tx_fifo, (void *)&pkt->len, 2);
	fifo_write_buf(&cdc1_tx_fifo, pkt->data, pkt->len);
	fifo_write_buf(&cdc1_tx_fifo, (void *)&pkt->crc, 4);

	NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);

	if (UNLIKELY(cdc1_tx_state == 0)) {
		cdc1_tx_state = 1;

		int rl = MIN(CDC1_DATA_SZ, fifo_get_read_count_cont(&cdc1_tx_fifo));

		rl = usbd_ep_write(&udev, CDC1_TXD_EP, fifo_get_read_addr(&cdc1_tx_fifo), rl);
		fifo_read_done_count(&cdc1_tx_fifo, rl);
	}

	NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
}


void usb_uart_cdc1_send(const char *buf, uint32_t len)
{
	uint32_t wl;

	wl = MIN(len, fifo_get_write_count(&cdc1_tx_fifo));

	fifo_write_buf(&cdc1_tx_fifo, buf, wl);

	NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);

	if (UNLIKELY(cdc1_tx_state == 0)) {
		cdc1_tx_state = 1;

		int rl = MIN(CDC1_DATA_SZ, fifo_get_read_count_cont(&cdc1_tx_fifo));

		rl = usbd_ep_write(&udev, CDC1_TXD_EP, fifo_get_read_addr(&cdc1_tx_fifo), rl);
		fifo_read_done_count(&cdc1_tx_fifo, rl);
	}

	NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
}

void usb_uart_cdc2_send(const char *buf, uint32_t len)
{
	uint32_t wl;

	wl = MIN(len, fifo_get_write_count(&cdc2_tx_fifo));

	fifo_write_buf(&cdc2_tx_fifo, buf, wl);

	NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);

	if (UNLIKELY(cdc2_tx_state == 0)) {
		cdc2_tx_state = 1;

		int rl = MIN(CDC2_DATA_SZ, fifo_get_read_count_cont(&cdc2_tx_fifo));

		rl = usbd_ep_write(&udev, CDC2_TXD_EP, fifo_get_read_addr(&cdc2_tx_fifo), rl);
		fifo_read_done_count(&cdc2_tx_fifo, rl);
	}

	NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
}

void usb_uart_cdc_init(void)
{
	usbd_init(&udev, &usbd_hw, CDC_EP0_SIZE, ubuf, sizeof(ubuf));
	usbd_reg_config(&udev, cdc_setconf);
	usbd_reg_control(&udev, cdc_control);
	usbd_reg_descr(&udev, cdc_getdesc);

	fifo_init(&cdc1_tx_fifo, fifo_tx1_buf, sizeof(uint8_t), sizeof(fifo_tx1_buf));
	fifo_init(&usb_rx1_fifo, fifo_rx1_buf, sizeof(uint8_t), sizeof(fifo_rx1_buf));

	fifo_init(&cdc2_tx_fifo, fifo_tx2_buf, sizeof(uint8_t), sizeof(fifo_tx2_buf));
	fifo_init(&usb_rx2_fifo, fifo_rx2_buf, sizeof(uint8_t), sizeof(fifo_rx2_buf));

	NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 1);
	NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);

	usbd_enable(&udev, true);
	usbd_connect(&udev, true);
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
    usbd_poll(&udev);
}

