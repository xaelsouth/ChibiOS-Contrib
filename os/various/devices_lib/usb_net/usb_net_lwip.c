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
 * @file    devices_lib/usb_net/usb_net_lwip.c
 * @brief   lwIP netif glue over a USB RNDIS gadget.
 *
 * @addtogroup USB_NET
 * @{
 */

#include "hal.h"
#include "usb_net_lwip.h"

#include <string.h>

#include "lwip/tcpip.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "netif/ethernet.h"
#include "netif/etharp.h"
#include "lwip/prot/ethernet.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

#define LINK_POLL_MS            1000U

/* DHCP/llmnr hostname advertised for the RNDIS netif: defaults to the
   board name from board.h, override with -DUSB_NET_LWIP_HOSTNAME="..."
   (keep it a strict hostname, i.e. [A-Za-z0-9-], if your DHCP server
   validates it). */
#ifndef USB_NET_LWIP_HOSTNAME
#define USB_NET_LWIP_HOSTNAME   BOARD_NAME
#endif

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/* RNDIS bulk OUT delivered a frame (ISR context): wrap in a pbuf and hand
   it to the tcpip thread. */
static void rx_cb(const uint8_t *frame, uint16_t len, void *arg) {
  usb_net_lwip_t *np = (usb_net_lwip_t *)arg;
  struct pbuf *p;

  if (len < 14U) {
    return;
  }
  p = pbuf_alloc(PBUF_RAW, len, PBUF_RAM);
  if (p == NULL) {
    return;     /* drop, lwIP upper layers retransmit if needed */
  }
  pbuf_take(p, frame, len);
  if (tcpip_inpkt(p, &np->netif, ethernet_input) != ERR_OK) {
    pbuf_free(p);
  }
}

/* netif->linkoutput: send one Ethernet frame to the host. Runs in the
   tcpip thread context. */
static err_t low_output(struct netif *netif, struct pbuf *p) {
  usb_net_lwip_t *np = (usb_net_lwip_t *)netif->state;

  if (p->tot_len > sizeof(np->tx_flat)) {
    return ERR_BUF;
  }
  if (p->next == NULL) {
    return usb_rndis_send(np->rndis, p->payload, (uint16_t)p->tot_len)
           ? ERR_OK : ERR_WOULDBLOCK;
  }
  /* Chained pbuf: flatten into the staging buffer. */
  pbuf_copy_partial(p, np->tx_flat, p->tot_len, 0U);
  return usb_rndis_send(np->rndis, np->tx_flat, (uint16_t)p->tot_len)
         ? ERR_OK : ERR_WOULDBLOCK;
}

/* netif init callback, runs in the tcpip thread. */
static err_t netif_init_cb(struct netif *netif) {
  usb_net_lwip_t *np = (usb_net_lwip_t *)netif->state;

  netif->hwaddr_len = ETHARP_HWADDR_LEN;
  memcpy(netif->hwaddr, np->rndis->mac, ETHARP_HWADDR_LEN);
  netif->mtu        = 1500U;
  netif->flags      = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP |
                      NETIF_FLAG_ETHERNET | NETIF_FLAG_LINK_UP;
  netif->output     = etharp_output;
  netif->linkoutput = low_output;
  return ERR_OK;
}

/* Runs in the tcpip thread context at the end of tcpip_init(). */
static void tcpip_ready_cb(void *arg) {
  usb_net_lwip_t *np = (usb_net_lwip_t *)arg;

  netif_add(&np->netif, &np->ipaddr, &np->netmask, &np->gw,
            np, netif_init_cb, ethernet_input);
  netif_set_default(&np->netif);
  netif_set_up(&np->netif);
  netif_set_link_up(&np->netif);
  netif_set_hostname(&np->netif, USB_NET_LWIP_HOSTNAME);
}

/* Link state poller thread: reflects the RNDIS initialized/configured state
   into netif link up/down. */
static THD_WORKING_AREA(link_poll_wa, 256U);
static THD_FUNCTION(link_poll_thread, arg) {
  usb_net_lwip_t *np = (usb_net_lwip_t *)arg;

  (void)arg;
  chRegSetThreadName("rndis_link");
  while (true) {
    bool up = usb_rndis_link_up(np->rndis);
    if (up != (netif_is_link_up(&np->netif) != 0)) {
      if (up) {
        netif_set_link_up(&np->netif);
      }
      else {
        netif_set_link_down(&np->netif);
      }
    }
    chThdSleepMilliseconds(LINK_POLL_MS);
  }
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

void usb_net_lwip_init(usb_net_lwip_t *np, usb_rndis_t *rp, USBDriver *usbp,
                       const uint8_t mac[6],
                       const ip4_addr_t *ip, const ip4_addr_t *nm,
                       const ip4_addr_t *gw) {

  memset(np, 0, sizeof(*np));
  np->rndis = rp;
  ip4_addr_copy(np->ipaddr, *ip);
  ip4_addr_copy(np->netmask, *nm);
  ip4_addr_copy(np->gw, *gw);

  /* RNDIS gadget with our RX callback. */
  usb_rndis_init(rp, usbp, mac, rx_cb, np);

  /* Start lwIP (spawns the tcpip thread, netif is added there). */
  tcpip_init(tcpip_ready_cb, np);

  /* Bring up the USB device. */
  usb_rndis_start(rp);

  chThdCreateStatic(link_poll_wa, sizeof(link_poll_wa),
                    NORMALPRIO - 1U, link_poll_thread, np);
}

struct netif *usb_net_lwip_netif(usb_net_lwip_t *np) {

  return &np->netif;
}

/** @} */
