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
 * @brief   Generic xshell clock commands.
 *
 * @details Port-generic: the flat list uses CLK_ARRAY_SIZE/CLK_POINT_NAMES
 *          and halClockGetPointX() from the port. The "clocks w" tree view
 *          goes through the weak cli_clock_tree_print() hook — ports with
 *          a real clock tree (e.g. EFR32FG23/efr32_clocks.c) override it
 *          with a strong definition; otherwise the flat list is printed.
 *
 * @addtogroup CLI_CLOCK_CMDS
 * @{
 */

#include <string.h>

#include "hal.h"
#include "chprintf.h"
#include "xshell.h"

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
/* Commands.                                                                 */
/*===========================================================================*/

/**
 * @brief   "clocks" command: list clock points, "clocks w" for the tree.
 */
void cmd_clocks(xshell_t *xshp, int argc, char *argv[], char *envp[]) {

  (void)envp;

  if (argc == 2 && strcmp(argv[1], "w") == 0) {
    cli_clock_tree_print(xshp->stream);
    return;
  }

  if (argc != 1) {
    xshellUsage(xshp, "clocks [w]");
    return;
  }

  clock_points_print(xshp->stream);
}

#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) || defined(__DOXYGEN__)
/**
 * @brief   "clock" command: switch clock configuration at run-time.
 */
void cmd_clock(xshell_t *xshp, int argc, char *argv[], char *envp[]) {
  bool result;
  const halclkcfg_t *ccp;

  (void)envp;

  if (argc != 2) {
    xshellUsage(xshp, "clock reset|default");
    return;
  }

  if (strcmp(argv[1], "reset") == 0) {
    chprintf(xshp->stream, XSHELL_NEWLINE_STR "Switching to post-reset clock settings: ");
    ccp = &hal_clkcfg_reset;
  }
  else if (strcmp(argv[1], "default") == 0) {
    chprintf(xshp->stream, XSHELL_NEWLINE_STR "Switching to default mcuconf.h clock settings: ");
    ccp = &hal_clkcfg_default;
  }
  else {
    xshellUsage(xshp, "clock reset|default");
    return;
  }

  /* Time for the serial TX buffer to flush. */
  chThdSleepMilliseconds(10);

  /* Switching clocks. */
  result = halClockSwitchMode(ccp);

  /* Printing result. */
  if (result) {
    chprintf(xshp->stream, "failed" XSHELL_NEWLINE_STR);
  }
  else {
    chprintf(xshp->stream, "done" XSHELL_NEWLINE_STR);
  }
}
#endif /* defined(HAL_LLD_USE_CLOCK_MANAGEMENT) */

#endif /* defined(CLK_ARRAY_SIZE) */

/** @} */
