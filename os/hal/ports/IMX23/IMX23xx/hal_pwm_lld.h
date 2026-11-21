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
 * @file    IMX23/IMX23xx/hal_pwm_lld.h
 * @brief   i.MX23 PWM subsystem low level driver header.
 *
 * @addtogroup IMX23
 * @{
 */

#ifndef HAL_PWM_LLD_H
#define HAL_PWM_LLD_H

#if (HAL_USE_PWM == TRUE) || defined(__DOXYGEN__)

#include "imx23.h"

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   Number of PWM channels.
 */
#define PWM_CHANNELS                IMX23_PWM_CHANNELS

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

typedef uint32_t pwmmode_t;
typedef uint8_t  pwmchannel_t;
typedef uint32_t pwmchnmsk_t;
typedef uint16_t pwmcnt_t;

/**
 * @brief   Type of a PWM driver channel configuration structure.
 */
typedef struct {
  pwmmode_t                 mode;       /**< Channel active logic level.   */
  pwmcallback_t             callback;   /**< Channel compare callback.     */
  /* End of the mandatory fields.*/
} PWMChannelConfig;

/**
 * @brief   Type of a PWM driver configuration structure.
 * @note    @p frequency is the desired timer clock in Hz; the LLD picks the
 *          closest available divider of the fixed 24MHz PWM clock and
 *          rescales @p period accordingly.
 */
typedef struct {
  uint32_t                  frequency;  /**< Timer clock in Hz.            */
  pwmcnt_t                  period;     /**< PWM period in ticks.          */
  pwmcallback_t             callback;   /**< Periodic callback or NULL.    */
  PWMChannelConfig          channels[PWM_CHANNELS];
  /* End of the mandatory fields.*/
} PWMConfig;

/**
 * @brief   Structure representing a PWM driver.
 */
struct PWMDriver {
  pwmstate_t                state;
  const PWMConfig           *config;
  pwmcnt_t                  period;
  pwmchnmsk_t               enabled;
  pwmchannel_t              channels;
#if defined(PWM_DRIVER_EXT_FIELDS)
  PWM_DRIVER_EXT_FIELDS
#endif
  /* End of the mandatory fields.*/
  imx23_pwm_t               *pwm;       /**< Pointer to the PWM registers. */
  uint8_t                   cdiv;       /**< Selected clock divider.       */
  uint32_t                  timer_hz;   /**< Resulting timer clock in Hz.  */
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

#define pwm_lld_change_period(pwmp, period) \
  imx23_pwm_lld_change_period(pwmp, period)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

extern PWMDriver PWMD1;

#ifdef __cplusplus
extern "C" {
#endif
  void imx23_pwm_lld_change_period(PWMDriver *pwmp, pwmcnt_t period);
  void pwm_lld_init(void);
  void pwm_lld_start(PWMDriver *pwmp);
  void pwm_lld_stop(PWMDriver *pwmp);
  void pwm_lld_enable_channel(PWMDriver *pwmp, pwmchannel_t channel,
                              pwmcnt_t width);
  void pwm_lld_disable_channel(PWMDriver *pwmp, pwmchannel_t channel);
  void pwm_lld_enable_periodic_notification(PWMDriver *pwmp);
  void pwm_lld_disable_periodic_notification(PWMDriver *pwmp);
  void pwm_lld_enable_channel_notification(PWMDriver *pwmp,
                                           pwmchannel_t channel);
  void pwm_lld_disable_channel_notification(PWMDriver *pwmp,
                                            pwmchannel_t channel);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_PWM == TRUE */

#endif /* HAL_PWM_LLD_H */

/** @} */
