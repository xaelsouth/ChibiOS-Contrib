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
 * @brief   Prints the flat list of all clock points and their rates.
 */
static void clock_points_print(BaseSequentialStream *ssp) {
  static const char *names[CLK_ARRAY_SIZE] = CLK_POINT_NAMES;
  unsigned i;

  chprintf(ssp, "%-14s %12s\r\n", "clock", "rate [Hz]");
  for (i = 0U; i < (unsigned)CLK_ARRAY_SIZE; i++) {
    chprintf(ssp, "%-14s %12u\r\n", names[i], (unsigned)halClockGetPointX(i));
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
/* xshell adapters.                                                          */
/*===========================================================================*/

#if CLI_HAS_XSHELL
CLI_XSHELL_ADAPTER(cli_cmd_clocks)
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
  CLI_CLOCK_SHELL_ENTRY
  {NULL, NULL}
};
#endif

#if CLI_HAS_XSHELL
const xshell_command_t cli_xshell_clock_commands[] = {
  CLI_CLOCKS_XSHELL_ENTRY
  CLI_CLOCK_XSHELL_ENTRY
  {NULL, NULL}
};
#endif

#endif /* defined(CLK_ARRAY_SIZE) */

/** @} */
