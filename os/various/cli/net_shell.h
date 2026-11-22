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
 * @file    cli/net_shell.h
 * @brief   lwIP commands (ifconfig/dhcpc/ping/arp/ntp/slip), shell + xshell.
 *
 * @details Network diagnostic and configuration commands operating on the
 *          lwIP netif list. The net_shell_cmd_*() cores have the classic
 *          shell signature; register them with CLI_SHELL_ENTRY() or, when
 *          xshell is in the build, with CLI_XSHELL_ENTRY():
 *          @code
 *          static const ShellCommand cmds[] = {          // classic shell
 *            CLI_NET_SHELL_CMDS
 *            {NULL, NULL}
 *          };
 *          static const xshell_command_t xcmds[] = {     // xshell
 *            CLI_NET_XSHELL_CMDS
 *            {NULL, NULL}
 *          };
 *          @endcode
 *
 * @addtogroup NET_SERVICES
 * @{
 */

#ifndef NET_SHELL_H
#define NET_SHELL_H

#include "hal.h"
#include "cli.h"

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
#if CLI_HAS_XSHELL
  /* xshell adapters, for CLI_XSHELL_ENTRY(). */
  void cli_xshell_net_shell_cmd_ifconfig(xshell_t *xshp, int argc, char *argv[], char *envp[]);
  void cli_xshell_net_shell_cmd_dhcpc(xshell_t *xshp, int argc, char *argv[], char *envp[]);
  void cli_xshell_net_shell_cmd_ping(xshell_t *xshp, int argc, char *argv[], char *envp[]);
  void cli_xshell_net_shell_cmd_arp(xshell_t *xshp, int argc, char *argv[], char *envp[]);
  void cli_xshell_net_shell_cmd_ntp(xshell_t *xshp, int argc, char *argv[], char *envp[]);
  void cli_xshell_net_shell_cmd_slip(xshell_t *xshp, int argc, char *argv[], char *envp[]);
#endif
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Registration entries.                                                     */
/*===========================================================================*/

#define CLI_NET_SHELL_CMDS                                                  \
  CLI_SHELL_ENTRY(net_shell_cmd_ifconfig, "ifconfig")                       \
  CLI_SHELL_ENTRY(net_shell_cmd_dhcpc,    "dhcpc")                          \
  CLI_SHELL_ENTRY(net_shell_cmd_ping,     "ping")                           \
  CLI_SHELL_ENTRY(net_shell_cmd_arp,      "arp")                            \
  CLI_SHELL_ENTRY(net_shell_cmd_ntp,      "ntp")                            \
  CLI_SHELL_ENTRY(net_shell_cmd_slip,     "slip")

#define CLI_NET_XSHELL_CMDS                                                 \
  CLI_XSHELL_ENTRY(net_shell_cmd_ifconfig, "ifconfig")                      \
  CLI_XSHELL_ENTRY(net_shell_cmd_dhcpc,    "dhcpc")                         \
  CLI_XSHELL_ENTRY(net_shell_cmd_ping,     "ping")                          \
  CLI_XSHELL_ENTRY(net_shell_cmd_arp,      "arp")                           \
  CLI_XSHELL_ENTRY(net_shell_cmd_ntp,      "ntp")                           \
  CLI_XSHELL_ENTRY(net_shell_cmd_slip,     "slip")

/* Ready-made NULL-terminated tables, for merging into an application's
   existing command table with cli_shell_merge()/cli_xshell_merge(). */
#if CLI_HAS_SHELL
  extern const ShellCommand cli_shell_net_commands[];
#endif
#if CLI_HAS_XSHELL
  extern const xshell_command_t cli_xshell_net_commands[];
#endif

#endif /* NET_SHELL_H */

/** @} */
