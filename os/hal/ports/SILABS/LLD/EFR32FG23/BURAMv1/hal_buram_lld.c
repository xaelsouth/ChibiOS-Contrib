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
 * separate RAM window. The whole BURAM has a single bus-clock gate,
 * CMU_CLKEN0.BURAM, shared by all four BURAMD1..4 blocks (RET0..7, RET8..15,
 * RET16..23, RET24..31 respectively). The RET words are retained in all
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

  /* Single clock gate shared by all BURAM blocks; it may already have been
     enabled during early init, so only set it if not yet on. */
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

  bool others_ready = false;

  /* The clock gate is shared: clear it only when no other compiled-in
     BURAM block is still in use. Note buramp itself is still BURAM_READY
     here (hal_buram.c sets BURAM_STOP after this call), so exclude it. */
  #if BURAM_USE_BURAM1
  if ((buramp != &BURAMD1) && (BURAMD1.state == BURAM_READY)) {
    others_ready = true;
  }
  #endif
  #if BURAM_USE_BURAM2
  if ((buramp != &BURAMD2) && (BURAMD2.state == BURAM_READY)) {
    others_ready = true;
  }
  #endif
  #if BURAM_USE_BURAM3
  if ((buramp != &BURAMD3) && (BURAMD3.state == BURAM_READY)) {
    others_ready = true;
  }
  #endif
  #if BURAM_USE_BURAM4
  if ((buramp != &BURAMD4) && (BURAMD4.state == BURAM_READY)) {
    others_ready = true;
  }
  #endif

  if (!others_ready) {
    CMU->CLKEN0_CLR = CMU_CLKEN0_BURAM;
  }
}

#endif /* HAL_USE_BURAM */

/** @} */
