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
 * @file    demos/IMX23/RT-IMX23-HELLO/net/telnetd.h
 * @brief   Minimal telnet server feeding an xshell session.
 *
 * @details Listens on TCP port 23 (lwIP sockets). Each accepted connection
 *          gets a BaseSequentialStream adapter over the socket, IAC
 *          negotiation (server echoes, suppress-go-ahead) and then runs the
 *          standard xshell thread on it with the caller-supplied command
 *          table. One session at a time; when the client disconnects the
 *          next one is accepted.
 *
 * @addtogroup NET_SERVICES
 * @{
 */

#ifndef TELNETD_H
#define TELNETD_H

#include "hal.h"
#include "shell.h"

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

#define TELNETD_PORT            23U

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  /* Start the telnet daemon thread (does not return). commands is the
     xshell extra-command table, same as for the serial shell. */
  void telnetdStart(const ShellCommand *commands);
#ifdef __cplusplus
}
#endif

#endif /* TELNETD_H */

/** @} */
