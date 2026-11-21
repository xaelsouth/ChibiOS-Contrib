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
 * @file    TIMERv1/hal_pwm_lld.h
 * @brief   EFR32FG23 PWM subsystem low level driver header.
 *
 * @addtogroup PWM
 * @{
 */

#ifndef HAL_PWM_LLD_H
#define HAL_PWM_LLD_H

#if (HAL_USE_PWM == TRUE) || defined(__DOXYGEN__)

#if !defined(PWMCNT_T_TYPE_DEFINED)
#define PWMCNT_T_TYPE_DEFINED
typedef uint32_t pwmcnt_t;
#endif

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Number of PWM channels per TIMER instance.
 */
#define PWM_CHANNELS            3U

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
#if !defined(EFR32_PWM_USE_TIMER0) || defined(__DOXYGEN__)
#define EFR32_PWM_USE_TIMER0    FALSE
#endif
#if !defined(EFR32_PWM_USE_TIMER1) || defined(__DOXYGEN__)
#define EFR32_PWM_USE_TIMER1    FALSE
#endif
#if !defined(EFR32_PWM_USE_TIMER2) || defined(__DOXYGEN__)
#define EFR32_PWM_USE_TIMER2    FALSE
#endif
#if !defined(EFR32_PWM_USE_TIMER3) || defined(__DOXYGEN__)
#define EFR32_PWM_USE_TIMER3    FALSE
#endif
#if !defined(EFR32_PWM_USE_TIMER4) || defined(__DOXYGEN__)
#define EFR32_PWM_USE_TIMER4    FALSE
#endif

#if !defined(EFR32_PWM_TIMER0_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_PWM_TIMER0_IRQ_PRIORITY   4
#endif
#if !defined(EFR32_PWM_TIMER1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_PWM_TIMER1_IRQ_PRIORITY   4
#endif
#if !defined(EFR32_PWM_TIMER2_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_PWM_TIMER2_IRQ_PRIORITY   4
#endif
#if !defined(EFR32_PWM_TIMER3_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_PWM_TIMER3_IRQ_PRIORITY   4
#endif
#if !defined(EFR32_PWM_TIMER4_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_PWM_TIMER4_IRQ_PRIORITY   4
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a PWM channel.
 */
typedef uint8_t pwmchannel_t;

/**
 * @brief   Type of a channels mask.
 */
typedef uint32_t pwmchnmsk_t;

/**
 * @brief   Type of a PWM output mode.
 */
typedef uint32_t pwmmode_t;

#if !defined(PWMCNT_T_TYPE_DEFINED)
#define PWMCNT_T_TYPE_DEFINED
typedef uint32_t pwmcnt_t;
#endif

/**
 * @brief   PWM driver channel configuration structure.
 */
typedef struct {
  /**
   * @brief Channel active logic level / output mode.
   */
  pwmmode_t                 mode;
  /**
   * @brief Channel callback pointer, invoked on compare match events.
   * @note  If set to @p NULL the notification is disabled.
   */
  pwmcallback_t             callback;
} PWMChannelConfig;

/**
 * @brief   PWM driver configuration structure.
 */
typedef struct {
  /**
   * @brief   Timer clock in Hz.
   */
  uint32_t                  frequency;
  /**
   * @brief   PWM period in timer ticks (TOP + 1).
   */
  pwmcnt_t                  period;
  /**
   * @brief   Periodic callback pointer (on counter overflow), or NULL.
   */
  pwmcallback_t             callback;
  /**
   * @brief   Channels configurations.
   */
  PWMChannelConfig          channels[PWM_CHANNELS];
  /**
   * @brief   Timer clock prescaler (0 = DIV1, 1 = DIV2, ...).
   */
  uint32_t                  prescaler;
} PWMConfig;

/**
 * @brief   Structure representing a PWM driver.
 */
struct PWMDriver {
  /**
   * @brief Driver state.
   */
  pwmstate_t                state;
  /**
   * @brief Current driver configuration data.
   */
  const PWMConfig           *config;
  /**
   * @brief   Current PWM period in ticks.
   */
  pwmcnt_t                  period;
  /**
   * @brief   Mask of the enabled channels.
   */
  pwmchnmsk_t               enabled;
  /**
   * @brief   Number of channels in this instance.
   */
  pwmchannel_t              channels;
  /* End of the mandatory fields.*/
  /**
   * @brief Pointer to the TIMER registers block.
   */
  TIMER_TypeDef             *timer;
  /**
   * @brief Timer index (0..4).
   */
  uint32_t                  index;
  /**
   * @brief Timer base clock in Hz.
   */
  uint32_t                  clock;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   PWM period change.
 */
#define pwm_lld_change_period(pwmp, period) {                               \
  (pwmp)->timer->TOP = (uint32_t)(period) - 1U;                             \
  (pwmp)->period = (pwmcnt_t)(period);                                     \
}

/**
 * @brief   Returns the timer pointer.
 */
#define pwm_lld_get_timer(pwmp) ((pwmp)->timer)

/**
 * @brief   Enables a PWM channel output.
 */
#define pwm_lld_enable_channel(pwmp, channel, width)                        \
  _pwm_lld_enable_channel(pwmp, channel, width)

/**
 * @brief   Disables a PWM channel output.
 */
#define pwm_lld_disable_channel(pwmp, channel)                              \
  _pwm_lld_disable_channel(pwmp, channel)

/**
 * @brief   Enables periodic (overflow) interrupt.
 */
#define pwm_lld_enable_periodic_notification(pwmp)                          \
  _pwm_lld_enable_periodic_notification(pwmp)

/**
 * @brief   Disables periodic (overflow) interrupt.
 */
#define pwm_lld_disable_periodic_notification(pwmp)                         \
  _pwm_lld_disable_periodic_notification(pwmp)

/**
 * @brief   Enables a channel compare interrupt.
 */
#define pwm_lld_enable_channel_notification(pwmp, channel)                   \
  _pwm_lld_enable_channel_notification(pwmp, channel)

/**
 * @brief   Disables a channel compare interrupt.
 */
#define pwm_lld_disable_channel_notification(pwmp, channel)                  \
  _pwm_lld_disable_channel_notification(pwmp, channel)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if EFR32_PWM_USE_TIMER0 && !defined(__DOXYGEN__)
extern PWMDriver PWMD1;
#endif
#if EFR32_PWM_USE_TIMER1 && !defined(__DOXYGEN__)
extern PWMDriver PWMD2;
#endif
#if EFR32_PWM_USE_TIMER2 && !defined(__DOXYGEN__)
extern PWMDriver PWMD3;
#endif
#if EFR32_PWM_USE_TIMER3 && !defined(__DOXYGEN__)
extern PWMDriver PWMD4;
#endif
#if EFR32_PWM_USE_TIMER4 && !defined(__DOXYGEN__)
extern PWMDriver PWMD5;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void pwm_lld_init(void);
  void pwm_lld_start(PWMDriver *pwmp);
  void pwm_lld_stop(PWMDriver *pwmp);
  void _pwm_lld_enable_channel(PWMDriver *pwmp, pwmchannel_t channel, pwmcnt_t width);
  void _pwm_lld_disable_channel(PWMDriver *pwmp, pwmchannel_t channel);
  void _pwm_lld_enable_periodic_notification(PWMDriver *pwmp);
  void _pwm_lld_disable_periodic_notification(PWMDriver *pwmp);
  void _pwm_lld_enable_channel_notification(PWMDriver *pwmp, pwmchannel_t channel);
  void _pwm_lld_disable_channel_notification(PWMDriver *pwmp, pwmchannel_t channel);
  void pwm_lld_serve_interrupt(PWMDriver *pwmp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_PWM == TRUE */

#endif /* HAL_PWM_LLD_H */

/** @} */
