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
#include "hal_lesense_lld.h"

#if EFR32_LESENSE_USE_LESENSE || defined(__DOXYGEN__)

#if !defined(EFR32_DMA_REQUIRED)
#define EFR32_DMA_REQUIRED
#endif

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
  LESENSED1.dma = NULL;
}

void lesense_lld_start(LESENSEDriver *lp, const LESENSEConfig *cfg) {
  uint32_t chen = 0;

  lp->config = cfg;

  /* Enable bus clock. */
  CMU->CLKEN0_SET = CMU_CLKEN0_LESENSE;

  /* LESENSE registers are clocked from LESENSEHFCLK; the reset default is
     FSRCO which may be disabled — select HFRCOEM23 (always available in
     EM0-EM2), otherwise register access faults. */
  CMU->LESENSEHFCLKCTRL = (CMU->LESENSEHFCLKCTRL & ~_CMU_LESENSEHFCLKCTRL_CLKSEL_MASK) |
                          CMU_LESENSEHFCLKCTRL_CLKSEL_HFRCOEM23;

  /* Configuration registers must be written while the module is disabled
     (EN=0); writing them while enabled bus-faults on xG23. */
  osalDbgAssert((LESENSE->EN & LESENSE_EN_EN) == 0U, "LESENSE already enabled");

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

  /* Enable interrupts for all active channels + scan done + result watermark. */
  LESENSE->IEN_SET = chen | LESENSE_IEN_SCANDONE | LESENSE_IEN_RESWL;

  /* Enable LESENSE last, after all configuration. */
  LESENSE->EN_SET = LESENSE_EN_EN;

  /* Wait for LF-domain synchronization of EN before any CMD write —
     writing CMD while SYNCBUSY is set bus-faults. */
  {
    volatile uint32_t sp = 1000000UL;
    while (((LESENSE->SYNCBUSY & _LESENSE_SYNCBUSY_MASK) != 0U) && (--sp != 0U)) {}
  }

  nvicEnableVector(EFR32_LESENSE_NUMBER, EFR32_LESENSE_IRQ_PRIORITY);

  /* Allocate DMA stream for result FIFO if configured. */
#if defined(EFR32_DMA_REQUIRED)
  if (EFR32_LESENSE_DMA_STREAM >= 0 && lp->dma == NULL) {
    lp->dma = dmaStreamAllocI(EFR32_LESENSE_DMA_STREAM,
                             EFR32_LESENSE_DMA_IRQ_PRIORITY, NULL, NULL);
  }
#endif

  lp->state = LESENSE_READY;
}

void lesense_lld_stop(LESENSEDriver *lp) {
  nvicDisableVector(EFR32_LESENSE_NUMBER);
#if defined(EFR32_DMA_REQUIRED)
  if (lp->dma != NULL) {
    dmaStreamFreeI(lp->dma);
    lp->dma = NULL;
  }
#endif
  LESENSE->IEN_CLR = _LESENSE_IEN_MASK;
  /* Disable the module first (a scan should have been stopped with
     lesense_lld_stop_scan() already); wait for register synchronization
     before touching the bus clock, otherwise the writes can bus-fault. */
  LESENSE->EN_CLR = LESENSE_EN_EN;
  {
    volatile uint32_t sp = 1000000UL;
    while (((LESENSE->SYNCBUSY & _LESENSE_SYNCBUSY_MASK) != 0U) && (--sp != 0U)) {}
  }
  /* Note: no CMD STOP here — writing CMD while SYNCBUSY is set bus-faults;
     EN_CLR already stops the scan engine. */
  CMU->CLKEN0_CLR = CMU_CLKEN0_LESENSE;
  lp->state = LESENSE_STOP;
  lp->config = NULL;
}

void lesense_lld_start_scan(LESENSEDriver *lp) {
  (void)lp;
  /* CMD writes bus-fault while SYNCBUSY is set — wait first. */
  volatile uint32_t sp = 1000000UL;
  while (((LESENSE->SYNCBUSY & _LESENSE_SYNCBUSY_MASK) != 0U) && (--sp != 0U)) {}
  LESENSE->CMD_SET = LESENSE_CMD_START;
}

void lesense_lld_stop_scan(LESENSEDriver *lp) {
  (void)lp;
  volatile uint32_t sp = 1000000UL;
  while (((LESENSE->SYNCBUSY & _LESENSE_SYNCBUSY_MASK) != 0U) && (--sp != 0U)) {}
  LESENSE->CMD_SET = LESENSE_CMD_STOP;
}

uint32_t lesense_lld_get_scanres(LESENSEDriver *lp) {
  return lp->scanres;
}

/**
 * @brief   Change the scan frequency at runtime.
 * @details Reprograms only the LFPRESC/STARTDLY fields of TIMCTRL, so the
 *          scan period becomes
 *            f_scan = EFR32_EM23GRPACLK / (2^(LFPRESC+1) x (STARTDLY+1)).
 *          The smallest achievable frequency >= @p hz is selected; if
 *          @p hz is above the grid maximum, the maximum is used.
 *          SYNCBUSY is waited out (bounded) before touching the LF-domain
 *          register — writing while busy bus-faults on xG23.
 *
 * @param[in] lp    pointer to LESENSEDriver
 * @param[in] hz    requested scan frequency in Hz
 */
void lesense_lld_set_scan_hz(LESENSEDriver *lp, float hz) {
  (void)lp;

  /* Grid search: smallest f >= hz over LFPRESC (0..7 -> div 2..256) and
     STARTDLY (0..3 -> mul 1..4). */
  uint32_t best_p = 0U, best_d = 0U;
  float best_f = 0.0f;
  for (uint32_t pr = 0U; pr < 8U; pr++) {
    for (uint32_t d = 0U; d < 4U; d++) {
      float f = (float)EFR32_EM23GRPACLK /
                ((float)(1UL << (pr + 1U)) * (float)(d + 1U));
      if (f >= hz && (best_f == 0.0f || f < best_f)) {
        best_f = f; best_p = pr; best_d = d;
      }
    }
  }
  if (best_f == 0.0f) {   /* hz above grid max -> maximum rate */
    best_p = 0U; best_d = 0U;
  }

  volatile uint32_t sp = 4000000UL;
  while (((LESENSE->SYNCBUSY & _LESENSE_SYNCBUSY_MASK) != 0U) && (--sp != 0U)) {}

  uint32_t timctrl = LESENSE->TIMCTRL;
  timctrl &= ~(_LESENSE_TIMCTRL_LFPRESC_MASK | _LESENSE_TIMCTRL_STARTDLY_MASK);
  timctrl |= (best_p << _LESENSE_TIMCTRL_LFPRESC_SHIFT) & _LESENSE_TIMCTRL_LFPRESC_MASK;
  timctrl |= (best_d << _LESENSE_TIMCTRL_STARTDLY_SHIFT) & _LESENSE_TIMCTRL_STARTDLY_MASK;
  LESENSE->TIMCTRL = timctrl;
}

#if defined(EFR32_DMA_REQUIRED) || defined(__DOXYGEN__)

/**
 * @brief   Start DMA draining of the LESENSE result FIFO into a memory buffer.
 * @details Each scan that produces a result pushes one word into RESFIFO.
 *          DMA reads RESFIFO → buf[n] automatically on LESENSEFIFO request.
 *
 * @param[in] lp    pointer to LESENSEDriver
 * @param[in] buf   destination buffer (array of uint32_t)
 * @param[in] n     number of FIFO entries to drain
 */
void lesense_lld_start_fifo_dma(LESENSEDriver *lp, uint32_t *buf, size_t n) {
  const efr32_dma_stream_t *st = lp->dma;
  if (st == NULL || n == 0U) return;

  dmaStreamSetRequestSource(st, LDMAXBAR_CH_REQSEL_SOURCESEL_LESENSE >> 16,
                            LDMAXBAR_CH_REQSEL_SIGSEL_LESENSEFIFO & 0xFU);
  dmaStartPeriphToMem(st, &LESENSE->RESFIFO, buf, n,
                      EFR32_DMA_CH_CTRL_SIZE_WORD);
}

/**
 * @brief   Stop the result FIFO DMA and free the stream.
 */
void lesense_lld_stop_fifo_dma(LESENSEDriver *lp) {
  const efr32_dma_stream_t *st = lp->dma;
  if (st != NULL) {
    dmaStreamDisable(st);
  }
}

#endif /* EFR32_DMA_REQUIRED */

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
