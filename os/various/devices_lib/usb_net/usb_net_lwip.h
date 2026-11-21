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
 * @file    devices_lib/usb_net/usb_net_lwip.h
 * @brief   lwIP netif glue over a USB RNDIS gadget.
 *
 * @details Hooks a usb_rndis_t instance into lwIP as a standard Ethernet
 *          netif: frames received on the RNDIS bulk OUT endpoint are fed
 *          to ethernet_input() via tcpip_inpkt(), frames to transmit go
 *          through netif->linkoutput into usb_rndis_send().
 *
 * @addtogroup USB_NET
 * @{
 */

#ifndef USB_NET_LWIP_H
#define USB_NET_LWIP_H

#include "hal.h"
#include "usb_rndis.h"

#include "lwip/netif.h"
#include "lwip/ip4_addr.h"

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

typedef struct {
  usb_rndis_t       *rndis;               /* RNDIS gadget                   */
  struct netif      netif;                /* lwIP netif                     */
  ip4_addr_t        ipaddr;
  ip4_addr_t        netmask;
  ip4_addr_t        gw;
  /* Flat staging buffer for chained TX pbufs. */
  uint8_t           tx_flat[USB_RNDIS_MAX_FRAME];
} usb_net_lwip_t;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void usb_net_lwip_init(usb_net_lwip_t *np, usb_rndis_t *rp, USBDriver *usbp,
                         const uint8_t mac[6],
                         const ip4_addr_t *ip, const ip4_addr_t *nm,
                         const ip4_addr_t *gw);
  struct netif *usb_net_lwip_netif(usb_net_lwip_t *np);
#ifdef __cplusplus
}
#endif

#endif /* USB_NET_LWIP_H */

/** @} */
