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
 * @file    TIMERv1/hal_pwm_lld.c
 * @brief   EFR32FG23 PWM subsystem low level driver source.
 *
 * @addtogroup PWM
 * @{
 */

#include "hal.h"

#if (HAL_USE_PWM == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* TIMER instances indexed 0..4. */
static TIMER_TypeDef * const efr32_timers[5] = {
  TIMER0, TIMER1, TIMER2, TIMER3, TIMER4
};

/* CMU clock enable bits for each TIMER. */
static const uint32_t efr32_timer_clken[5] = {
  CMU_CLKEN0_TIMER0, CMU_CLKEN0_TIMER1, CMU_CLKEN0_TIMER2,
  CMU_CLKEN0_TIMER3, CMU_CLKEN0_TIMER4
};

/* NVIC IRQ numbers for each TIMER. */
static const uint32_t efr32_timer_irqn[5] = {
  EFR32_TIMER1_NUMBER, EFR32_TIMER2_NUMBER, EFR32_TIMER3_NUMBER, EFR32_TIMER4_NUMBER, EFR32_TIMER5_NUMBER
};

/* IRQ priorities from mcuconf. */
static const uint32_t efr32_timer_irqprio[5] = {
  EFR32_PWM_TIMER0_IRQ_PRIORITY, EFR32_PWM_TIMER1_IRQ_PRIORITY,
  EFR32_PWM_TIMER2_IRQ_PRIORITY, EFR32_PWM_TIMER3_IRQ_PRIORITY,
  EFR32_PWM_TIMER4_IRQ_PRIORITY
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

#if EFR32_PWM_USE_TIMER0 || defined(__DOXYGEN__)
PWMDriver PWMD1;
#endif
#if EFR32_PWM_USE_TIMER1 || defined(__DOXYGEN__)
PWMDriver PWMD2;
#endif
#if EFR32_PWM_USE_TIMER2 || defined(__DOXYGEN__)
PWMDriver PWMD3;
#endif
#if EFR32_PWM_USE_TIMER3 || defined(__DOXYGEN__)
PWMDriver PWMD4;
#endif
#if EFR32_PWM_USE_TIMER4 || defined(__DOXYGEN__)
PWMDriver PWMD5;
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static PWMDriver *pwm_lld_get_driver(uint32_t index) {
  switch (index) {
#if EFR32_PWM_USE_TIMER0
  case 0: return &PWMD1;
#endif
#if EFR32_PWM_USE_TIMER1
  case 1: return &PWMD2;
#endif
#if EFR32_PWM_USE_TIMER2
  case 2: return &PWMD3;
#endif
#if EFR32_PWM_USE_TIMER3
  case 3: return &PWMD4;
#endif
#if EFR32_PWM_USE_TIMER4
  case 4: return &PWMD5;
#endif
  default: return NULL;
  }
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/
/* TIMER IRQ handlers are shared with the GPT driver and defined in
   hal_timer_irq.c to avoid duplicate symbol definitions. */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void pwm_lld_init(void) {
#if EFR32_PWM_USE_TIMER0
  pwmObjectInit(&PWMD1);
  PWMD1.timer = TIMER0;
  PWMD1.index = 0;
#endif
#if EFR32_PWM_USE_TIMER1
  pwmObjectInit(&PWMD2);
  PWMD2.timer = TIMER1;
  PWMD2.index = 1;
#endif
#if EFR32_PWM_USE_TIMER2
  pwmObjectInit(&PWMD3);
  PWMD3.timer = TIMER2;
  PWMD3.index = 2;
#endif
#if EFR32_PWM_USE_TIMER3
  pwmObjectInit(&PWMD4);
  PWMD4.timer = TIMER3;
  PWMD4.index = 3;
#endif
#if EFR32_PWM_USE_TIMER4
  pwmObjectInit(&PWMD5);
  PWMD5.timer = TIMER4;
  PWMD5.index = 4;
#endif
}

void pwm_lld_start(PWMDriver *pwmp) {
  const PWMConfig *config = pwmp->config;
  TIMER_TypeDef *tim = pwmp->timer;
  uint32_t index = pwmp->index;

  pwmp->channels = PWM_CHANNELS;
  pwmp->clock = EFR32_EM01GRPACLK;

  if (pwmp->state == PWM_STOP) {
    /* Enable clock. */
    CMU->CLKEN0_SET = efr32_timer_clken[index];

    /* Prescaler from frequency (fall back to the prescaler field if set),
       clock EM01GRPACLK, while disabled — CFG writes fault while EN=1. */
    uint32_t presc = (config->prescaler != 0U) ? config->prescaler
                                               : efr32_timer_presc_for_freq(EFR32_EM01GRPACLK, config->frequency);
    tim->CFG = ((((1U << presc) - 1U) << _TIMER_CFG_PRESC_SHIFT) & _TIMER_CFG_PRESC_MASK) |
               _TIMER_CFG_CLKSEL_PRESCEM01GRPACLK;

    /* CCx_CFG is writable only while EN=0 on FG23. */
    for (uint32_t ch = 0; ch < PWM_CHANNELS; ch++) {
      /* PWM mode in CC_CFG. */
      tim->CC[ch].CFG = (tim->CC[ch].CFG & ~_TIMER_CC_CFG_MODE_MASK) |
                        _TIMER_CC_CFG_MODE_PWM;
    }

    /* Enable the module: CCx_CTRL / CCx_OC bus-fault while EN=0. */
    tim->EN_SET = TIMER_EN_EN;
    while ((tim->STATUS & _TIMER_STATUS_SYNCBUSY_MASK) != 0U) {}

    for (uint32_t ch = 0; ch < PWM_CHANNELS; ch++) {
      tim->CC[ch].CTRL = (tim->CC[ch].CTRL &
                          ~(_TIMER_CC_CTRL_COFOA_MASK |
                            _TIMER_CC_CTRL_CMOA_MASK |
                            _TIMER_CC_CTRL_CUFOA_MASK |
                            TIMER_CC_CTRL_OUTINV)) |
                         ((config->channels[ch].mode == PWM_OUTPUT_ACTIVE_LOW) ?
                          TIMER_CC_CTRL_OUTINV : 0U);
      /* Initial duty cycle 0 (channels are enabled via pwmEnableChannel). */
      tim->CC[ch].OC = 0U;
    }

    /* Set period (TOP). */
    tim->TOP = config->period - 1U;
    pwmp->period = config->period;

    /* Clear interrupt flags, start counter. */
    tim->IF_CLR = _TIMER_IF_MASK;
    tim->CMD_SET = TIMER_CMD_START;

    /* Enable NVIC vector. */
    nvicEnableVector(efr32_timer_irqn[index], efr32_timer_irqprio[index]);
  }
}

void pwm_lld_stop(PWMDriver *pwmp) {
  TIMER_TypeDef *tim = pwmp->timer;
  uint32_t index = pwmp->index;

  if (pwmp->state == PWM_READY) {
    tim->CMD_SET = TIMER_CMD_STOP;
    tim->IEN = 0;
    tim->IF_CLR = _TIMER_IF_MASK;
    nvicDisableVector(efr32_timer_irqn[index]);
    tim->EN_CLR = TIMER_EN_EN;
    CMU->CLKEN0_CLR = efr32_timer_clken[index];
  }
}

void _pwm_lld_enable_channel(PWMDriver *pwmp, pwmchannel_t channel, pwmcnt_t width) {
  osalDbgCheck(channel < PWM_CHANNELS);
  pwmp->timer->CC[channel].OC = (uint32_t)width;
}

void _pwm_lld_disable_channel(PWMDriver *pwmp, pwmchannel_t channel) {
  osalDbgCheck(channel < PWM_CHANNELS);
  pwmp->timer->CC[channel].OC = 0U;
}

void _pwm_lld_enable_periodic_notification(PWMDriver *pwmp) {
  pwmp->timer->IEN_SET = TIMER_IEN_OF;
}

void _pwm_lld_disable_periodic_notification(PWMDriver *pwmp) {
  pwmp->timer->IEN_CLR = TIMER_IEN_OF;
}

void _pwm_lld_enable_channel_notification(PWMDriver *pwmp, pwmchannel_t channel) {
  osalDbgCheck(channel < PWM_CHANNELS);
  pwmp->timer->IEN_SET = (TIMER_IEN_CC0 << channel);
}

void _pwm_lld_disable_channel_notification(PWMDriver *pwmp, pwmchannel_t channel) {
  osalDbgCheck(channel < PWM_CHANNELS);
  pwmp->timer->IEN_CLR = (TIMER_IEN_CC0 << channel);
}

void pwm_lld_serve_interrupt(PWMDriver *pwmp) {
  TIMER_TypeDef *tim = pwmp->timer;
  uint32_t isr = tim->IF & tim->IEN;

  if ((isr & TIMER_IF_OF) != 0U) {
    tim->IF_CLR = TIMER_IF_OF;
    if (pwmp->config->callback != NULL) {
      pwmp->config->callback(pwmp);
    }
  }

  for (uint32_t ch = 0; ch < PWM_CHANNELS; ch++) {
    if ((isr & (TIMER_IF_CC0 << ch)) != 0U) {
      tim->IF_CLR = (TIMER_IF_CC0 << ch);
      if (pwmp->config->channels[ch].callback != NULL) {
        pwmp->config->channels[ch].callback(pwmp);
      }
    }
  }
}

#endif /* HAL_USE_PWM == TRUE */

/** @} */
