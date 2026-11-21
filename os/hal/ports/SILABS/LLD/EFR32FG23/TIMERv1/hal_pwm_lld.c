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
  TIMER0_IRQn, TIMER1_IRQn, TIMER2_IRQn, TIMER3_IRQn, TIMER4_IRQn
};

/* IRQ priorities from mcuconf. */
static const uint32_t efr32_timer_irqprio[5] = {
  EFR32_PWM_TIMER0_IRQ_PRIORITY, EFR32_PWM_TIMER1_IRQ_PRIORITY,
  EFR32_PWM_TIMER2_IRQ_PRIORITY, EFR32_PWM_TIMER3_IRQ_PRIORITY,
  EFR32_PWM_TIMER4_IRQ_PRIORITY
};

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

void pwm_lld_start(PWMDriver *pwmp, const PWMConfig *config) {
  TIMER_TypeDef *tim = pwmp->timer;
  uint32_t index = pwmp->index;

  if (pwmp->state == PWM_STOP) {
    /* Enable clock. */
    CMU->CLKEN0_SET = efr32_timer_clken[index];

    /* Enable timer module. */
    tim->EN_SET = TIMER_EN_EN;

    /* Configure prescaler and clock source (EM01GRPACLK). */
    tim->CFG = (tim->CFG & ~_TIMER_CFG_PRESC_MASK) |
               ((config->prescaler & 0xFFU) << _TIMER_CFG_PRESC_SHIFT) |
               _TIMER_CFG_CLKSEL_PRESCEM01GRPACLK;

    /* Set period (TOP). */
    tim->TOP = config->period - 1U;
    pwmp->period = config->period;

    /* Configure all 3 CC channels for PWM mode. */
    for (uint32_t ch = 0; ch < PWM_CHANNELS; ch++) {
      /* PWM mode in CC_CFG. */
      tim->CC[ch].CFG = (tim->CC[ch].CFG & ~_TIMER_CC_CFG_MODE_MASK) |
                        _TIMER_CC_CFG_MODE_PWM;
      /* On overflow: set output. On compare match: clear output.
         This gives active-high PWM with duty = width/period. */
      tim->CC[ch].CTRL = (tim->CC[ch].CTRL &
                          ~(_TIMER_CC_CTRL_COFOA_MASK |
                            _TIMER_CC_CTRL_CMOA_MASK |
                            _TIMER_CC_CTRL_CUFOA_MASK)) |
                         (_TIMER_CC_CTRL_COFOA_SET << _TIMER_CC_CTRL_COFOA_SHIFT) |
                         (_TIMER_CC_CTRL_CMOA_CLEAR << _TIMER_CC_CTRL_CMOA_SHIFT);
      /* Set initial duty cycle. */
      tim->CC[ch].OC = config->channels[ch];
    }

    /* Clear interrupt flags, start counter. */
    tim->IF_CLR = _TIMER_IF_MASK;
    tim->CMD_SET = TIMER_CMD_START;

    /* Enable NVIC vector. */
    nvicEnableVector(efr32_timer_irqn[index], efr32_timer_irqprio[index]);
  }

  pwmp->callback = config->callback;
  pwmp->callback_param = config->callback_param;
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
  pwmp->timer->CC[channel].CFG =
    (pwmp->timer->CC[channel].CFG & ~_TIMER_CC_CFG_MODE_MASK) |
    _TIMER_CC_CFG_MODE_OFF;
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
    if (pwmp->callback != NULL) {
      pwmp->callback(pwmp, pwmp->callback_param);
    }
  }

  for (uint32_t ch = 0; ch < PWM_CHANNELS; ch++) {
    if ((isr & (TIMER_IF_CC0 << ch)) != 0U) {
      tim->IF_CLR = (TIMER_IF_CC0 << ch);
    }
  }
}

#endif /* HAL_USE_PWM == TRUE */

/** @} */
