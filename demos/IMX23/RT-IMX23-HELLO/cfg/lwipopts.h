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
 * @file    lwipopts.h
 * @brief   lwIP options for the i.MX23 RNDIS demo.
 *
 * lwIP heap (MEM_LIBC_MALLOC) is routed to the TLSF constant-time
 * allocator via tlsf_bindings.
 */

#ifndef __LWIPOPT_H__
#define __LWIPOPT_H__

#include <string.h>
#include <stdlib.h>

#include "tlsf_bindings.h"

/*--------------------------------------------------------------------------*/
/* System / threading.                                                      */
/*--------------------------------------------------------------------------*/
#define NO_SYS                          0
#define SYS_LIGHTWEIGHT_PROT            1
#define LWIP_TCPIP_CORE_LOCKING         0
#define LWIP_TCPIP_CORE_LOCKING_INPUT   0
#define LWIP_TCPIP_TIMEOUT              1

#define TCPIP_THREAD_NAME               "tcpip"
/* TLS handshake (mbedTLS) runs on this thread: needs a large stack. */
#define TCPIP_THREAD_STACKSIZE          8192
#define TCPIP_THREAD_PRIO               (NORMALPRIO)
#define DEFAULT_THREAD_STACKSIZE        1024
#define DEFAULT_THREAD_PRIO             (NORMALPRIO)

#define DEFAULT_RAW_RECVMBOX_SIZE       4
#define DEFAULT_UDP_RECVMBOX_SIZE       8
#define DEFAULT_TCP_RECVMBOX_SIZE       16
#define DEFAULT_ACCEPTMBOX_SIZE         8

/*--------------------------------------------------------------------------*/
/* Memory: route the lwIP heap to TLSF.                                     */
/*--------------------------------------------------------------------------*/
#define MEM_LIBC_MALLOC                 1
#define mem_clib_malloc(s)              tlsfHeapAlloc(s)
#define mem_clib_free(p)                tlsfHeapFree(p)
#define mem_clib_calloc(c, s)           tlsfHeapCalloc((c), (s))
#define MEM_ALIGNMENT                   4
/* With MEM_LIBC_MALLOC the heap is TLSF; MEM_SIZE is still used by
   altcp_tls_mbedtls_mem.c as the max single mbedTLS allocation. */
#define MEM_SIZE                        (128 * 1024)
#define MEM_STATS                       0
#define MEMP_MEM_MALLOC                 0
#define MEM_USE_POOLS                   0
#define LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT  0

#define MEMCPY(dst, src, len)           memcpy((dst), (src), (len))
#define SMEMCPY(dst, src, len)          memcpy((dst), (src), (len))

/* Randomness for DNS TX ids / ports (stdlib rand, seeded by the app). */
#define LWIP_RAND()                     ((u32_t)rand())
/* PPP magic.c wants jiffies; ms ticks are fine. */
#define sys_jiffies()                   sys_now()

#define MEMP_NUM_PBUF                   16
#define MEMP_NUM_RAW_PCB                1       /* ping (raw ICMP echo)      */
#define MEMP_NUM_UDP_PCB                10      /* dhcp + dns + sntp + tftp  */
#define MEMP_NUM_TCP_PCB                8
#define MEMP_NUM_TCP_PCB_LISTEN         8
#define MEMP_NUM_TCP_SEG                32
#define MEMP_NUM_REASSDATA              4
#define MEMP_NUM_FRAG_PBUF              8
#define MEMP_NUM_ARP_QUEUE              10
#define MEMP_NUM_NETBUF                 2
#define MEMP_NUM_NETCONN                16
#define MEMP_NUM_TCPIP_MSG_API          16
#define MEMP_NUM_TCPIP_MSG_INPKT        16
/* Full internal formula: includes 2*LWIP_DHCP, LWIP_DNS, PPP_NUM_TIMEOUTS. */
#define MEMP_NUM_SYS_TIMEOUT            LWIP_NUM_SYS_TIMEOUT_INTERNAL

#define PBUF_POOL_SIZE                  16
#define PBUF_POOL_BUFSIZE               LWIP_MEM_ALIGN_SIZE(TCP_MSS + 40 + PBUF_LINK_HLEN)

/*--------------------------------------------------------------------------*/
/* L3/L4.                                                                   */
/*--------------------------------------------------------------------------*/
#define LWIP_IPV4                       1
#define LWIP_ARP                        1
#define ARP_TABLE_SIZE                  10
#define ARP_QUEUEING                    1
#define LWIP_ETHERNET                   1
#define LWIP_ICMP                       1
#define LWIP_RAW                        1       /* ping (raw ICMP echo)      */
#define LWIP_DHCP                       1       /* dhcpc shell command       */
#define LWIP_DHCP_GET_NTP_SRV           1
#define LWIP_DHCP_MAX_NTP_SERVERS       2
#define LWIP_AUTOIP                     0
#define LWIP_DNS                        1       /* ntp by name, ping by name */
#define LWIP_UDP                        1
#define LWIP_TCP                        1
#define LWIP_IGMP                       0

#define IP_REASSEMBLY                   1
#define IP_FRAG                         0
#define IP_DEFAULT_TTL                  255
#define IP_SOF_BROADCAST                1
#define IP_SOF_BROADCAST_RECV           1

#define TCP_MSS                         1460
#define TCP_WND                         (4 * TCP_MSS)
#define TCP_SND_BUF                     (4 * TCP_MSS)
#define TCP_SND_QUEUELEN                ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))
#define TCP_LISTEN_BACKLOG              1
#define LWIP_TCP_TIMESTAMPS             0
#define LWIP_TCP_KEEPALIVE              1

/*--------------------------------------------------------------------------*/
/* Sockets / netconn API (used by telnetd).                                 */
/*--------------------------------------------------------------------------*/
#define LWIP_NETCONN                    1
#define LWIP_SOCKET                     1
#define LWIP_COMPAT_SOCKETS             1
#define LWIP_SO_RCVTIMEO                1
#define LWIP_SO_SNDTIMEO                1
#define LWIP_SO_RCVBUF                  1
#define SO_REUSE                        1
#define LWIP_PROVIDE_ERRNO              1

/*--------------------------------------------------------------------------*/
/* netif.                                                                   */
/*--------------------------------------------------------------------------*/
#define LWIP_NETIF_HOSTNAME             1
#define LWIP_NETIF_API                  1
#define LWIP_HAVE_LOOPIF                0
#define LWIP_NETIF_STATUS_CALLBACK      1
#define LWIP_NETIF_LINK_CALLBACK        1

/*--------------------------------------------------------------------------*/
/* Apps.                                                                    */
/*--------------------------------------------------------------------------*/
/* altcp + TLS (our own HTTPS server, net_services/httpsd) via mbedtls. */
#define LWIP_ALTCP                      1
#define LWIP_ALTCP_TLS                  1
#define LWIP_ALTCP_TLS_MBEDTLS          1

/* SNTP client (apps/sntp, "ntp" shell command in net_services/net_shell). */
#define SNTP_SERVER_DNS                 1
#define SNTP_MAX_SERVERS                2
/* Called from the tcpip thread on each received NTP time (Unix seconds).
   (unsigned long: lwipopts.h is included before <stdint.h> everywhere.) */
void lwip_sntp_time_hook(unsigned long sec);
#define SNTP_SET_SYSTEM_TIME(sec)       lwip_sntp_time_hook(sec)

/*--------------------------------------------------------------------------*/
/* SLIP netif (netif/slipif.c; sio_* shim in net_services/sio_sio.c).       */
/*--------------------------------------------------------------------------*/
#define SLIP_USE_RX_THREAD              1       /* !NO_SYS default           */

/*--------------------------------------------------------------------------*/
/* PPP (netif/ppp, PPPoS over a serial line).                               */
/*--------------------------------------------------------------------------*/
#define PPP_SUPPORT                     1
#define PPPOS_SUPPORT                   1
#define PPPOE_SUPPORT                   0
#define PAP_SUPPORT                     1
/* CHAP/MSCHAP/MPPE need the bundled polarssl copies, which clash with the
   mbedTLS objects (md5.o/sha1.o); keep PAP-only auth. */
#define CHAP_SUPPORT                    0
#define MSCHAP_SUPPORT                  0
#define CBCP_SUPPORT                    0
#define CCP_SUPPORT                     0
#define VJ_SUPPORT                      1
#define MD5_SUPPORT                     1

/*--------------------------------------------------------------------------*/
/* Stats / debug.                                                           */
/*--------------------------------------------------------------------------*/
#define LWIP_STATS                      0
#define LWIP_DEBUG                      0

#endif /* __LWIPOPT_H__ */
