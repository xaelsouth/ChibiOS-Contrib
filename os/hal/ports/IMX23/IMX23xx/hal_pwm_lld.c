/*
    ChibiOS - Copyright (C) 2026 Xael South.

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
 * @file    IMX23/IMX23xx/hal_pwm_lld.c
 * @brief   i.MX23 PWM subsystem low level driver (5 channels on one block).
 *
 * @addtogroup IMX23
 * @{
 */

#include "hal.h"

#if (HAL_USE_PWM == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* Divider shift for CDIV values 0..7: clock = 24MHz >> shift. */
const uint8_t imx23_pwm_cdiv_shift[8] = { 0, 1, 2, 3, 4, 6, 8, 10 };

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

PWMDriver PWMD1 = {
  .state    = PWM_STOP,
  .config   = NULL,
  .pwm      = IMX23_PWM,
  .channels = IMX23_PWM_CHANNELS,
};

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static uint32_t pwm_polarity_bits(pwmmode_t mode) {

  switch (mode & PWM_OUTPUT_MASK) {
  case PWM_OUTPUT_ACTIVE_LOW:
    return IMX23_PWM_PERIOD_POLARITY_INVERSE;
  case PWM_OUTPUT_ACTIVE_HIGH:
  default:
    return IMX23_PWM_PERIOD_POLARITY_NORMAL;
  }
}

/* Write PERIODn of all configured channels (ACTIVE first per RM). */
static void pwm_write_periods(PWMDriver *pwmp, pwmcnt_t period) {
  unsigned i;
  uint32_t timer_hz = pwmp->timer_hz;
  uint32_t freq = pwmp->config->frequency;
  uint32_t cycles;

  /* Rescale period ticks from the requested timer clock to the real one. */
  cycles = ((uint64_t)period * timer_hz + freq / 2U) / freq;
  if (cycles == 0U) {
    cycles = 1U;
  }
  if (cycles > 0xFFFFU) {
    cycles = 0xFFFFU;
  }

  for (i = 0U; i < pwmp->channels; i++) {
    if (pwmp->config->channels[i].mode != PWM_OUTPUT_DISABLED) {
      uint32_t pol = pwm_polarity_bits(pwmp->config->channels[i].mode);
      IMX23_PWM_PERIODREG(pwmp->pwm, i) =
          IMX23_PWM_PERIOD_PERIOD(cycles) | pol | IMX23_PWM_PERIOD_CDIV(pwmp->cdiv);
    }
  }
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void pwm_lld_init(void) {

  /* Ungate the fixed 24MHz PWM clock and release the block from reset. */
  *(volatile uint32_t *)IMX23_CLKCTRL_XTAL &= ~IMX23_CLKCTRL_XTAL_PWM_CLK24M_GATE;
  IMX23_PWM->CTRL_CLR = IMX23_PWM_CTRL_SFTRST | IMX23_PWM_CTRL_CLKGATE;

  pwmObjectInit(&PWMD1);
}

void pwm_lld_start(PWMDriver *pwmp) {
  unsigned i;
  uint32_t freq = pwmp->config->frequency;
  uint32_t best = 0U;
  uint64_t best_err = UINT64_MAX;

  osalDbgAssert((pwmp->state == PWM_STOP) || (pwmp->state == PWM_READY),
                "invalid state");

  if (pwmp->state == PWM_STOP) {
    /* Pick the divider whose resulting clock is closest to the request. */
    for (i = 0U; i < 8U; i++) {
      uint32_t clk = IMX23_PWM_CLK_HZ >> imx23_pwm_cdiv_shift[i];
      uint64_t err = (clk > freq) ? (uint64_t)(clk - freq)
                                  : (uint64_t)(freq - clk);
      if (err < best_err) {
        best_err = err;
        best = i;
      }
    }
    pwmp->cdiv = (uint8_t)best;
    pwmp->timer_hz = IMX23_PWM_CLK_HZ >> imx23_pwm_cdiv_shift[best];

    /* Make sure the block is out of reset and clocked. */
    IMX23_PWM->CTRL_CLR = IMX23_PWM_CTRL_SFTRST | IMX23_PWM_CTRL_CLKGATE;

    /* Program all configured channels (ACTIVE first, then PERIOD). */
    for (i = 0U; i < pwmp->channels; i++) {
      if (pwmp->config->channels[i].mode != PWM_OUTPUT_DISABLED) {
        IMX23_PWM_ACTIVE(pwmp->pwm, i) = 0U; /* ACTIVE=0, INACTIVE=0 */
      }
    }
    pwm_write_periods(pwmp, pwmp->config->period);
  }
}

void pwm_lld_stop(PWMDriver *pwmp) {

  osalDbgAssert((pwmp->state == PWM_STOP) || (pwmp->state == PWM_READY),
                "invalid state");

  if (pwmp->state == PWM_READY) {
    IMX23_PWM->CTRL_CLR = 0x1FU; /* disable all 5 channels */
  }
}

void pwm_lld_enable_channel(PWMDriver *pwmp, pwmchannel_t channel,
                            pwmcnt_t width) {
  uint32_t cycles;

  osalDbgAssert((pwmp->state == PWM_READY), "invalid state");
  osalDbgAssert(channel < pwmp->channels, "invalid channel");
  osalDbgAssert(width <= pwmp->period, "invalid width");

  /* Rescale the width to hardware cycles of the real timer clock. */
  cycles = ((uint64_t)width * pwmp->timer_hz + pwmp->config->frequency / 2U) /
           pwmp->config->frequency;
  if (cycles > 0xFFFFU) {
    cycles = 0xFFFFU;
  }

  /* INACTIVE[31:16] = cycles high, ACTIVE[15:0] = 0 (RM order: ACTIVE first). */
  IMX23_PWM_ACTIVE(pwmp->pwm, channel) = cycles << 16;
  pwm_write_periods(pwmp, pwmp->period);

  IMX23_PWM->CTRL_SET = IMX23_PWM_CTRL_PWM_EN(channel);
}

void pwm_lld_disable_channel(PWMDriver *pwmp, pwmchannel_t channel) {

  osalDbgAssert((pwmp->state == PWM_READY), "invalid state");
  osalDbgAssert(channel < pwmp->channels, "invalid channel");

  IMX23_PWM->CTRL_CLR = IMX23_PWM_CTRL_PWM_EN(channel);
}

void imx23_pwm_lld_change_period(PWMDriver *pwmp, pwmcnt_t period) {

  osalDbgAssert((pwmp->state == PWM_READY), "invalid state");

  pwmp->period = period;
  pwm_write_periods(pwmp, period);
}

void pwm_lld_enable_periodic_notification(PWMDriver *pwmp) {

  (void)pwmp;
  /* No period interrupt on this hardware. */
}

void pwm_lld_disable_periodic_notification(PWMDriver *pwmp) {

  (void)pwmp;
}

void pwm_lld_enable_channel_notification(PWMDriver *pwmp,
                                         pwmchannel_t channel) {

  (void)pwmp;
  (void)channel;
}

void pwm_lld_disable_channel_notification(PWMDriver *pwmp,
                                          pwmchannel_t channel) {

  (void)pwmp;
  (void)channel;
}

#endif /* HAL_USE_PWM == TRUE */

/** @} */
