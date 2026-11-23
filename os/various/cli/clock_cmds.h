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
 * @brief   Generic clock commands, usable with both shell and xshell.
 *
 * @details Commands:
 *          - clocks          flat list of all clock points, rates and
 *                            consumer counts
 *          - lowpower [sec]  show the low-power mode decision (consumer
 *                            registry) and sleep in the port's low-power
 *                            mode (e.g. EM2 on SILABS EFR32), waking on
 *                            each interrupt
 *          - clocks w      clock tree view, if the port provides one via
 *                          the cli_clock_tree_print() hook (e.g. EFR32FG23
 *                          prints a Linux clk debugfs style tree)
 *          - clock reset|default   (only when HAL_LLD_USE_CLOCK_MANAGEMENT)
 *                          switch the clock configuration at run-time
 *
 *          The core functions have the classic shell signature and work
 *          for any port that defines CLK_ARRAY_SIZE and CLK_POINT_NAMES in
 *          its hal_lld.h (SILABS EFR32 ports, STM32 xhal ports, ...).
 *          Register them with CLI_SHELL_ENTRY()/CLI_XSHELL_ENTRY() from
 *          cli.h, depending on which shell(s) the application uses:
 *
 *          classic shell:  static const ShellCommand cmds[] = {
 *                            CLI_SHELL_ENTRY(cli_cmd_clocks, "clocks")
 *                            CLI_CLOCK_SHELL_ENTRY
 *                            {NULL, NULL}
 *                          };
 *          xshell:         static const xshell_command_t cmds[] = {
 *                            CLI_XSHELL_ENTRY(cli_cmd_clocks, "clocks")
 *                            CLI_CLOCK_XSHELL_ENTRY
 *                            {NULL, NULL}
 *                          };
 *
 *          The whole module compiles to nothing on ports without clock
 *          points (no CLK_ARRAY_SIZE).
 *
 * @addtogroup CLI_CLOCK_CMDS
 * @{
 */

#ifndef CLOCK_CMDS_H
#define CLOCK_CMDS_H

#include "cli.h"

#if defined(CLK_ARRAY_SIZE) || defined(__DOXYGEN__)

#ifdef __cplusplus
extern "C" {
#endif

  void cli_cmd_clocks(BaseSequentialStream *chp, int argc, char *argv[]);
  void cli_cmd_lowpower(BaseSequentialStream *chp, int argc, char *argv[]);
#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) || defined(__DOXYGEN__)
  void cli_cmd_clock(BaseSequentialStream *chp, int argc, char *argv[]);
#endif

  /* Consumer-count hook for the "clocks" users column (weak default 0). */
  unsigned cli_clock_users(halclkpt_t pt);
  /* Low-power hooks for the "lowpower" command (weak defaults: mode name
     "low-power", all points block it, entry not allowed, enter is a no-op).
     Ports override these; e.g. SILABS EFR32 enters EM2. */
  const char *cli_pm_lowpower_name(void);
  bool cli_pm_point_blocks_lowpower(halclkpt_t pt);
  bool cli_pm_lowpower_allowed(void);
  void cli_pm_lowpower_enter_once(void);

  /* Tree-view hook: weak default prints the flat list; ports with a real
     clock tree (e.g. EFR32FG23/efr32_clocks.c) override it with a strong
     definition. */
  void cli_clock_tree_print(BaseSequentialStream *ssp);

#if CLI_HAS_XSHELL
  /* xshell adapters (defined in clock_cmds.c), for CLI_XSHELL_ENTRY(). */
  void cli_xshell_cli_cmd_clocks(xshell_t *xshp, int argc, char *argv[], char *envp[]);
  void cli_xshell_cli_cmd_lowpower(xshell_t *xshp, int argc, char *argv[], char *envp[]);
#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) || defined(__DOXYGEN__)
  void cli_xshell_cli_cmd_clock(xshell_t *xshp, int argc, char *argv[], char *envp[]);
#endif
#endif /* CLI_HAS_XSHELL */

#ifdef __cplusplus
}
#endif

/* Registration entries. CLI_CLOCK_*_ENTRY expands to nothing when
   HAL_LLD_USE_CLOCK_MANAGEMENT is disabled. */
#define CLI_CLOCKS_SHELL_ENTRY    CLI_SHELL_ENTRY(cli_cmd_clocks, "clocks")
#define CLI_CLOCKS_XSHELL_ENTRY   CLI_XSHELL_ENTRY(cli_cmd_clocks, "clocks")
#define CLI_LOWPOWER_SHELL_ENTRY       CLI_SHELL_ENTRY(cli_cmd_lowpower, "lowpower")
#define CLI_LOWPOWER_XSHELL_ENTRY      CLI_XSHELL_ENTRY(cli_cmd_lowpower, "lowpower")

#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) || defined(__DOXYGEN__)
#define CLI_CLOCK_SHELL_ENTRY     CLI_SHELL_ENTRY(cli_cmd_clock, "clock")
#define CLI_CLOCK_XSHELL_ENTRY    CLI_XSHELL_ENTRY(cli_cmd_clock, "clock")
#else
#define CLI_CLOCK_SHELL_ENTRY
#define CLI_CLOCK_XSHELL_ENTRY
#endif

/* Ready-made NULL-terminated tables, for merging into an application's
   existing command table with cli_shell_merge()/cli_xshell_merge()
   without touching the application's own table, e.g.:
   @code
   static xshell_command_t merged[16];
   mgr_cfg.commands = cli_xshell_merge(app_commands,
                                       cli_xshell_clock_commands, merged);
   @endcode */
#if CLI_HAS_SHELL
  extern const ShellCommand cli_shell_clock_commands[];
#endif
#if CLI_HAS_XSHELL
  extern const xshell_command_t cli_xshell_clock_commands[];
#endif

#endif /* defined(CLK_ARRAY_SIZE) */

#endif /* CLOCK_CMDS_H */

/** @} */
