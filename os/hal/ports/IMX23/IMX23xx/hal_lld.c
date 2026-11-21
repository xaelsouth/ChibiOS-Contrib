/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

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
 * @file    IMX23/IMX23xx/hal_lld.c
 * @brief   i.MX23 HAL subsystem low level driver code.
 *
 * @details Power and DRAM are initialized by the bootstream before the
 *          ChibiOS image is jumped to, so no EMI/power setup is done here.
 *          This driver only initializes the interrupt collector.
 *
 * @addtogroup IMX23
 * @{
 */

#include "hal.h"
#include "board.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/* CPU clock as left by the bootstream, board overridable. */
#if !defined(IMX23_CPU_HZ) || defined(__DOXYGEN__)
#define IMX23_CPU_HZ    454000000U
#endif

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Returns the frequency of a clock point in Hz.
 *
 * @param[in] clkpt     clock point to be returned
 * @return              The clock point frequency in Hz.
 *
 * @notapi
 */
uint32_t hal_lld_get_clock_point(imx23_clkpt_t clkpt) {

  switch (clkpt) {
  case IMX23_CLKPT_XTAL:
    return 24000000U;
  case IMX23_CLKPT_CPU:
    return IMX23_CPU_HZ;
  case IMX23_CLKPT_HBUS:
    return IMX23_CPU_HZ / 2U;
  case IMX23_CLKPT_DUART:
    return IMX23_DUART_CLK_HZ;
  case IMX23_CLKPT_TIMROT:
    return IMX23_TIMROT_TICK_HZ;
  }
  return 0U;
}

/**
 * @brief   Low level HAL driver initialization.
 * @note    Runs with the MMU/caches off (as left by the bootstream) and
 *          before any driver is started; exception vectors have already
 *          been placed in OCRAM by __early_init().
 *
 * @notapi
 */
void hal_lld_init(void) {

  /* Interrupt collector: vector table + IRQ routing to the CPU. */
  icoll_init();
}

/** @} */
