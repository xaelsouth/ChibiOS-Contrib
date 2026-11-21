/*
    ChibiOS - Copyright (C) 2026 Xael South.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    devices_lib/usb_net/usb_rndis.c
 * @brief   USB RNDIS device class code (minimal CDC/RNDIS gadget).
 *
 * @addtogroup USB_NET
 * @{
 */

#include "hal.h"
#include "usb_rndis.h"

#include <string.h>

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

#define USB_VID                 0x0525U     /* Netchip (RNDIS gadget)       */
#define USB_PID                 0xA4A1U

#define EP_BULK                 1U          /* bulk IN + OUT share EP1      */
#define EP_INTR                 2U          /* CDC notification IN          */

#define RNDIS_DATA_HDR_LEN      44U

/* CDC class-specific notifications on the interrupt endpoint. */
#define CDC_NOTIFICATION        0xA1U
#define RNDIS_RESPONSE_AVAILABLE 0x01U

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

static usb_rndis_t *rndis;      /* single instance (one USB device)       */

/* EP states. */
static USBInEndpointState  ep1_in_state, ep2_in_state;
static USBOutEndpointState ep1_out_state;

/* EP0 OUT scratch for class requests with an OUT data stage
   (CDC SEND_ENCAPSULATED_COMMAND carrying a RNDIS message). */
static uint8_t  ep0_out_buf[256U];
static uint32_t ep0_out_len;

/* RNDIS response scratch: filled when a RNDIS message arrives, returned to
   the host by the next GET_ENCAPSULATED_RESPONSE control read. */
static uint8_t  rndis_resp[256U];
static uint32_t rndis_resp_len;

/*===========================================================================*/
/* Descriptors.                                                              */
/*===========================================================================*/

static const uint8_t device_desc_data[] = {
  0x12, USB_DESCRIPTOR_DEVICE,
  0x00, 0x02,                     /* bcdUSB 2.0                       */
  0x02,                           /* bDeviceClass: CDC                */
  0x00, 0x00,                     /* subclass, protocol               */
  64U,                            /* bMaxPacketSize0                  */
  0x25, 0x05,                     /* idVendor 0x0525                  */
  0xA1, 0xA4,                     /* idProduct 0xA4A1                 */
  0x00, 0x01,                     /* bcdDevice                        */
  0x01, 0x02, 0x00,               /* iManufacturer, iProduct, iSerial */
  0x01                            /* bNumConfigurations               */
};

/* Configuration: CDC control iface (with functional headers + intr EP2)
   + CDC data iface (bulk EP1 IN + OUT). */
static const uint8_t config_desc_data[] = {
  /* Configuration descriptor. */
  0x09, USB_DESCRIPTOR_CONFIGURATION,
  67U, 0x00,                      /* wTotalLength                     */
  0x02,                           /* bNumInterfaces                   */
  0x01,                           /* bConfigurationValue              */
  0x00,                           /* iConfiguration                   */
  0xC0,                           /* bmAttributes: self powered       */
  0x32,                           /* bMaxPower = 100mA                */

  /* Interface 0: CDC control. */
  0x09, USB_DESCRIPTOR_INTERFACE,
  0x00, 0x00,                     /* bInterfaceNumber, alt            */
  0x01,                           /* bNumEndpoints (intr in)          */
  0x02, 0x02, 0xFF,               /* CDC, ACM, vendor (RNDIS)         */
  0x00,
  /* CDC header functional. */
  0x05, 0x24, 0x00, 0x10, 0x01,
  /* CDC call management. */
  0x05, 0x24, 0x01, 0x00, 0x01,
  /* CDC ACM (no capabilities). */
  0x04, 0x24, 0x02, 0x00,
  /* CDC union: master 0, slave 1. */
  0x05, 0x24, 0x06, 0x00, 0x01,
  /* Interrupt IN endpoint 2. */
  0x07, USB_DESCRIPTOR_ENDPOINT,
  0x82,                           /* EP2 IN                           */
  0x03,                           /* interrupt                        */
  0x08, 0x00,                     /* wMaxPacketSize 8                 */
  0x0AU,                          /* bInterval                        */

  /* Interface 1: CDC data. */
  0x09, USB_DESCRIPTOR_INTERFACE,
  0x01, 0x00,
  0x02,                           /* 2 endpoints                      */
  0x0AU, 0x00, 0x00,              /* CDC data                         */
  0x00,
  /* Bulk IN endpoint 1. */
  0x07, USB_DESCRIPTOR_ENDPOINT,
  0x81, 0x02, 0x00, 0x02, 0x00,   /* EP1 IN bulk 512                  */
  /* Bulk OUT endpoint 1. */
  0x07, USB_DESCRIPTOR_ENDPOINT,
  0x01, 0x02, 0x00, 0x02, 0x00    /* EP1 OUT bulk 512                 */
};

static const uint8_t str0_data[] = { 0x04, USB_DESCRIPTOR_STRING, 0x09, 0x04 };
static const uint8_t str1_data[] = {
  18U, USB_DESCRIPTOR_STRING,
  'C', 0, 'h', 0, 'i', 0, 'b', 0, 'i', 0, 'O', 0, 'S', 0
};
static const uint8_t str2_data[] = {
  22U, USB_DESCRIPTOR_STRING,
  'R', 0, 'N', 0, 'D', 0, 'I', 0, 'S', 0, ' ', 0, 'G', 0, 'a', 0, 'd', 0,
  'g', 0, 'e', 0, 't', 0
};

static const USBDescriptor device_desc = {
  sizeof(device_desc_data), device_desc_data
};
static const USBDescriptor config_desc = {
  sizeof(config_desc_data), config_desc_data
};
static const USBDescriptor str0 = { sizeof(str0_data), str0_data };
static const USBDescriptor str1 = { sizeof(str1_data), str1_data };
static const USBDescriptor str2 = { sizeof(str2_data), str2_data };

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

static uint32_t get_le32(const uint8_t *p) {

  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void put_le32(uint8_t *p, uint32_t v) {

  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

/* Build a RNDIS *_CMPLT response in rndis_resp, returns its length. */
static uint32_t rndis_response(uint32_t msg_type, uint32_t request_id,
                               uint32_t status, const uint8_t *data,
                               uint32_t data_len) {
  uint8_t *p = rndis_resp;
  uint32_t total = 44U + data_len;

  memset(p, 0, 44U);
  put_le32(p + 0, msg_type);
  put_le32(p + 4, total);
  put_le32(p + 8, request_id);
  put_le32(p + 12, status);
  if (msg_type == RNDIS_QUERY_CMPLT) {
    put_le32(p + 16, data_len);          /* BytesWritten                  */
    put_le32(p + 20, 44U);               /* InformationBufferOffset       */
    put_le32(p + 36, data_len);          /* InformationBufferLength       */
  }
  if (data_len != 0U) {
    memcpy(p + 44U, data, data_len);
  }
  return total;
}

static uint32_t rndis_query_oid(uint32_t oid, uint32_t request_id) {
  uint8_t buf[64U];
  uint32_t n = 0U;

  switch (oid) {
  case OID_GEN_SUPPORTED_LIST: {
    static const uint32_t oids[] = {
      OID_GEN_HARDWARE_STATUS, OID_GEN_MEDIA_CONNECT_STATUS,
      OID_GEN_MEDIA_SUPPORTED, OID_GEN_MAXIMUM_FRAME_SIZE,
      OID_802_3_PERMANENT_ADDRESS, OID_802_3_CURRENT_ADDRESS,
      OID_GEN_XMIT_OK,
    };
    for (unsigned i = 0U; i < sizeof(oids) / sizeof(oids[0]); i++) {
      put_le32(buf + n, oids[i]);
      n += 4U;
    }
    break;
  }
  case OID_GEN_HARDWARE_STATUS:         /* NdisHardwareStatusReady       */
    put_le32(buf, 0U); n = 4U; break;
  case OID_GEN_MEDIA_CONNECT_STATUS:
    put_le32(buf, RNDIS_MEDIA_CONNECTED); n = 4U; break;
  case OID_GEN_MEDIA_SUPPORTED:         /* NdisMedium802_3               */
    put_le32(buf, 1U); n = 4U; break;
  case OID_GEN_MAXIMUM_FRAME_SIZE:
    put_le32(buf, 1514U); n = 4U; break;
  case OID_GEN_MAX_TOTAL_SIZE:
    put_le32(buf, 1514U + 44U + 14U); n = 4U; break;
  case OID_802_3_PERMANENT_ADDRESS:
  case OID_802_3_CURRENT_ADDRESS:
    memcpy(buf, rndis->mac, 6U); n = 6U; break;
  default:
    put_le32(buf, 0U); n = 4U; break;
  }
  return rndis_response(RNDIS_QUERY_CMPLT, request_id, RNDIS_STATUS_SUCCESS,
                        buf, n);
}

/* Send a zero-length RESPONSE_AVAILABLE notification on the interrupt EP so
   hosts that poll it know a control response is waiting. */
static void send_notification(USBDriver *usbp) {
  static uint8_t notif[8U];

  notif[0] = CDC_NOTIFICATION;
  notif[1] = RNDIS_RESPONSE_AVAILABLE;
  notif[2] = 0x00; notif[3] = 0x00;    /* wValue  */
  notif[4] = 0x00; notif[5] = 0x00;    /* wIndex  */
  notif[6] = 0x00; notif[7] = 0x00;    /* wLength */
  usbStartTransmitI(usbp, EP_INTR, notif, sizeof(notif));
}

/* A RNDIS message arrived on EP0 via SEND_ENCAPSULATED_COMMAND: process it
   and stage the response for the following GET_ENCAPSULATED_RESPONSE. */
static void rndis_handle_msg(const uint8_t *msg, uint32_t len) {
  uint32_t type, req_id;

  if (len < 16U) {
    return;
  }
  type   = get_le32(msg + 0);
  req_id = get_le32(msg + 8);

  switch (type) {
  case RNDIS_INIT_MSG:
    rndis->initialized = true;
    rndis_resp_len = rndis_response(RNDIS_INIT_CMPLT, req_id,
                                    RNDIS_STATUS_SUCCESS, NULL, 0U);
    send_notification(rndis->usbp);
    break;
  case RNDIS_QUERY_MSG:
    if (len >= 16U) {
      rndis_resp_len = rndis_query_oid(get_le32(msg + 12), req_id);
    }
    send_notification(rndis->usbp);
    break;
  case RNDIS_SET_MSG:
    rndis_resp_len = rndis_response(RNDIS_SET_CMPLT, req_id,
                                    RNDIS_STATUS_SUCCESS, NULL, 0U);
    send_notification(rndis->usbp);
    break;
  case RNDIS_RESET_MSG:
    rndis->initialized = false;
    rndis_resp_len = rndis_response(RNDIS_RESET_CMPLT, req_id,
                                    RNDIS_STATUS_SUCCESS, NULL, 0U);
    send_notification(rndis->usbp);
    break;
  case RNDIS_KEEPALIVE_MSG:
    rndis->rndis_state = RNDIS_STATUS_SUCCESS;
    rndis_resp_len = rndis_response(RNDIS_KEEPALIVE_CMPLT, req_id,
                                    RNDIS_STATUS_SUCCESS, NULL, 0U);
    break;
  default:
    break;
  }
}

/* EP0 endcb for the SEND_ENCAPSULATED_COMMAND data stage. */
static void ep0_cmd_endcb(USBDriver *usbp) {

  (void)usbp;
  rndis_handle_msg(ep0_out_buf, ep0_out_len);
}

/* Class/vendor request hook (called for every setup packet, setup already
   read into usbp->setup by the core). */
static bool requests_hook(USBDriver *usbp) {
  uint8_t  bm  = usbp->setup[0];
  uint8_t  req = usbp->setup[1];
  uint16_t len = (uint16_t)(usbp->setup[6] | ((uint16_t)usbp->setup[7] << 8));

  if ((bm & USB_RTYPE_TYPE_MASK) != USB_RTYPE_TYPE_CLASS) {
    return false;   /* standard requests handled by the core */
  }

  if ((bm & USB_RTYPE_RECIPIENT_MASK) != USB_RTYPE_RECIPIENT_INTERFACE) {
    return false;
  }

  switch (req) {
  case CDC_SEND_ENCAPSULATED_COMMAND:
    /* OUT data stage: the core arms EP0 RX with ep0next/ep0n and calls
       ep0endcb when done. */
    ep0_out_len = len;
    if (ep0_out_len > sizeof(ep0_out_buf)) {
      ep0_out_len = sizeof(ep0_out_buf);
    }
    usbSetupTransfer(usbp, ep0_out_buf, ep0_out_len, ep0_cmd_endcb);
    return true;
  case CDC_GET_ENCAPSULATED_RESPONSE:
    /* IN data stage: return the staged RNDIS response. */
    usbSetupTransfer(usbp, rndis_resp, rndis_resp_len, NULL);
    return true;
  default:
    /* SET_LINE_CODING, SET_CONTROL_LINE_STATE, SEND_BREAK, ...: accept
       with a zero-length status. */
    usbSetupTransfer(usbp, NULL, 0U, NULL);
    return true;
  }
}

static const USBDescriptor *get_descriptor(USBDriver *usbp, uint8_t dtype,
                                            uint8_t dindex, uint16_t lang) {

  (void)usbp; (void)lang;
  switch (dtype) {
  case USB_DESCRIPTOR_DEVICE:
    return &device_desc;
  case USB_DESCRIPTOR_CONFIGURATION:
    return (dindex == 0U) ? &config_desc : NULL;
  case USB_DESCRIPTOR_STRING:
    switch (dindex) {
    case 0U: return &str0;
    case 1U: return &str1;
    case 2U: return &str2;
    default: return NULL;
    }
  default:
    return NULL;
  }
}

/* Bulk OUT callback: a RNDIS data packet from the host (ISR context). */
static void ep1_out_cb(USBDriver *usbp, usbep_t ep) {
  USBOutEndpointState *osp = usbp->epc[ep]->out_state;
  const uint8_t *msg = rndis->rx_buf;
  uint32_t data_off, data_len;

  if (osp->rxcnt >= 16U && get_le32(msg + 0) == RNDIS_PACKET_MSG) {
    data_off = get_le32(msg + 8);
    data_len = get_le32(msg + 12);
    if (data_off >= RNDIS_DATA_HDR_LEN &&
        data_off + data_len <= osp->rxcnt && data_len != 0U) {
      if (rndis->rx_cb != NULL) {
        rndis->rx_cb(msg + data_off, (uint16_t)data_len, rndis->rx_arg);
      }
    }
  }
  /* Re-arm the OUT endpoint. */
  usbStartReceiveI(usbp, ep, rndis->rx_buf, sizeof(rndis->rx_buf));
}

/* Bulk IN callback: frame delivered to the host. */
static void ep1_in_cb(USBDriver *usbp, usbep_t ep) {

  (void)usbp; (void)ep;
  rndis->tx_busy = false;
}

/* Interrupt IN callback: notification delivered. */
static void ep2_in_cb(USBDriver *usbp, usbep_t ep) {

  (void)usbp; (void)ep;
}

/* Bulk IN/OUT share EP1, notification is EP2 IN. */
static USBEndpointConfig rndis_ep1_cfg = {
  .ep_mode      = USB_EP_MODE_TYPE_BULK,
  .setup_cb     = NULL,
  .in_cb        = ep1_in_cb,
  .out_cb       = ep1_out_cb,
  .in_maxsize   = 512U,
  .out_maxsize  = 512U,
  .in_state     = &ep1_in_state,
  .out_state    = &ep1_out_state,
  .ep_buffers   = 1U,
  .setup_buf    = NULL
};

static USBEndpointConfig rndis_ep2_cfg = {
  .ep_mode      = USB_EP_MODE_TYPE_INTR,
  .setup_cb     = NULL,
  .in_cb        = ep2_in_cb,
  .out_cb       = NULL,
  .in_maxsize   = 8U,
  .out_maxsize  = 0U,
  .in_state     = &ep2_in_state,
  .out_state    = NULL,
  .ep_buffers   = 1U,
  .setup_buf    = NULL
};

static void event_cb(USBDriver *usbp, usbevent_t event) {

  switch (event) {
  case USB_EVENT_CONFIGURED:
    usbInitEndpointI(usbp, EP_BULK, &rndis_ep1_cfg);
    usbInitEndpointI(usbp, EP_INTR, &rndis_ep2_cfg);
    rndis->configured = true;
    /* Start listening on the bulk OUT endpoint. */
    usbStartReceiveI(usbp, EP_BULK, rndis->rx_buf, sizeof(rndis->rx_buf));
    break;
  case USB_EVENT_UNCONFIGURED:
  case USB_EVENT_RESET:
    rndis->configured  = false;
    rndis->initialized = false;
    rndis->tx_busy     = false;
    break;
  default:
    break;
  }
}

static const USBConfig rndis_usb_cfg = {
  .event_cb          = event_cb,
  .get_descriptor_cb = get_descriptor,
  .requests_hook_cb  = requests_hook,
  .sof_cb            = NULL,
};

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

void usb_rndis_init(usb_rndis_t *rp, USBDriver *usbp, const uint8_t mac[6],
                    usb_rndis_rx_cb_t rx_cb, void *rx_arg) {

  memset(rp, 0, sizeof(*rp));
  rp->usbp   = usbp;
  rp->rx_cb  = rx_cb;
  rp->rx_arg = rx_arg;
  memcpy(rp->mac, mac, 6U);
  rndis = rp;
}

void usb_rndis_start(usb_rndis_t *rp) {

  usbStart(rp->usbp, &rndis_usb_cfg);
  usbConnectBus(rp->usbp);
}

bool usb_rndis_send(usb_rndis_t *rp, const uint8_t *frame, uint16_t len) {
  uint8_t *p = rp->tx_buf;

  if (!rp->configured || len > USB_RNDIS_MAX_FRAME) {
    return false;
  }

  osalSysLock();
  if (rp->tx_busy) {
    osalSysUnlock();
    return false;
  }
  if (len < 60U) {   /* pad to minimum Ethernet frame size (no FCS) */
    memset(p + RNDIS_DATA_HDR_LEN + len, 0, 60U - len);
    len = 60U;
  }
  memset(p, 0, RNDIS_DATA_HDR_LEN);
  put_le32(p + 0, RNDIS_PACKET_MSG);
  put_le32(p + 4, RNDIS_DATA_HDR_LEN + len);
  put_le32(p + 8, RNDIS_DATA_HDR_LEN);      /* DataOffset                 */
  put_le32(p + 12, len);                     /* DataLength                 */
  memcpy(p + RNDIS_DATA_HDR_LEN, frame, len);

  rp->tx_busy = true;
  rp->tx_len  = (uint16_t)(RNDIS_DATA_HDR_LEN + len);
  usbStartTransmitI(rp->usbp, EP_BULK, rp->tx_buf, rp->tx_len);
  osalSysUnlock();
  return true;
}

bool usb_rndis_link_up(usb_rndis_t *rp) {

  return rp->configured && rp->initialized;
}

/** @} */
