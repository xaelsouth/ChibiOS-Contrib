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
 * @file    LESENSEv1/hal_lesense_lld.c
 * @brief   LESENSE low-level driver source.
 *
 * @details LESENSE autonomously scans sensor channels in EM2/EM3.
 *          For a water meter with a rotating disc + metal element:
 *          use one channel in inductive mode (LC sensor) - the metal
 *          element passing over the coil changes the resonant frequency,
 *          which LESENSE detects as a count change above threshold.
 *
 * @addtogroup LESENSE
 * @{
 */

#include "hal.h"

#if EFR32_LESENSE_USE_LESENSE || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

LESENSEDriver LESENSED1;

/*===========================================================================*/
/* Driver interrupt handler.                                                 */
/*===========================================================================*/

OSAL_IRQ_HANDLER(EFR32_LESENSE_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  lesense_lld_serve_interrupt(&LESENSED1);
  OSAL_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void lesense_lld_init(void) {
  LESENSED1.state = LESENSE_STOP;
  LESENSED1.config = NULL;
  LESENSED1.scanres = 0;
}

void lesense_lld_start(LESENSEDriver *lp, const LESENSEConfig *cfg) {
  uint32_t chen = 0;

  lp->config = cfg;

  /* Enable bus clock. */
  CMU->CLKEN0_SET = CMU_CLKEN0_LESENSE;

  /* Enable LESENSE. */
  LESENSE->EN_SET = LESENSE_EN_EN;

  /* Configure scan mode. */
  LESENSE->CFG = (LESENSE->CFG & ~_LESENSE_CFG_SCANMODE_MASK) |
                (cfg->scanmode & _LESENSE_CFG_SCANMODE_MASK);

  /* Timing, peripheral control. */
  LESENSE->TIMCTRL = cfg->timctrl;
  LESENSE->PERCTRL = cfg->perctrl;

  /* Configure each channel. */
  for (uint32_t i = 0; i < cfg->num_channels && i < EFR32_LESENSE_MAX_CHANNELS; i++) {
    LESENSE->CH[i].INTERACT  = cfg->channels[i].interact;
    LESENSE->CH[i].EVALCFG   = cfg->channels[i].evalcfg;
    LESENSE->CH[i].EVALTHRES = cfg->channels[i].evalthres;
    chen |= (1UL << i);
  }
  LESENSE->CHEN = chen;

  /* Clear result buffer and flags. */
  LESENSE->CMD_SET = LESENSE_CMD_CLEARBUF;
  LESENSE->IF_CLR = _LESENSE_IF_MASK;

  /* Enable interrupts for all active channels + scan done. */
  LESENSE->IEN_SET = chen | LESENSE_IEN_SCANDONE;

  nvicEnableVector(EFR32_LESENSE_NUMBER, EFR32_LESENSE_IRQ_PRIORITY);

  lp->state = LESENSE_READY;
}

void lesense_lld_stop(LESENSEDriver *lp) {
  nvicDisableVector(EFR32_LESENSE_NUMBER);
  LESENSE->IEN_CLR = _LESENSE_IEN_MASK;
  LESENSE->CMD_SET = LESENSE_CMD_STOP;
  LESENSE->EN_CLR = LESENSE_EN_EN;
  CMU->CLKEN0_CLR = CMU_CLKEN0_LESENSE;
  lp->state = LESENSE_STOP;
  lp->config = NULL;
}

void lesense_lld_start_scan(LESENSEDriver *lp) {
  (void)lp;
  LESENSE->CMD_SET = LESENSE_CMD_START;
}

void lesense_lld_stop_scan(LESENSEDriver *lp) {
  (void)lp;
  LESENSE->CMD_SET = LESENSE_CMD_STOP;
}

uint32_t lesense_lld_get_scanres(LESENSEDriver *lp) {
  return lp->scanres;
}

void lesense_lld_serve_interrupt(LESENSEDriver *lp) {
  uint32_t isr = LESENSE->IF;
  LESENSE->IF_CLR = isr;

  if (isr & LESENSE_IF_SCANDONE) {
    lp->scanres = LESENSE->SCANRES;
    if (lp->config && lp->config->callback) {
      lp->config->callback(lp->scanres, lp->config->callback_arg);
    }
  }

  /* Individual channel hits (bits 0-15) are also captured in SCANRES. */
  if (isr & 0xFFFFU) {
    lp->scanres = LESENSE->SCANRES;
  }
}

#endif /* EFR32_LESENSE_USE_LESENSE */

/** @} */
