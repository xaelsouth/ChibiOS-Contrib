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
 * @file    cli/net_shell.c
 * @brief   lwIP shell commands: ifconfig, dhcpc, ping, arp, ntp, slip.
 *
 * @addtogroup NET_SERVICES
 * @{
 */

#include "hal.h"
#include "chprintf.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "net_shell.h"
#include "sio_sio.h"

#include "lwip/tcpip.h"
#include "lwip/sys.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/ip4.h"
#include "lwip/raw.h"
#include "lwip/pbuf.h"
#include "lwip/inet_chksum.h"
#include "lwip/prot/ip.h"
#include "lwip/prot/icmp.h"
#include "lwip/prot/ethernet.h"
#include "lwip/ip4_addr.h"
#include "lwip/api.h"
#include "lwip/apps/sntp.h"
#include "netif/slipif.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

#define PING_DEFAULT_COUNT      4U
#define PING_DEFAULT_SIZE       32U
#define PING_DEFAULT_INTERVAL   1000U   /* ms */
#define PING_DEFAULT_TIMEOUT    2000U   /* ms */

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/* SNTP last-received time hook state. */
static volatile uint32_t ntp_last_sec;
static volatile bool     ntp_got;
static bool              sntp_started;

/* Single SLIP interface instance (shell glue, started on demand). */
static struct netif      slip_netif;
static bool              slip_active;

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/* Find a netif by lwIP name ("en0") or by index into netif_list. */
static struct netif *find_netif(BaseSequentialStream *chp, const char *arg) {
  struct netif *n;

  if (arg == NULL) {
    return netif_default;
  }
  n = netif_find((char *)arg);
  if (n != NULL) {
    return n;
  }
  if ((arg[0] >= '0') && (arg[0] <= '9')) {
    unsigned idx = (unsigned)strtoul(arg, NULL, 10);
    for (n = netif_list; n != NULL; n = n->next) {
      if (idx-- == 0U) {
        return n;
      }
    }
  }
  if (chp != NULL) {
    chprintf(chp, "interface '%s' not found\r\n", arg);
  }
  return NULL;
}

static void print_netif(BaseSequentialStream *chp, struct netif *n) {
  sys_prot_t lev;
  char flags[64];
  unsigned f = 0U;
  char name0, name1;
  int num;
  unsigned mtu;
  u8_t hwaddr[6];
  u8_t hwaddr_len;
  ip4_addr_t ip, nm, gw;
#if LWIP_NETIF_HOSTNAME
  const char *hostname;
#endif
  bool dhcp_active;

  /* Snapshot under the lwIP protect (chprintf may block, so print after). */
  flags[0] = '\0';
  SYS_ARCH_PROTECT(lev);
  name0 = (char)n->name[0];
  name1 = (char)n->name[1];
  num   = (int)n->num;
  mtu   = (unsigned)n->mtu;
  hwaddr_len = n->hwaddr_len;
  if (hwaddr_len != 0U) {
    memcpy(hwaddr, n->hwaddr, 6U);
  }
  ip = n->ip_addr;
  nm = n->netmask;
  gw = n->gw;
#if LWIP_NETIF_HOSTNAME
  hostname = n->hostname;
#endif
#if LWIP_DHCP
  dhcp_active = (netif_dhcp_data(n) != NULL);
#else
  dhcp_active = false;
#endif
  if (n->flags & NETIF_FLAG_UP)          { strcat(flags, "UP "); f = 1U; }
  if (n->flags & NETIF_FLAG_LINK_UP)     { strcat(flags, "LINK "); f = 1U; }
  if (n->flags & NETIF_FLAG_BROADCAST)   { strcat(flags, "BROADCAST "); f = 1U; }
  if (n->flags & NETIF_FLAG_ETHARP)      { strcat(flags, "ETHARP "); f = 1U; }
  if (n->flags & NETIF_FLAG_ETHERNET)    { strcat(flags, "ETHERNET "); f = 1U; }
  if (dhcp_active)                       { strcat(flags, "DHCP "); f = 1U; }
  SYS_ARCH_UNPROTECT(lev);

  if (f == 0U) {
    strcat(flags, "-");
  }
  chprintf(chp, "%c%c%d: flags=[%s] mtu=%u\r\n",
           name0, name1, num, flags, mtu);
  if (hwaddr_len != 0U) {
    chprintf(chp, "  hwaddr %02x:%02x:%02x:%02x:%02x:%02x\r\n",
             hwaddr[0], hwaddr[1], hwaddr[2],
             hwaddr[3], hwaddr[4], hwaddr[5]);
  }
  chprintf(chp, "  inet %s  netmask %s  gw %s\r\n",
           ip4addr_ntoa(&ip), ip4addr_ntoa(&nm), ip4addr_ntoa(&gw));
#if LWIP_NETIF_HOSTNAME
  if (hostname != NULL) {
    chprintf(chp, "  hostname %s\r\n", hostname);
  }
#endif
#if LWIP_DHCP
  if (dhcp_active) {
    chprintf(chp, "  dhcp client running\r\n");
  }
#endif
}

/*--------------------------------------------------------------------------*/
/* ifconfig                                                                 */
/*--------------------------------------------------------------------------*/

void net_shell_cmd_ifconfig(BaseSequentialStream *chp, int argc, char *argv[]) {
  struct netif *n;

  if (argc == 0) {
    for (n = netif_list; n != NULL; n = n->next) {
      print_netif(chp, n);
    }
    return;
  }

  n = find_netif(chp, argv[0]);
  if (n == NULL) {
    return;
  }
  if (argc == 1) {
    print_netif(chp, n);
    return;
  }

  if (strcmp(argv[1], "up") == 0) {
    netifapi_netif_set_up(n);
  }
  else if (strcmp(argv[1], "down") == 0) {
    netifapi_netif_set_down(n);
  }
  else if (strcmp(argv[1], "linkup") == 0) {
    netifapi_netif_set_link_up(n);
  }
  else if (strcmp(argv[1], "linkdown") == 0) {
    netifapi_netif_set_link_down(n);
  }
#if LWIP_DHCP
  else if (strcmp(argv[1], "dhcp") == 0) {
    netifapi_dhcp_start(n);
    chprintf(chp, "dhcp start on %c%c%d\r\n", n->name[0], n->name[1], (int)n->num);
  }
#endif
#if LWIP_NETIF_HOSTNAME
  else if ((strcmp(argv[1], "hostname") == 0) && (argc >= 3)) {
    netif_set_hostname(n, argv[2]);
  }
#endif
  else if (argc >= 4) {
    ip4_addr_t ip, nm, gw;
    if ((ip4addr_aton(argv[1], &ip) == 0) ||
        (ip4addr_aton(argv[2], &nm) == 0) ||
        (ip4addr_aton(argv[3], &gw) == 0)) {
      chprintf(chp, "usage: ifconfig <if> <ip> <netmask> <gw>\r\n");
      return;
    }
    netifapi_netif_set_addr(n, &ip, &nm, &gw);
    chprintf(chp, "%c%c%d set to %s/%s gw %s\r\n",
             n->name[0], n->name[1], (int)n->num,
             ip4addr_ntoa(&ip), ip4addr_ntoa(&nm), ip4addr_ntoa(&gw));
  }
  else {
    chprintf(chp, "usage: ifconfig [<if> [up|down|linkup|linkdown|dhcp|"
                  "hostname <name>|<ip> <netmask> <gw>]]\r\n");
  }
}

/*--------------------------------------------------------------------------*/
/* dhcpc                                                                    */
/*--------------------------------------------------------------------------*/

void net_shell_cmd_dhcpc(BaseSequentialStream *chp, int argc, char *argv[]) {
  struct netif *n;

  if (argc < 2) {
    chprintf(chp, "usage: dhcpc <if> start|stop|renew|release|status\r\n");
    return;
  }
  n = find_netif(chp, argv[0]);
  if (n == NULL) {
    return;
  }
#if LWIP_DHCP
  if (strcmp(argv[1], "start") == 0) {
    netifapi_dhcp_start(n);
    chprintf(chp, "dhcp start\r\n");
  }
  else if (strcmp(argv[1], "stop") == 0) {
    netifapi_dhcp_release_and_stop(n);
    chprintf(chp, "dhcp released and stopped\r\n");
  }
  else if (strcmp(argv[1], "renew") == 0) {
    netifapi_dhcp_renew(n);
    chprintf(chp, "dhcp renew\r\n");
  }
  else if (strcmp(argv[1], "release") == 0) {
    netifapi_dhcp_release(n);
    chprintf(chp, "dhcp release\r\n");
  }
  else if (strcmp(argv[1], "status") == 0) {
    sys_prot_t lev;
    bool active;
    ip4_addr_t ip;
    char name0, name1;
    int num;

    SYS_ARCH_PROTECT(lev);
    active = (netif_dhcp_data(n) != NULL);
    ip = n->ip_addr;
    name0 = (char)n->name[0];
    name1 = (char)n->name[1];
    num   = (int)n->num;
    SYS_ARCH_UNPROTECT(lev);
    chprintf(chp, "%c%c%d: dhcp %s, inet %s\r\n",
             name0, name1, num, active ? "active" : "inactive",
             ip4addr_ntoa(&ip));
  }
#else
  chprintf(chp, "LWIP_DHCP is disabled\r\n");
#endif
}

/*--------------------------------------------------------------------------*/
/* ping (raw ICMP echo)                                                     */
/*--------------------------------------------------------------------------*/

typedef struct {
  semaphore_t sem;
  u16_t       id;
  u16_t       seq;
  u32_t       rtt;
  bool        replied;
} ping_ctx_t;

static u8_t ping_recv_cb(void *arg, struct raw_pcb *pcb, struct pbuf *p,
                         const ip_addr_t *addr) {
  ping_ctx_t *ctx = (ping_ctx_t *)arg;

  (void)pcb; (void)addr;
  if ((p->len >= (8U + 4U)) && (p->payload != NULL)) {
    struct icmp_echo_hdr *eh = (struct icmp_echo_hdr *)p->payload;
    if ((eh->type == ICMP_ER) && (eh->id == ctx->id)) {
      u32_t ts;
      memcpy(&ts, (const u8_t *)p->payload + 8U, 4U);
      ctx->seq     = eh->seqno;
      ctx->rtt     = sys_now() - ts;
      ctx->replied = true;
      chSemSignal(&ctx->sem);
    }
  }
  pbuf_free(p);
  return 1U;
}

void net_shell_cmd_ping(BaseSequentialStream *chp, int argc, char *argv[]) {
  ip_addr_t dst;
  struct raw_pcb *pcb;
  ping_ctx_t ctx;
  sys_prot_t lev;
  unsigned count = PING_DEFAULT_COUNT;
  unsigned size  = PING_DEFAULT_SIZE;
  unsigned interval = PING_DEFAULT_INTERVAL;
  unsigned timeout  = PING_DEFAULT_TIMEOUT;
  unsigned i, sent = 0U, recv = 0U;
  u16_t seq;
  err_t err;

  if (argc < 1) {
    chprintf(chp, "usage: ping <host> [-c count] [-s size] [-i ms] [-t ms]\r\n");
    return;
  }

  /* Resolve host: numeric IP first, then DNS. */
  if (ipaddr_aton(argv[0], &dst) == 0) {
#if LWIP_DNS && LWIP_NETCONN
    err = netconn_gethostbyname(argv[0], &dst);
    if (err != ERR_OK) {
      chprintf(chp, "ping: cannot resolve %s (err %d)\r\n", argv[0], (int)err);
      return;
    }
#else
    chprintf(chp, "ping: '%s' is not an IP and DNS is disabled\r\n", argv[0]);
    return;
#endif
  }

  for (i = 1U; i < (unsigned)argc; i++) {
    if ((strcmp(argv[i], "-c") == 0) && (i + 1U < (unsigned)argc)) {
      count = (unsigned)strtoul(argv[++i], NULL, 10);
    }
    else if ((strcmp(argv[i], "-s") == 0) && (i + 1U < (unsigned)argc)) {
      size = (unsigned)strtoul(argv[++i], NULL, 10);
    }
    else if ((strcmp(argv[i], "-i") == 0) && (i + 1U < (unsigned)argc)) {
      interval = (unsigned)strtoul(argv[++i], NULL, 10);
    }
    else if ((strcmp(argv[i], "-t") == 0) && (i + 1U < (unsigned)argc)) {
      timeout = (unsigned)strtoul(argv[++i], NULL, 10);
    }
  }
  if (size > 1024U) {
    size = 1024U;
  }

  chSemObjectInit(&ctx.sem, 0);
  ctx.id      = htons((u16_t)(sys_now() & 0xFFFFU));
  ctx.replied = false;

  SYS_ARCH_PROTECT(lev);
  pcb = raw_new(IP_PROTO_ICMP);
  if (pcb != NULL) {
    raw_recv(pcb, ping_recv_cb, &ctx);
    raw_bind(pcb, IP_ADDR_ANY);
    if (netif_default != NULL) {
      raw_bind_netif(pcb, netif_default);
    }
  }
  SYS_ARCH_UNPROTECT(lev);

  if (pcb == NULL) {
    chprintf(chp, "ping: raw_new failed\r\n");
    return;
  }

  chprintf(chp, "PING %s (%s): %u data bytes\r\n", argv[0],
           ipaddr_ntoa(&dst), size);

  for (seq = 0U; seq < (u16_t)count; seq++) {
    struct pbuf *p;
    struct icmp_echo_hdr *eh;
    u16_t len = (u16_t)(8U + size);
    u8_t *pl;

    p = pbuf_alloc(PBUF_RAW, len, PBUF_RAM);
    if (p == NULL) {
      chprintf(chp, "ping: pbuf alloc failed\r\n");
      break;
    }
    eh = (struct icmp_echo_hdr *)p->payload;
    eh->type   = ICMP_ECHO;
    eh->code   = 0U;
    eh->chksum = 0U;
    eh->id     = ctx.id;
    eh->seqno  = htons(seq);
    /* Payload: timestamp (for RTT) + pattern. */
    pl = (u8_t *)p->payload + 8U;
    {
      u32_t ts = sys_now();
      memcpy(pl, &ts, 4U);
    }
    for (i = 4U; i < size; i++) {
      pl[i] = (u8_t)(i & 0xFFU);
    }
    eh->chksum = inet_chksum(eh, len);

    SYS_ARCH_PROTECT(lev);
    err = raw_sendto(pcb, p, &dst);
    SYS_ARCH_UNPROTECT(lev);
    pbuf_free(p);
    if (err != ERR_OK) {
      chprintf(chp, "ping: send failed err %d\r\n", (int)err);
      break;
    }
    sent++;

    ctx.replied = false;
    if ((chSemWaitTimeout(&ctx.sem, chTimeMS2I(timeout)) == MSG_OK) &&
        ctx.replied) {
      recv++;
      chprintf(chp, "%u bytes from %s: icmp_seq=%u time=%lu ms\r\n",
               len, ipaddr_ntoa(&dst), (unsigned)ntohs(ctx.seq),
               (unsigned long)ctx.rtt);
    }
    else {
      chprintf(chp, "no reply from %s: icmp_seq=%u (timeout)\r\n",
               ipaddr_ntoa(&dst), (unsigned)seq);
    }
    if (seq + 1U < (u16_t)count) {
      chThdSleepMilliseconds(interval);
    }
  }

  chprintf(chp, "--- %s ping statistics ---\r\n", argv[0]);
  chprintf(chp, "%u packets transmitted, %u received, %u%% loss\r\n",
           sent, recv, (sent != 0U) ? (100U * (sent - recv)) / sent : 0U);

  SYS_ARCH_PROTECT(lev);
  raw_remove(pcb);
  SYS_ARCH_UNPROTECT(lev);
}

/*--------------------------------------------------------------------------*/
/* arp                                                                      */
/*--------------------------------------------------------------------------*/

void net_shell_cmd_arp(BaseSequentialStream *chp, int argc, char *argv[]) {
  sys_prot_t lev;

  if ((argc == 0) || ((argc == 1) && (strcmp(argv[0], "-a") == 0))) {
    /* Dump all stable ARP table entries. */
    size_t i;
    unsigned shown = 0U;

    chprintf(chp, "ARP table:\r\n");
    for (i = 0U; i < (size_t)ARP_TABLE_SIZE; i++) {
      ip4_addr_t *ip;
      struct netif *n;
      struct eth_addr *eth;
      int ok;
      /* Local copies: the table may change once unprotected. */
      ip4_addr_t ip_c;
      struct eth_addr eth_c;
      char name0, name1;
      int num;

      SYS_ARCH_PROTECT(lev);
      ok = etharp_get_entry(i, &ip, &n, &eth);
      if (ok != 0) {
        ip_c  = *ip;
        eth_c = *eth;
        name0 = (char)n->name[0];
        name1 = (char)n->name[1];
        num   = (int)n->num;
      }
      SYS_ARCH_UNPROTECT(lev);
      if (ok != 0) {
        chprintf(chp, "  %s at %02x:%02x:%02x:%02x:%02x:%02x on %c%c%d\r\n",
                 ip4addr_ntoa(&ip_c),
                 eth_c.addr[0], eth_c.addr[1], eth_c.addr[2],
                 eth_c.addr[3], eth_c.addr[4], eth_c.addr[5],
                 name0, name1, num);
        shown++;
      }
    }
    if (shown == 0U) {
      chprintf(chp, "  (empty)\r\n");
    }
    return;
  }

  /* Look up a single address on all netifs. */
  {
    ip4_addr_t ip;
    struct netif *n;

    if (ip4addr_aton(argv[0], &ip) == 0) {
      chprintf(chp, "arp: '%s' is not an IP address\r\n", argv[0]);
      return;
    }
    for (n = netif_list; n != NULL; n = n->next) {
      struct eth_addr *eth;
      const ip4_addr_t *eip;
      ssize_t idx;
      struct eth_addr eth_c;
      char name0, name1;
      int num;

      SYS_ARCH_PROTECT(lev);
      idx = etharp_find_addr(n, &ip, &eth, &eip);
      if (idx >= 0) {
        eth_c = *eth;
        name0 = (char)n->name[0];
        name1 = (char)n->name[1];
        num   = (int)n->num;
      }
      SYS_ARCH_UNPROTECT(lev);
      if (idx >= 0) {
        chprintf(chp, "%s at %02x:%02x:%02x:%02x:%02x:%02x on %c%c%d\r\n",
                 ip4addr_ntoa(&ip),
                 eth_c.addr[0], eth_c.addr[1], eth_c.addr[2],
                 eth_c.addr[3], eth_c.addr[4], eth_c.addr[5],
                 name0, name1, num);
        return;
      }
    }
    chprintf(chp, "%s not in ARP table\r\n", ip4addr_ntoa(&ip));
  }
}

/*--------------------------------------------------------------------------*/
/* ntp (SNTP client)                                                        */
/*--------------------------------------------------------------------------*/

void lwip_sntp_time_hook(uint32_t sec) {

  ntp_last_sec = sec;
  ntp_got      = true;
}

void net_shell_cmd_ntp(BaseSequentialStream *chp, int argc, char *argv[]) {

  if (argc < 1) {
    chprintf(chp, "usage: ntp <server> | ntp status | ntp stop\r\n");
    return;
  }

  if (strcmp(argv[0], "stop") == 0) {
    sys_prot_t lev;
    SYS_ARCH_PROTECT(lev);
    sntp_stop();
    SYS_ARCH_UNPROTECT(lev);
    sntp_started = false;
    chprintf(chp, "sntp stopped\r\n");
    return;
  }

  if (strcmp(argv[0], "status") == 0) {
    sys_prot_t lev;
    u8_t en, reach;
    SYS_ARCH_PROTECT(lev);
    en    = sntp_enabled();
    reach = sntp_getreachability(0U);
    SYS_ARCH_UNPROTECT(lev);
    chprintf(chp, "sntp %s, reachability 0x%02x\r\n",
             en ? "running" : "stopped", (unsigned)reach);
    if (ntp_got) {
      time_t t = (time_t)ntp_last_sec;
      struct tm *tm = gmtime(&t);
      if (tm != NULL) {
        chprintf(chp, "last sync: %04d-%02d-%02d %02d:%02d:%02d UTC\r\n",
                 tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                 tm->tm_hour, tm->tm_min, tm->tm_sec);
      }
      else {
        chprintf(chp, "last sync: %lu (unix)\r\n", (unsigned long)ntp_last_sec);
      }
    }
    else {
      chprintf(chp, "no sync yet\r\n");
    }
    return;
  }

  /* ntp <server>: configure server 0 and (re)start polling. */
  {
    sys_prot_t lev;
    SYS_ARCH_PROTECT(lev);
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    if (!sntp_started) {
      sntp_init();
      sntp_started = true;
    }
#if SNTP_SERVER_DNS
    {
      ip4_addr_t a4;
      if (ip4addr_aton(argv[0], &a4) != 0) {
        ip_addr_t a;
        ip_addr_set_ip4_u32(&a, ip4_addr_get_u32(&a4));
        sntp_setserver(0U, &a);
      }
      else {
        sntp_setservername(0U, argv[0]);
      }
    }
#else
    {
      ip_addr_t a;
      if (ipaddr_aton(argv[0], &a) == 0) {
        SYS_ARCH_UNPROTECT(lev);
        chprintf(chp, "ntp: '%s' is not an IP and SNTP_SERVER_DNS is off\r\n",
                 argv[0]);
        return;
      }
      sntp_setserver(0U, &a);
    }
#endif
    SYS_ARCH_UNPROTECT(lev);
  }
  chprintf(chp, "ntp server 0 set to '%s', polling...\r\n", argv[0]);
}

/*--------------------------------------------------------------------------*/
/* slip                                                                     */
/*--------------------------------------------------------------------------*/

void net_shell_cmd_slip(BaseSequentialStream *chp, int argc, char *argv[]) {

  if ((argc >= 1) && (strcmp(argv[0], "stop") == 0)) {
    if (slip_active) {
      /* netifapi posts to the tcpip thread: must NOT hold the protect. */
      netifapi_netif_remove(&slip_netif);
      slip_active = false;
      chprintf(chp, "slip interface removed\r\n");
    }
    else {
      chprintf(chp, "slip not active\r\n");
    }
    return;
  }

  if (argc < 1) {
    chprintf(chp, "usage: slip <sio#> [baud] [ip netmask gw] | slip stop\r\n");
    return;
  }
  if (slip_active) {
    chprintf(chp, "slip already active (slip stop first)\r\n");
    return;
  }

  {
    u8_t devnum = (u8_t)strtoul(argv[0], NULL, 10);
    unsigned baud = (argc >= 2) ? (unsigned)strtoul(argv[1], NULL, 10) : 115200U;
    ip4_addr_t ip, nm, gw;
    SIODriver *siop;
    SIOConfig cfg;
    err_t err;

    siop = sio_sio_lookup(devnum);
    if (siop == NULL) {
      chprintf(chp, "sio device %u not registered (sio_sio_register)\r\n",
               (unsigned)devnum);
      return;
    }

    if (argc >= 5) {
      if ((ip4addr_aton(argv[2], &ip) == 0) ||
          (ip4addr_aton(argv[3], &nm) == 0) ||
          (ip4addr_aton(argv[4], &gw) == 0)) {
        chprintf(chp, "usage: slip <sio#> [baud] [ip netmask gw]\r\n");
        return;
      }
    }
    else {
      ip4addr_aton("10.0.0.2", &ip);
      ip4addr_aton("255.255.255.0", &nm);
      ip4addr_aton("10.0.0.1", &gw);
    }

    memset(&cfg, 0, sizeof(cfg));
    cfg.baud = baud;    /* framecfg left 0: the LLD applies its default
                           frame format (8N1+FIFO on i.MX23) */
    sioStart(siop, &cfg);

    memset(&slip_netif, 0, sizeof(slip_netif));
    /* slipif_init() reads the sio device number from netif->state.
       netifapi posts to the tcpip thread and waits: no protect held. */
    err = netifapi_netif_add(&slip_netif, &ip, &nm, &gw,
                             (void *)(uintptr_t)devnum,
                             slipif_init, ip4_input);
    if (err == ERR_OK) {
      netifapi_netif_set_up(&slip_netif);
    }

    if (err != ERR_OK) {
      chprintf(chp, "slip: netif add failed err %d\r\n", (int)err);
      return;
    }
    slip_active = true;
    chprintf(chp, "slip up on sio%u @ %u baud, inet %s\r\n",
             (unsigned)devnum, baud, ip4addr_ntoa(&ip));
  }
}

/*===========================================================================*/
/* xshell adapters (only when xshell is part of the build).                  */
/*===========================================================================*/

#if CLI_HAS_XSHELL
CLI_XSHELL_ADAPTER(net_shell_cmd_ifconfig)
CLI_XSHELL_ADAPTER(net_shell_cmd_dhcpc)
CLI_XSHELL_ADAPTER(net_shell_cmd_ping)
CLI_XSHELL_ADAPTER(net_shell_cmd_arp)
CLI_XSHELL_ADAPTER(net_shell_cmd_ntp)
CLI_XSHELL_ADAPTER(net_shell_cmd_slip)
#endif

/*===========================================================================*/
/* Ready-made command tables (for cli_shell_merge()/cli_xshell_merge()).     */
/*===========================================================================*/

#if CLI_HAS_SHELL
const ShellCommand cli_shell_net_commands[] = {
  CLI_NET_SHELL_CMDS
  {NULL, NULL}
};
#endif

#if CLI_HAS_XSHELL
const xshell_command_t cli_xshell_net_commands[] = {
  CLI_NET_XSHELL_CMDS
  {NULL, NULL}
};
#endif

/** @} */
