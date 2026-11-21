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
 * @file    TIMERv1/hal_icu_lld.c
 * @brief   ICU LLD source for EFR32 TIMER (input capture on CC channels).
 *
 * @addtogroup ICU
 * @{
 */

#include "hal.h"

#if (HAL_USE_ICU == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

static TIMER_TypeDef * const icu_timer[5] = { TIMER0, TIMER1, TIMER2, TIMER3, TIMER4 };
static const uint32_t icu_clken[5] = {
  CMU_CLKEN0_TIMER0, CMU_CLKEN0_TIMER1, CMU_CLKEN0_TIMER2,
  CMU_CLKEN0_TIMER3, CMU_CLKEN0_TIMER4
};
static const uint32_t icu_irqn[5] = {
  EFR32_TIMER1_NUMBER, EFR32_TIMER2_NUMBER, EFR32_TIMER3_NUMBER,
  EFR32_TIMER4_NUMBER, EFR32_TIMER5_NUMBER
};
static const uint32_t icu_irqprio[5] = {
  EFR32_ICU_TIMER0_IRQ_PRIORITY, EFR32_ICU_TIMER1_IRQ_PRIORITY,
  EFR32_ICU_TIMER2_IRQ_PRIORITY, EFR32_ICU_TIMER3_IRQ_PRIORITY,
  EFR32_ICU_TIMER4_IRQ_PRIORITY
};

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if EFR32_ICU_USE_TIMER0 || defined(__DOXYGEN__)
ICUDriver ICUD1;
#endif
#if EFR32_ICU_USE_TIMER1 || defined(__DOXYGEN__)
ICUDriver ICUD2;
#endif
#if EFR32_ICU_USE_TIMER2 || defined(__DOXYGEN__)
ICUDriver ICUD3;
#endif
#if EFR32_ICU_USE_TIMER3 || defined(__DOXYGEN__)
ICUDriver ICUD4;
#endif
#if EFR32_ICU_USE_TIMER4 || defined(__DOXYGEN__)
ICUDriver ICUD5;
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void icu_lld_init(void) {
#if EFR32_ICU_USE_TIMER0
  ICUD1.state = ICU_STOP; ICUD1.timer = TIMER0; ICUD1.index = 0;
  ICUD1.clock = EFR32_EM01GRPACLK;
#endif
#if EFR32_ICU_USE_TIMER1
  ICUD2.state = ICU_STOP; ICUD2.timer = TIMER1; ICUD2.index = 1;
  ICUD2.clock = EFR32_EM01GRPACLK;
#endif
#if EFR32_ICU_USE_TIMER2
  ICUD3.state = ICU_STOP; ICUD3.timer = TIMER2; ICUD3.index = 2;
  ICUD3.clock = EFR32_EM01GRPACLK;
#endif
#if EFR32_ICU_USE_TIMER3
  ICUD4.state = ICU_STOP; ICUD4.timer = TIMER3; ICUD4.index = 3;
  ICUD4.clock = EFR32_EM01GRPACLK;
#endif
#if EFR32_ICU_USE_TIMER4
  ICUD5.state = ICU_STOP; ICUD5.timer = TIMER4; ICUD5.index = 4;
  ICUD5.clock = EFR32_EM01GRPACLK;
#endif
}

void icu_lld_start(ICUDriver *icup) {
  TIMER_TypeDef *tim = icup->timer;
  if (icup->state == ICU_STOP) {
    CMU->CLKEN0_SET = icu_clken[icup->index];
    /* Up-count, clock from EM01GRPACLK, prescaler DIV1. */
    tim->CFG = _TIMER_CFG_CLKSEL_PRESCEM01GRPACLK |
               (0U << _TIMER_CFG_PRESC_SHIFT);
    tim->TOP = 0xFFFFU;  /* 16-bit max for FG23 TIMER (or 32-bit if wide). */
    tim->CNT = 0;
    tim->EN_SET = TIMER_EN_EN;
    while ((tim->SYNCBUSY & _TIMER_SYNCBUSY_MASK) != 0U) {}
    tim->CMD_SET = TIMER_CMD_START;
  }
}

void icu_lld_stop(ICUDriver *icup) {
  TIMER_TypeDef *tim = icup->timer;
  if (icup->state == ICU_READY || icup->state == ICU_ACTIVE) {
    nvicDisableVector(icu_irqn[icup->index]);
    tim->CMD_SET = TIMER_CMD_STOP;
    tim->IEN_CLR = _TIMER_IEN_MASK;
    tim->IF_CLR = tim->IF;
    tim->EN_CLR = TIMER_EN_EN;
    while ((tim->EN & TIMER_EN_DISABLING) == TIMER_EN_DISABLING) {}
    CMU->CLKEN0_CLR = icu_clken[icup->index];
  }
}

void icu_lld_start_capture(ICUDriver *icup) {
  TIMER_TypeDef *tim = icup->timer;
  uint8_t ch = icup->config->channel;

  /* Configure CC channel for input capture. */
  tim->CC[ch].CFG = TIMER_CC_CFG_MODE_INPUTCAPTURE;
  tim->CC[ch].CTRL =
                     icup->config->edge |
                     TIMER_CC_CTRL_ICEVCTRL_EVERYEDGE;
  tim->IF_CLR = tim->IF;
  tim->IEN_SET = (TIMER_IEN_CC0 << ch) | TIMER_IEN_OF;
  nvicEnableVector(icu_irqn[icup->index], icu_irqprio[icup->index]);
  icup->last_capture = 0;
  icup->width = 0;
  icup->period = 0;
}

bool icu_lld_wait_capture(ICUDriver *icup) {
  /* In ChibiOS this is typically a blocking wait; the core calls it from
     icuWaitCapture which uses a semaphore. We just return true if a capture
     has completed (state == ICU_ACTIVE). */
  return (icup->state == ICU_ACTIVE);
}

void icu_lld_stop_capture(ICUDriver *icup) {
  TIMER_TypeDef *tim = icup->timer;
  uint8_t ch = icup->config->channel;
  tim->IEN_CLR = (TIMER_IEN_CC0 << ch);
  tim->CC[ch].CFG = TIMER_CC_CFG_MODE_OFF;
}

void icu_lld_enable_notifications(ICUDriver *icup) {
  uint8_t ch = icup->config->channel;
  icup->timer->IEN_SET = (TIMER_IEN_CC0 << ch);
}

void icu_lld_disable_notifications(ICUDriver *icup) {
  uint8_t ch = icup->config->channel;
  icup->timer->IEN_CLR = (TIMER_IEN_CC0 << ch);
}

void icu_lld_serve_interrupt(ICUDriver *icup) {
  TIMER_TypeDef *tim = icup->timer;
  uint8_t ch = icup->config->channel;
  uint32_t isr = tim->IF;
  tim->IF_CLR = isr;

  if (isr & (TIMER_IF_CC0 << ch)) {
    uint32_t cap = tim->CC[ch].ICF;
    if (icup->state == ICU_WAITING) {
      /* First edge: record as start of period. */
      icup->last_capture = cap;
      _icu_isr_invoke_period_cb(icup);
    } else {
      /* Subsequent edge: compute period and width. */
      uint32_t diff = cap - icup->last_capture;
      icup->period = diff;
      icup->width = diff;  /* simplified: same edge type */
      icup->last_capture = cap;
      _icu_isr_invoke_width_cb(icup);
      _icu_isr_invoke_period_cb(icup);
    }
  }

  if (isr & TIMER_IF_OF) {
    _icu_isr_invoke_overflow_cb(icup);
  }
}

#endif /* HAL_USE_ICU == TRUE */
/** @} */
