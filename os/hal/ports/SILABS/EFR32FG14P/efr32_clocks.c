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
 * @file    efr32_clocks.c
 * @brief   EFR32FG14P clock inspection helpers.
 *
 * @details Flat clock point listing only — the FG14P (Series 1) has no
 *          Series-2 style group clocks, so there is no tree view.
 *
 * @addtogroup EFR32FG14P_CLOCKS
 * @{
 */

#include "hal.h"
#include "chprintf.h"
#include "efr32_clocks.h"

/**
 * @brief   Prints the flat list of all clock points and their rates.
 *
 * @param[in] ssp       output stream
 */
void efr32_clock_points_print(BaseSequentialStream *ssp) {
  static const char *names[CLK_ARRAY_SIZE] = CLK_POINT_NAMES;
  unsigned i;

  chprintf(ssp, "%-14s %12s\r\n", "clock", "rate [Hz]");
  for (i = 0U; i < (unsigned)CLK_ARRAY_SIZE; i++) {
    chprintf(ssp, "%-14s %12u\r\n", names[i], (unsigned)halClockGetPointX(i));
  }
}

/** @} */
