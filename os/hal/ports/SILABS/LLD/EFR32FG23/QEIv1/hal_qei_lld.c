/*
    ChibiOS - Copyright (C) 2026 Xael South
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
        http://www.apache.org/licenses/LICENSE-2.0
*/

/**
 * @file    QEIv1/hal_qei_lld.c
 * @brief   Quadrature encoder (PCNT0) LLD source.
 *
 * @addtogroup QEI
 * @{
 */

#include "hal.h"

#if (HAL_USE_QEI == TRUE) || defined(__DOXYGEN__)

#if EFR32_QEI_USE_PCNT0 || defined(__DOXYGEN__)
QEIDriver QEID1;

OSAL_IRQ_HANDLER(EFR32_PCNT0_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  qei_lld_serve_interrupt(&QEID1);
  OSAL_IRQ_EPILOGUE();
}
#endif

void qei_lld_init(void) {
#if EFR32_QEI_USE_PCNT0
  QEID1.state = QEI_STOP; QEID1.config = NULL;
#endif
}

/* Wait for PCNT SYNCBUSY to clear. In external-clock mode (CLKSEL=PCNTS0)
   the LF register sync runs on the S0 clock, so we must feed S0 edges on the
   configured PRS channel, otherwise SYNCBUSY never clears and writes to PCNT
   registers would stall the bus.
   NOTE: ASYNC_SWPULSE alone is a one-EM01GRPACLK-cycle (~50 ns) pulse — too
   narrow for the CMU PCNTS0 glitch-free mux synchronizer, so switching
   CLKSEL to PCNTS0 while S0 is idle never completes and the domain stays
   dead (verified on BRD2600A). Toggle ASYNC_SWLEVEL with a wide high/low
   instead: those edges reliably clock both the CMU switch and the PCNT LF
   sync domain. SWPULSE pulses still count in CNT once the domain is alive. */
static void pcnt_sync_wait(uint32_t s0prs) {
  /* ~1 ms per level at 19 MHz HCLK: wide enough for the CMU PCNTS0 mux
     synchronizer (narrow SWPULSE pulses get swallowed — verified on
     BRD2600A). Bounded: worst case ~0.4 s when the domain is dead. */
  volatile uint32_t guard = 200U;
  uint32_t mask = s0prs < 12U ? (1U << s0prs) : 0U;
  uint32_t original_level = mask != 0U ? PRS->ASYNC_SWLEVEL & mask : 0U;
  while (((PCNT0->SYNCBUSY & _PCNT_SYNCBUSY_MASK) != 0U) && (--guard != 0U)) {
    if (mask != 0U) {
      PRS->ASYNC_SWLEVEL_SET = mask;
      for (volatile uint32_t d = 0U; d < 20000U; d++) {}
      PRS->ASYNC_SWLEVEL_CLR = mask;
      for (volatile uint32_t d = 0U; d < 20000U; d++) {}
    }
  }
  if (original_level != 0U) {
    PRS->ASYNC_SWLEVEL_SET = mask;
  }
}

void qei_lld_start(QEIDriver *qp) {
  const QEIConfig *cfg = qp->config;
  CMU->CLKEN1_SET = CMU_CLKEN1_PCNT0;

  PCNT0->EN_CLR = PCNT_EN_EN;
  { volatile uint32_t sp = 4000000UL;
    while (((PCNT0->SYNCBUSY & _PCNT_SYNCBUSY_MASK) != 0U) && (--sp != 0U)) {} }
  /* External clock modes: switch the PCNT clock domain to S0IN
     (CMU PCNT0CLKCTRL.CLKSEL=PCNTS0) and core-reset before the change.
     Without this the PCNT free-runs on EM23GRPACLK (~32 kHz). */
  bool extclk = ((cfg->cfg & _PCNT_CFG_MODE_MASK) == PCNT_CFG_MODE_EXTCLKSINGLE ||
                 (cfg->cfg & _PCNT_CFG_MODE_MASK) == PCNT_CFG_MODE_EXTCLKQUAD);
  if (extclk) {
    CMU->CLKEN0_SET = CMU_CLKEN0_PRS;   /* needed to pump SWPULSE below */
    PCNT0->CMD_SET = PCNT_CMD_CORERST;
    pcnt_sync_wait(12U);   /* domain still on EM23GRPACLK here */
    /* Route PRS async channels into S0IN/S1IN BEFORE switching the clock
       domain: series 2 needs S0PRSEN/S1PRSEN + PRS->CONSUMER_PCNTx. This must
       happen while the LF domain still runs on EM23GRPACLK, otherwise the
       SWPULSE sync pulses below never reach S0IN and SYNCBUSY never clears
       (guard expires, next PCNT write stalls the bus). */
    PCNT0->CFG = cfg->cfg;
    if (cfg->s0prs < 12U) {
      PCNT0->CFG |= PCNT_CFG_S0PRSEN;     /* S0IN taken from PRS, not the pin */
      PRS->CONSUMER_PCNT0_S0IN = cfg->s0prs;
    }
    if (cfg->s1prs < 12U) {
      PCNT0->CFG |= PCNT_CFG_S1PRSEN;
      PRS->CONSUMER_PCNT0_S1IN = cfg->s1prs;
    }
    pcnt_sync_wait(12U);   /* CFG write syncs on EM23GRPACLK */
    /* The CMU glitch-free mux needs edges on the NEW clock around the switch:
       if S0 is idle when CLKSEL is set to PCNTS0, the switch never completes
       and the PCNT LF domain stays dead (SYNCBUSY stuck — verified on
       BRD2600A). Feed a few wide S0 edges (SWLEVEL toggles) before and after
       the switch. */
    if (cfg->s0prs < 12U) {
      uint32_t mask = 1U << cfg->s0prs;
      uint32_t original_level = PRS->ASYNC_SWLEVEL & mask;
      for (int i = 0; i < 8; i++) {
        PRS->ASYNC_SWLEVEL_SET = mask;
        for (volatile uint32_t d = 0U; d < 20000U; d++) {}
        PRS->ASYNC_SWLEVEL_CLR = mask;
        for (volatile uint32_t d = 0U; d < 20000U; d++) {}
      }
      if (original_level != 0U) {
        PRS->ASYNC_SWLEVEL_SET = mask;
      }
    }
    CMU->PCNT0CLKCTRL = CMU_PCNT0CLKCTRL_CLKSEL_PCNTS0;
    /* now the LF domain is clocked from S0: pump edges to let it sync */
    pcnt_sync_wait(cfg->s0prs);
  }
  else {
    PCNT0->CFG = cfg->cfg;
  }
  PCNT0->CTRL = cfg->ctrl;
  PCNT0->TOP = cfg->top;
  PCNT0->TOPB = cfg->top;
  PCNT0->IF_CLR = _PCNT_IF_MASK;
  PCNT0->IEN_SET = PCNT_IEN_OF | PCNT_IEN_UF | PCNT_IEN_DIRCNG;
  PCNT0->EN_SET = PCNT_EN_EN;
  pcnt_sync_wait(extclk ? cfg->s0prs : 12U);
  PCNT0->CMD_SET = PCNT_CMD_CNTRST;   /* CNT is read-only on series 2 */
  PCNT0->CMD_SET = PCNT_CMD_STARTCNT;
  pcnt_sync_wait(extclk ? cfg->s0prs : 12U);

  nvicEnableVector(EFR32_PCNT0_NUMBER, EFR32_QEI_PCNT0_IRQ_PRIORITY);
  qp->state = QEI_READY;
}

void qei_lld_stop(QEIDriver *qp) {
  nvicDisableVector(EFR32_PCNT0_NUMBER);
  PCNT0->CMD_SET = PCNT_CMD_STOPCNT;
  /* back to the internal LF clock first, otherwise the ext-clocked domain
     cannot sync and the following writes would stall the bus */
  CMU->PCNT0CLKCTRL = CMU_PCNT0CLKCTRL_CLKSEL_EM23GRPACLK;
  { volatile uint32_t sp = 4000000UL;
    while (((PCNT0->SYNCBUSY & _PCNT_SYNCBUSY_MASK) != 0U) && (--sp != 0U)) {} }
  PCNT0->IEN_CLR = _PCNT_IEN_MASK;
  PCNT0->EN_CLR = PCNT_EN_EN;
  while ((PCNT0->EN & PCNT_EN_DISABLING) == PCNT_EN_DISABLING) {}
  CMU->CLKEN1_CLR = CMU_CLKEN1_PCNT0;
  qp->state = QEI_STOP;
  qp->config = NULL;
}

void qei_lld_enable(QEIDriver *qp) {
  (void)qp;
  { volatile uint32_t sp = 4000000UL;
    while (((PCNT0->SYNCBUSY & _PCNT_SYNCBUSY_MASK) != 0U) && (--sp != 0U)) {} }
  PCNT0->CMD_SET = PCNT_CMD_STARTCNT;
}

void qei_lld_disable(QEIDriver *qp) {
  (void)qp;
  { volatile uint32_t sp = 4000000UL;
    while (((PCNT0->SYNCBUSY & _PCNT_SYNCBUSY_MASK) != 0U) && (--sp != 0U)) {} }
  PCNT0->CMD_SET = PCNT_CMD_STOPCNT;
}

int32_t qei_lld_get_count(QEIDriver *qp) {
  (void)qp;
  return (int32_t)PCNT0->CNT;
}

void qei_lld_set_count(QEIDriver *qp, int32_t count) {
  (void)qp;
  /* CNT is read-only on series 2: only reset-to-zero is possible. */
  osalDbgAssert(count == 0, "PCNT CNT can only be reset to 0 on xG23");
  { volatile uint32_t sp = 4000000UL;
    while (((PCNT0->SYNCBUSY & _PCNT_SYNCBUSY_MASK) != 0U) && (--sp != 0U)) {} }
  PCNT0->CMD_SET = PCNT_CMD_CNTRST;
}

void qei_lld_serve_interrupt(QEIDriver *qp) {
  uint32_t isr = PCNT0->IF;
  PCNT0->IF_CLR = isr;
  if (qp->config && qp->config->callback)
    qp->config->callback(qp);
}

#endif /* HAL_USE_QEI */
/** @} */
