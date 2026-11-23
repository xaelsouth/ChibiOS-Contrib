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
 * @file    efr32_clocks.h
 * @brief   EFR32FG23 clock tree inspection helpers.
 *
 * @details Declared in a separate header (not hal_lld.h) because
 *          BaseSequentialStream is only defined later in hal.h.
 *          Used e.g. by the xshell "clocks" command
 *          (LLD/xshell/efr32_clock_cmds.c).
 *
 * @addtogroup EFR32FG23_CLOCKS
 * @{
 */

#ifndef EFR32_CLOCKS_H
#define EFR32_CLOCKS_H

#include "hal.h"

#ifdef __cplusplus
extern "C" {
#endif

  void efr32_clock_points_print(BaseSequentialStream *ssp);
  void efr32_clock_tree_print(BaseSequentialStream *ssp);

  /*
   * Consumer registry: drivers register a consumer on a clock point while
   * they need that clock running (e.g. a peripheral on HCLK/PCLK, the radio
   * on RHCLK, the LF oscillator for the RTC).  The "clocks" command prints
   * the counts; efr32_em2_allowed() uses them to decide whether the device
   * may enter EM2 (any consumer on an EM0-only clock point blocks EM2).
   */
  void     efr32_clock_consumer_acquire(halclkpt_t pt);
  void     efr32_clock_consumer_release(halclkpt_t pt);
  unsigned efr32_clock_consumers(halclkpt_t pt);
  bool     efr32_clock_pt_blocks_em2(halclkpt_t pt);
  bool     efr32_em2_allowed(void);

#ifdef __cplusplus
}
#endif

#endif /* EFR32_CLOCKS_H */

/** @} */
