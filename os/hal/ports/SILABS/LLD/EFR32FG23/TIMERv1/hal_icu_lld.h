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
 * @file    TIMERv1/hal_icu_lld.h
 * @brief   ICU (Input Capture Unit) LLD header for EFR32 TIMER.
 *
 * @addtogroup ICU
 * @{
 */

#ifndef HAL_ICU_LLD_H
#define HAL_ICU_LLD_H

#if (HAL_USE_ICU == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

#if !defined(EFR32_ICU_USE_TIMER0) || defined(__DOXYGEN__)
#define EFR32_ICU_USE_TIMER0    FALSE
#endif
#if !defined(EFR32_ICU_USE_TIMER1) || defined(__DOXYGEN__)
#define EFR32_ICU_USE_TIMER1    FALSE
#endif
#if !defined(EFR32_ICU_USE_TIMER2) || defined(__DOXYGEN__)
#define EFR32_ICU_USE_TIMER2    FALSE
#endif
#if !defined(EFR32_ICU_USE_TIMER3) || defined(__DOXYGEN__)
#define EFR32_ICU_USE_TIMER3    FALSE
#endif
#if !defined(EFR32_ICU_USE_TIMER4) || defined(__DOXYGEN__)
#define EFR32_ICU_USE_TIMER4    FALSE
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

typedef struct ICUDriver ICUDriver;

typedef struct {
  icucallback_t   width_cb;
  icucallback_t   period_cb;
  icucallback_t   overflow_cb;
  uint8_t         channel;     /**< CC channel 0-2. */
  uint32_t        edge;        /**< TIMER_CC_CTRL_ICEDGE_*: rising/falling/both. */
} ICUConfig;

#define _icu_driver_data                                                        \
  _base_channel_data                                                            \
  icustate_t      state;                                                        \
  const ICUConfig *config;

struct ICUDriver {
  _icu_driver_data
  TIMER_TypeDef  *timer;
  uint32_t        index;      /**< 0-4 (TIMER0-4). */
  uint32_t        clock;      /**< Timer clock frequency. */
  uint32_t        period;     /**< Last captured period (ticks). */
  uint32_t        width;      /**< Last captured width (ticks). */
  uint32_t        last_capture;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

#define icu_lld_get_width(icup)   ((icup)->width)
#define icu_lld_get_period(icup)  ((icup)->period)
#define icu_lld_are_notifications_enabled(icup) \
  (((icup)->timer->IEN & (TIMER_IEN_CC0 << (icup)->config->channel)) != 0U)
#define icu_lld_get_timer(icup)   ((icup)->timer)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if EFR32_ICU_USE_TIMER0 && !defined(__DOXYGEN__)
extern ICUDriver ICUD1;
#endif
#if EFR32_ICU_USE_TIMER1 && !defined(__DOXYGEN__)
extern ICUDriver ICUD2;
#endif
#if EFR32_ICU_USE_TIMER2 && !defined(__DOXYGEN__)
extern ICUDriver ICUD3;
#endif
#if EFR32_ICU_USE_TIMER3 && !defined(__DOXYGEN__)
extern ICUDriver ICUD4;
#endif
#if EFR32_ICU_USE_TIMER4 && !defined(__DOXYGEN__)
extern ICUDriver ICUD5;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void icu_lld_init(void);
  void icu_lld_start(ICUDriver *icup);
  void icu_lld_stop(ICUDriver *icup);
  void icu_lld_start_capture(ICUDriver *icup);
  bool icu_lld_wait_capture(ICUDriver *icup);
  void icu_lld_stop_capture(ICUDriver *icup);
  void icu_lld_enable_notifications(ICUDriver *icup);
  void icu_lld_disable_notifications(ICUDriver *icup);
  void icu_lld_serve_interrupt(ICUDriver *icup);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_ICU == TRUE */
#endif /* HAL_ICU_LLD_H */
/** @} */
