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
 * @file    devices_lib/usb_net/usb_rndis.h
 * @brief   USB RNDIS (USB Ethernet/RNDIS Gadget) device class on the
 *          ChibiOS USB driver.
 *
 * @details Implements a minimal CDC/RNDIS device: control interface with
 *          RNDIS_INIT/QUERY/SET handling on EP0, bulk IN/OUT on EP1 and the
 *          CDC notification interrupt IN on EP2. Received Ethernet frames are
 *          delivered to a caller-supplied callback (to feed a lwIP netif),
 *          frames to transmit are queued with usb_rndis_send().
 *
 *          Windows hosts bind this as "USB Ethernet/RNDIS Gadget" with no
 *          extra driver.
 *
 * @addtogroup USB_NET
 * @{
 */

#ifndef USB_RNDIS_H
#define USB_RNDIS_H

#include "hal.h"
#include "hal_usb.h"

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/* RNDIS message types. */
#define RNDIS_PACKET_MSG        0x00000001U
#define RNDIS_INIT_MSG          0x00000002U
#define RNDIS_INIT_CMPLT        0x80000002U
#define RNDIS_QUERY_MSG         0x00000003U
#define RNDIS_QUERY_CMPLT       0x80000003U
#define RNDIS_SET_MSG           0x00000004U
#define RNDIS_SET_CMPLT         0x80000004U
#define RNDIS_RESET_MSG         0x00000005U
#define RNDIS_RESET_CMPLT       0x80000005U
#define RNDIS_KEEPALIVE_MSG     0x00000006U
#define RNDIS_KEEPALIVE_CMPLT   0x80000006U

#define RNDIS_STATUS_SUCCESS    0x00000000U

/* RNDIS OIDs we answer to (minimal set). */
#define OID_GEN_SUPPORTED_LIST  0x00010101U
#define OID_GEN_HARDWARE_STATUS 0x00010102U
#define OID_GEN_MEDIA_CONNECT_STATUS 0x00010103U
#define OID_GEN_MEDIA_SUPPORTED 0x00010104U
#define OID_GEN_MAXIMUM_FRAME_SIZE 0x00010106U
#define OID_GEN_MAX_TOTAL_SIZE  0x0001010BU
#define OID_802_3_PERMANENT_ADDRESS 0x01010101U
#define OID_802_3_CURRENT_ADDRESS 0x01010102U
#define OID_GEN_XMIT_OK         0x0002010DU

#define RNDIS_MEDIA_CONNECTED   0x00000001U

/* CDC class requests. */
#define CDC_SEND_ENCAPSULATED_COMMAND 0x00U
#define CDC_GET_ENCAPSULATED_RESPONSE 0x01U

/* Max Ethernet frame we handle (RNDIS data offset is 44 bytes). */
#define USB_RNDIS_MAX_FRAME     1600U

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

typedef void (*usb_rndis_rx_cb_t)(const uint8_t *frame, uint16_t len,
                                  void *arg);

typedef struct {
  USBDriver                 *usbp;
  usb_rndis_rx_cb_t         rx_cb;
  void                      *rx_arg;
  bool                      initialized;    /* RNDIS_INIT received        */
  bool                      configured;     /* SET_CONFIGURATION done     */
  uint32_t                  rndis_state;
  uint8_t                   mac[6];
  /* RX path (host -> device). */
  uint8_t                   rx_buf[USB_RNDIS_MAX_FRAME + 64U];
  /* TX path (device -> host). */
  uint8_t                   tx_buf[USB_RNDIS_MAX_FRAME + 64U];
  bool                      tx_busy;
  uint16_t                  tx_len;
} usb_rndis_t;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void usb_rndis_init(usb_rndis_t *rp, USBDriver *usbp, const uint8_t mac[6],
                      usb_rndis_rx_cb_t rx_cb, void *rx_arg);
  void usb_rndis_start(usb_rndis_t *rp);
  /* Queue an Ethernet frame for transmission to the host (wraps in a
     RNDIS_PACKET_MSG). Returns false if the TX endpoint is busy. */
  bool usb_rndis_send(usb_rndis_t *rp, const uint8_t *frame, uint16_t len);
  bool usb_rndis_link_up(usb_rndis_t *rp);
#ifdef __cplusplus
}
#endif

#endif /* USB_RNDIS_H */

/** @} */
