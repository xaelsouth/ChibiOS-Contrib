/*
    ChibiOS - Copyright (C) 2026 Xael South

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
 * @file    efr32_clock_cmds.h
 * @brief   xshell commands for the SILABS clock tree.
 *
 * @details Commands (register them in the application's xshell command
 *          table, e.g. {"clocks", cmd_clocks}):
 *          - clocks        flat list of all clock points and their rates
 *          - clocks w      clock tree view (like Linux clk debugfs), with
 *                          run-time source selection and on/off state
 *          - clock reset|default   (only when HAL_LLD_USE_CLOCK_MANAGEMENT)
 *                          switch the clock configuration at run-time
 *
 *          This file is opt-in: add efr32_clock_cmds.c to CSRC and this
 *          directory to INCDIR in the application Makefile.
 *
 * @addtogroup SILABS_XSHELL_CLOCKS
 * @{
 */

#ifndef EFR32_CLOCK_CMDS_H
#define EFR32_CLOCK_CMDS_H

#include "xshell.h"

#ifdef __cplusplus
extern "C" {
#endif

  void cmd_clocks(xshell_t *xshp, int argc, char *argv[], char *envp[]);
#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) || defined(__DOXYGEN__)
  void cmd_clock(xshell_t *xshp, int argc, char *argv[], char *envp[]);
#endif

#ifdef __cplusplus
}
#endif

#endif /* EFR32_CLOCK_CMDS_H */

/** @} */
