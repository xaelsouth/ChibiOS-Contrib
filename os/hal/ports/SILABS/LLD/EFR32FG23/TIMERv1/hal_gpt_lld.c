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
 * @file    TIMERv1/hal_gpt_lld.c
 * @brief   EFR32FG23 GPT subsystem low level driver source.
 *
 * @addtogroup GPT
 * @{
 */

#include "hal.h"

#if defined(EFR32_LETIMER_GPT_USE_LETIMER0) && (EFR32_LETIMER_GPT_USE_LETIMER0 == TRUE) && !defined(__DOXYGEN__)
#include "hal_letimer_lld.h"  /* letimer_gpt_lld_* (LETIMERv1 dir is on the include path only then) */
#endif

#if (HAL_USE_GPT == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

static TIMER_TypeDef * const efr32_gpt_timers[5] = {
  TIMER0, TIMER1, TIMER2, TIMER3, TIMER4
};

static const uint32_t efr32_gpt_clken[5] = {
  CMU_CLKEN0_TIMER0, CMU_CLKEN0_TIMER1, CMU_CLKEN0_TIMER2,
  CMU_CLKEN0_TIMER3, CMU_CLKEN0_TIMER4
};

static const uint32_t efr32_gpt_irqn[5] = {
  EFR32_TIMER1_NUMBER, EFR32_TIMER2_NUMBER, EFR32_TIMER3_NUMBER, EFR32_TIMER4_NUMBER, EFR32_TIMER5_NUMBER
};

static const uint32_t efr32_gpt_irqprio[5] = {
  EFR32_GPT_TIMER0_IRQ_PRIORITY, EFR32_GPT_TIMER1_IRQ_PRIORITY,
  EFR32_GPT_TIMER2_IRQ_PRIORITY, EFR32_GPT_TIMER3_IRQ_PRIORITY,
  EFR32_GPT_TIMER4_IRQ_PRIORITY
};


/* Compute TIMER prescaler (0=DIV1 .. 9=DIV512) for the given clock/frequency. */
__STATIC_INLINE uint32_t efr32_timer_presc_for_freq(uint32_t clk, uint32_t freq) {
  uint32_t div = (clk + freq - 1U) / freq;
  uint32_t presc = 0U, d = 1U;
  if (div == 0U) div = 1U;
  while (presc < 9U && d < div) { d <<= 1U; presc++; }
  return presc;
}

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if EFR32_GPT_USE_TIMER0 || defined(__DOXYGEN__)
GPTDriver GPTD1;
#endif
#if EFR32_GPT_USE_TIMER1 || defined(__DOXYGEN__)
GPTDriver GPTD2;
#endif
#if EFR32_GPT_USE_TIMER2 || defined(__DOXYGEN__)
GPTDriver GPTD3;
#endif
#if EFR32_GPT_USE_TIMER3 || defined(__DOXYGEN__)
GPTDriver GPTD4;
#endif
#if EFR32_GPT_USE_TIMER4 || defined(__DOXYGEN__)
GPTDriver GPTD5;
#endif

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/
/* TIMER IRQ handlers are shared with the PWM driver and defined in
   hal_timer_irq.c to avoid duplicate symbol definitions. */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void gpt_lld_init(void) {
#if EFR32_GPT_USE_TIMER0
  gptObjectInit(&GPTD1);
  GPTD1.timer = TIMER0; GPTD1.index = 0;
#endif
#if EFR32_GPT_USE_TIMER1
  gptObjectInit(&GPTD2);
  GPTD2.timer = TIMER1; GPTD2.index = 1;
#endif
#if EFR32_GPT_USE_TIMER2
  gptObjectInit(&GPTD3);
  GPTD3.timer = TIMER2; GPTD3.index = 2;
#endif
#if EFR32_GPT_USE_TIMER3
  gptObjectInit(&GPTD4);
  GPTD4.timer = TIMER3; GPTD4.index = 3;
#endif
#if EFR32_GPT_USE_TIMER4
  gptObjectInit(&GPTD5);
  GPTD5.timer = TIMER4; GPTD5.index = 4;
#endif
#if defined(EFR32_LETIMER_GPT_USE_LETIMER0) && (EFR32_LETIMER_GPT_USE_LETIMER0 == TRUE) && !defined(__DOXYGEN__)
  letimer_gpt_lld_init();
#endif
}

void gpt_lld_start(GPTDriver *gptp) {
  const GPTConfig *config = gptp->config;
#if defined(EFR32_LETIMER_GPT_USE_LETIMER0) && (EFR32_LETIMER_GPT_USE_LETIMER0 == TRUE) && !defined(__DOXYGEN__)
  if (gptp->letimer != NULL) {
    letimer_gpt_lld_start(gptp);
    return;
  }
#endif

  TIMER_TypeDef *tim = gptp->timer;

  if (gptp->state == GPT_STOP) {
    CMU->CLKEN0_SET = efr32_gpt_clken[gptp->index];
    /* Prescaler from the requested frequency, clock EM01GRPACLK (while EN=0). */
    uint32_t presc = efr32_timer_presc_for_freq(EFR32_EM01GRPACLK, config->frequency);
    tim->CFG = ((((1U << presc) - 1U) << _TIMER_CFG_PRESC_SHIFT) & _TIMER_CFG_PRESC_MASK) |
               _TIMER_CFG_CLKSEL_PRESCEM01GRPACLK;
    tim->EN_SET = TIMER_EN_EN;
    while ((tim->STATUS & _TIMER_STATUS_SYNCBUSY_MASK) != 0U) {}
    tim->IF_CLR = _TIMER_IF_MASK;
    tim->IEN = 0;
    nvicEnableVector(efr32_gpt_irqn[gptp->index], efr32_gpt_irqprio[gptp->index]);
  }
}

void gpt_lld_stop(GPTDriver *gptp) {
#if defined(EFR32_LETIMER_GPT_USE_LETIMER0) && (EFR32_LETIMER_GPT_USE_LETIMER0 == TRUE) && !defined(__DOXYGEN__)
  if (gptp->letimer != NULL) {
    letimer_gpt_lld_stop(gptp);
    return;
  }
#endif

  TIMER_TypeDef *tim = gptp->timer;

  if (gptp->state == GPT_READY) {
    tim->CMD_SET = TIMER_CMD_STOP;
    tim->IEN = 0;
    tim->IF_CLR = _TIMER_IF_MASK;
    nvicDisableVector(efr32_gpt_irqn[gptp->index]);
    tim->EN_CLR = TIMER_EN_EN;
    CMU->CLKEN0_CLR = efr32_gpt_clken[gptp->index];
  }
}

void gpt_lld_start_timer(GPTDriver *gptp, gptcnt_t period) {
#if defined(EFR32_LETIMER_GPT_USE_LETIMER0) && (EFR32_LETIMER_GPT_USE_LETIMER0 == TRUE) && !defined(__DOXYGEN__)
  if (gptp->letimer != NULL) {
    letimer_gpt_lld_start_timer(gptp, period);
    return;
  }
#endif

  TIMER_TypeDef *tim = gptp->timer;

  tim->TOP = (uint32_t)period - 1U;
  tim->CNT = 0;
  tim->IF_CLR = _TIMER_IF_MASK;
  tim->IEN_SET = TIMER_IEN_OF;
  tim->CMD_SET = TIMER_CMD_START;
}

void gpt_lld_stop_timer(GPTDriver *gptp) {
#if defined(EFR32_LETIMER_GPT_USE_LETIMER0) && (EFR32_LETIMER_GPT_USE_LETIMER0 == TRUE) && !defined(__DOXYGEN__)
  if (gptp->letimer != NULL) {
    letimer_gpt_lld_stop_timer(gptp);
    return;
  }
#endif

  TIMER_TypeDef *tim = gptp->timer;

  tim->CMD_SET = TIMER_CMD_STOP;
  tim->IEN_CLR = TIMER_IEN_OF;
  tim->IF_CLR = TIMER_IF_OF;
}

void gpt_lld_polled_delay(GPTDriver *gptp, gptcnt_t interval) {
#if defined(EFR32_LETIMER_GPT_USE_LETIMER0) && (EFR32_LETIMER_GPT_USE_LETIMER0 == TRUE) && !defined(__DOXYGEN__)
  if (gptp->letimer != NULL) {
    letimer_gpt_lld_polled_delay(gptp, interval);
    return;
  }
#endif

  TIMER_TypeDef *tim = gptp->timer;

  tim->TOP = (uint32_t)interval - 1U;
  tim->CNT = 0;
  tim->IF_CLR = TIMER_IF_OF;
  tim->CMD_SET = TIMER_CMD_START;
  while ((tim->IF & TIMER_IF_OF) == 0U) {
  }
  tim->CMD_SET = TIMER_CMD_STOP;
  tim->IF_CLR = TIMER_IF_OF;
}

void gpt_lld_serve_interrupt(GPTDriver *gptp) {
#if defined(EFR32_LETIMER_GPT_USE_LETIMER0) && (EFR32_LETIMER_GPT_USE_LETIMER0 == TRUE) && !defined(__DOXYGEN__)
  if (gptp->letimer != NULL) {
    letimer_gpt_lld_serve_interrupt(gptp);
    return;
  }
#endif

  TIMER_TypeDef *tim = gptp->timer;

  if ((tim->IF & TIMER_IF_OF) != 0U) {
    tim->IF_CLR = TIMER_IF_OF;
    _gpt_isr_invoke_cb(gptp);
  }
}

#endif /* HAL_USE_GPT == TRUE */

/** @} */
