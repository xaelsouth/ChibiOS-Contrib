/*
    ChibiOS/HAL - Copyright (C) 2024 Xael South

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
 * @file    hal_buram_lld.c
 * @brief   BURAM Driver subsystem low level driver source.
 *
 * @addtogroup BURAM
 * @{
 */
#include "hal.h"

#if (HAL_USE_BURAM == TRUE) || defined(__DOXYGEN__)

/* hal_buram.h pulls in hal_buram_lld.h and defines BURAMDriver/buramstate_t,
   which buram_lld_stop() needs for the runtime state check. */
#include "hal_buram.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/*
 * EFR32FG23 BURAM notes (RM chapter 17): the BURAM *is* the 32 RETx_REG
 * retention registers — 32 x 32-bit = 128 bytes at BURAM_BASE + 0x000..0x07C,
 * with SET/CLR/TGL write aliases at +0x1000/+0x2000/+0x3000. There is no
 * separate RAM window. One peripheral, one 128-byte block (BURAMD1), one
 * bus-clock gate (CMU_CLKEN0.BURAM). The RET words are retained in all
 * energy modes including EM4 by hardware — no retention enable needed.
 */

/**
 * @brief   Configures and activates the BURAM peripheral.
 *
 * @param[in] buramp        pointer to the @p BURAMDriver object
 * @param[in] cfgp          pointer to the @p BURAMConfig object
 *
 * @notapi
 */
void buram_lld_start(BURAMDriver *buramp, const BURAMConfig *cfgp) {

  (void)buramp;
  (void)cfgp;

  /* Single clock gate; it may already have been enabled during early
     init, so only set it if not yet on. */
  if ((CMU->CLKEN0 & CMU_CLKEN0_BURAM) == 0U) {
    CMU->CLKEN0_SET = CMU_CLKEN0_BURAM;
  }
}

/**
 * @brief   Deactivates the BURAM peripheral.
 *
 * @param[in] buramp        pointer to the @p BURAMDriver object
 *
 * @notapi
 */
void buram_lld_stop(BURAMDriver *buramp) {

  (void)buramp;

  /* Single driver for the single hardware block: clear the gate. */
  CMU->CLKEN0_CLR = CMU_CLKEN0_BURAM;
}

#endif /* HAL_USE_BURAM */

/** @} */
