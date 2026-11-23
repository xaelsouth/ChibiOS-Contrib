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
 * @file    clock_cmds.c
 * @brief   Generic clock commands, usable with both shell and xshell.
 *
 * @details Port-generic: the flat list uses CLK_ARRAY_SIZE/CLK_POINT_NAMES
 *          and halClockGetPointX() from the port. The "clocks w" tree view
 *          goes through the weak cli_clock_tree_print() hook — ports with
 *          a real clock tree (e.g. EFR32FG23/efr32_clocks.c) override it
 *          with a strong definition; otherwise the flat list is printed.
 *
 *          The cli_cmd_*() cores have the classic shell signature; the
 *          cli_xshell_*() adapters (compiled when xshell is in the build)
 *          forward xshp->stream to them — see cli.h.
 *
 * @addtogroup CLI_CLOCK_CMDS
 * @{
 */

#include <string.h>

#include "hal.h"
#include "chprintf.h"

#include "cli.h"
#include "clock_cmds.h"

#if defined(CLK_ARRAY_SIZE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Flat list of clock points.                                                */
/*===========================================================================*/

/**
 * @brief   Weak hook: number of consumers registered on a clock point.
 * @details Ports with a consumer registry (e.g. EFR32FG23/efr32_clocks.c)
 *          override this with a strong definition; default is 0.
 */
CC_WEAK unsigned cli_clock_users(halclkpt_t pt) {

  (void)pt;
  return 0U;
}

/**
 * @brief   Prints the flat list of all clock points, rates and consumer
 *          counts.
 */
static void clock_points_print(BaseSequentialStream *ssp) {
  static const char *names[CLK_ARRAY_SIZE] = CLK_POINT_NAMES;
  unsigned i;

  chprintf(ssp, "%-14s %12s %6s\r\n", "clock", "rate [Hz]", "users");
  for (i = 0U; i < (unsigned)CLK_ARRAY_SIZE; i++) {
    chprintf(ssp, "%-14s %12u %6u\r\n", names[i], (unsigned)halClockGetPointX(i),
             cli_clock_users((halclkpt_t)i));
  }
}

/**
 * @brief   Default (weak) clock tree view: no tree on this port, so just
 *          print the flat list. Ports with a real clock tree override this
 *          with a strong definition.
 *
 * @param[in] ssp       output stream
 */
CC_WEAK void cli_clock_tree_print(BaseSequentialStream *ssp) {

  chprintf(ssp, "no clock tree on this port, flat list:\r\n");
  clock_points_print(ssp);
}

/*===========================================================================*/
/* Core commands (classic shell signature).                                  */
/*===========================================================================*/

/**
 * @brief   "clocks" command: list clock points, "clocks w" for the tree.
 */
void cli_cmd_clocks(BaseSequentialStream *chp, int argc, char *argv[]) {

  if (argc == 2 && strcmp(argv[1], "w") == 0) {
    cli_clock_tree_print(chp);
    return;
  }

  if (argc != 1) {
    chprintf(chp, "Usage: clocks [w]\r\n");
    return;
  }

  clock_points_print(chp);
}

#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) || defined(__DOXYGEN__)
/**
 * @brief   "clock" command: switch clock configuration at run-time.
 */
void cli_cmd_clock(BaseSequentialStream *chp, int argc, char *argv[]) {
  bool result;
  const halclkcfg_t *ccp;

  if (argc != 2) {
    chprintf(chp, "Usage: clock reset|default\r\n");
    return;
  }

  if (strcmp(argv[1], "reset") == 0) {
    chprintf(chp, "\r\nSwitching to post-reset clock settings: ");
    ccp = &hal_clkcfg_reset;
  }
  else if (strcmp(argv[1], "default") == 0) {
    chprintf(chp, "\r\nSwitching to default mcuconf.h clock settings: ");
    ccp = &hal_clkcfg_default;
  }
  else {
    chprintf(chp, "Usage: clock reset|default\r\n");
    return;
  }

  /* Time for the serial TX buffer to flush. */
  chThdSleepMilliseconds(10);

  /* Switching clocks. */
  result = halClockSwitchMode(ccp);

  /* Printing result. */
  if (result) {
    chprintf(chp, "failed\r\n");
  }
  else {
    chprintf(chp, "done\r\n");
  }
}
#endif /* defined(HAL_LLD_USE_CLOCK_MANAGEMENT) */

/*===========================================================================*/
/* Power management: "em2" command.                                          */
/*===========================================================================*/

/**
 * @brief   Weak hook: does this clock point block EM2?
 * @details Ports override (default: every point blocks EM2).
 */
CC_WEAK bool cli_pm_point_blocks_em2(halclkpt_t pt) {

  (void)pt;
  return true;
}

/**
 * @brief   Weak hook: is EM2 entry currently allowed?
 * @details Ports with a consumer registry override this (default: no).
 */
CC_WEAK bool cli_pm_em2_allowed(void) {

  return false;
}

/**
 * @brief   Weak hook: enter EM2 once, wake on the next interrupt.
 * @details Ports override (default: no-op).
 */
CC_WEAK void cli_pm_em2_enter_once(void) {

}

/**
 * @brief   "em2 [seconds]" command: show the EM2 decision, then sleep in
 *          EM2 for the given time (default 5 s), waking on each interrupt.
 */
void cli_cmd_em2(BaseSequentialStream *chp, int argc, char *argv[]) {
  static const char *names[CLK_ARRAY_SIZE] = CLK_POINT_NAMES;
  uint32_t seconds = 5U;
  systime_t deadline;
  unsigned wakes = 0U;
  unsigned i;

  if (argc == 2) {
    seconds = (uint32_t)strtoul(argv[1], NULL, 0);
    if (seconds == 0U) {
      chprintf(chp, "Usage: em2 [seconds]\r\n");
      return;
    }
  }
  else if (argc != 1) {
    chprintf(chp, "Usage: em2 [seconds]\r\n");
    return;
  }

  if (!cli_pm_em2_allowed()) {
    chprintf(chp, "EM2 blocked — consumers on EM0-only clock points:\r\n");
    for (i = 0U; i < (unsigned)CLK_ARRAY_SIZE; i++) {
      unsigned u = cli_clock_users((halclkpt_t)i);
      if ((u > 0U) && cli_pm_point_blocks_em2((halclkpt_t)i)) {
        chprintf(chp, "  %-14s users=%u\r\n", names[i], u);
      }
    }
    return;
  }

  chprintf(chp, "EM2 allowed — sleeping in EM2 for %u s (wake on each IRQ)...\r\n",
           (unsigned)seconds);
  deadline = osalOsGetSystemTimeX() + (systime_t)seconds * OSAL_ST_FREQUENCY;
  while (osalOsGetSystemTimeX() < deadline) {
    cli_pm_em2_enter_once();
    wakes++;
  }
  chprintf(chp, "woke %u times, done\r\n", wakes);
}

/*===========================================================================*/
/* xshell adapters.                                                          */
/*===========================================================================*/

#if CLI_HAS_XSHELL
CLI_XSHELL_ADAPTER(cli_cmd_clocks)
CLI_XSHELL_ADAPTER(cli_cmd_em2)
#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) || defined(__DOXYGEN__)
CLI_XSHELL_ADAPTER(cli_cmd_clock)
#endif
#endif /* CLI_HAS_XSHELL */

/*===========================================================================*/
/* Ready-made command tables (for cli_shell_merge()/cli_xshell_merge()).     */
/*===========================================================================*/

#if CLI_HAS_SHELL
const ShellCommand cli_shell_clock_commands[] = {
  CLI_CLOCKS_SHELL_ENTRY
  CLI_EM2_SHELL_ENTRY
  CLI_CLOCK_SHELL_ENTRY
  {NULL, NULL}
};
#endif

#if CLI_HAS_XSHELL
const xshell_command_t cli_xshell_clock_commands[] = {
  CLI_CLOCKS_XSHELL_ENTRY
  CLI_EM2_XSHELL_ENTRY
  CLI_CLOCK_XSHELL_ENTRY
  {NULL, NULL}
};
#endif

#endif /* defined(CLK_ARRAY_SIZE) */

/** @} */
