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
 * @file    devices_lib/net_services/net_shell.h
 * @brief   lwIP shell commands (ifconfig/dhcpc/ping/arp/ntp/slip).
 *
 * @details Network diagnostic and configuration commands operating on the
 *          lwIP netif list. Register them in the application ShellCommand
 *          table, e.g.:
 *          @code
 *            { "ifconfig", net_shell_cmd_ifconfig },
 *            { "dhcpc",    net_shell_cmd_dhcpc },
 *            { "ping",     net_shell_cmd_ping },
 *            { "arp",      net_shell_cmd_arp },
 *            { "ntp",      net_shell_cmd_ntp },
 *            { "slip",     net_shell_cmd_slip },
 *          @endcode
 *
 * @addtogroup NET_SERVICES
 * @{
 */

#ifndef NET_SHELL_H
#define NET_SHELL_H

#include "hal.h"

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void net_shell_cmd_ifconfig(BaseSequentialStream *chp, int argc, char *argv[]);
  void net_shell_cmd_dhcpc(BaseSequentialStream *chp, int argc, char *argv[]);
  void net_shell_cmd_ping(BaseSequentialStream *chp, int argc, char *argv[]);
  void net_shell_cmd_arp(BaseSequentialStream *chp, int argc, char *argv[]);
  void net_shell_cmd_ntp(BaseSequentialStream *chp, int argc, char *argv[]);
  void net_shell_cmd_slip(BaseSequentialStream *chp, int argc, char *argv[]);
  /* Hook referenced by SNTP_SET_SYSTEM_TIME in lwipopts.h. */
  void lwip_sntp_time_hook(uint32_t sec);
#ifdef __cplusplus
}
#endif

#endif /* NET_SHELL_H */

/** @} */
