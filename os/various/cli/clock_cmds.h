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
 * @file    clock_cmds.h
 * @brief   Generic xshell clock commands.
 *
 * @details Commands (register them in the application's xshell command
 *          table, e.g. {"clocks", cmd_clocks}):
 *          - clocks        flat list of all clock points and their rates
 *          - clocks w      clock tree view, if the port provides one via
 *                          the cli_clock_tree_print() hook (e.g. EFR32FG23
 *                          prints a Linux clk debugfs style tree)
 *          - clock reset|default   (only when HAL_LLD_USE_CLOCK_MANAGEMENT)
 *                          switch the clock configuration at run-time
 *
 *          Works for any port that defines CLK_ARRAY_SIZE and
 *          CLK_POINT_NAMES in its hal_lld.h (SILABS EFR32 ports, STM32
 *          xhal ports, ...). The whole module compiles to nothing on
 *          ports without clock points.
 *
 * @addtogroup CLI_CLOCK_CMDS
 * @{
 */

#ifndef CLOCK_CMDS_H
#define CLOCK_CMDS_H

#include "hal.h"
#include "xshell.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CLK_ARRAY_SIZE) || defined(__DOXYGEN__)
  void cmd_clocks(xshell_t *xshp, int argc, char *argv[], char *envp[]);
#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) || defined(__DOXYGEN__)
  void cmd_clock(xshell_t *xshp, int argc, char *argv[], char *envp[]);
#endif
  /* Tree-view hook: weak default prints the flat list; ports with a real
     clock tree (e.g. EFR32FG23) override it with a strong definition. */
  void cli_clock_tree_print(BaseSequentialStream *ssp);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CLOCK_CMDS_H */

/** @} */
