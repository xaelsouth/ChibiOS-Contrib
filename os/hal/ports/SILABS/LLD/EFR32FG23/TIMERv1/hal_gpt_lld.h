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
 * @file    TIMERv1/hal_gpt_lld.h
 * @brief   EFR32FG23 GPT subsystem low level driver header.
 *
 * @addtogroup GPT
 * @{
 */

#ifndef HAL_GPT_LLD_H
#define HAL_GPT_LLD_H

#if (HAL_USE_GPT == TRUE) || defined(__DOXYGEN__)

#if !defined(GPTCNT_T_TYPE_DEFINED)
#define GPTCNT_T_TYPE_DEFINED
typedef uint32_t gptcnt_t;
#endif

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

#if !defined(EFR32_GPT_USE_TIMER0) || defined(__DOXYGEN__)
#define EFR32_GPT_USE_TIMER0    FALSE
#endif
#if !defined(EFR32_GPT_USE_TIMER1) || defined(__DOXYGEN__)
#define EFR32_GPT_USE_TIMER1    FALSE
#endif
#if !defined(EFR32_GPT_USE_TIMER2) || defined(__DOXYGEN__)
#define EFR32_GPT_USE_TIMER2    FALSE
#endif
#if !defined(EFR32_GPT_USE_TIMER3) || defined(__DOXYGEN__)
#define EFR32_GPT_USE_TIMER3    FALSE
#endif
#if !defined(EFR32_GPT_USE_TIMER4) || defined(__DOXYGEN__)
#define EFR32_GPT_USE_TIMER4    FALSE
#endif

#if !defined(EFR32_GPT_TIMER0_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_GPT_TIMER0_IRQ_PRIORITY   4
#endif
#if !defined(EFR32_GPT_TIMER1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_GPT_TIMER1_IRQ_PRIORITY   4
#endif
#if !defined(EFR32_GPT_TIMER2_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_GPT_TIMER2_IRQ_PRIORITY   4
#endif
#if !defined(EFR32_GPT_TIMER3_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_GPT_TIMER3_IRQ_PRIORITY   4
#endif
#if !defined(EFR32_GPT_TIMER4_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_GPT_TIMER4_IRQ_PRIORITY   4
#endif
/** @} */

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

#if !defined(GPTFREQ_T_TYPE_DEFINED)
#define GPTFREQ_T_TYPE_DEFINED
typedef uint32_t gptfreq_t;
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/* Definitions shared with LETIMERv1 (which provides a GPTDriver on top of
   LETIMER0). Guarded so both LLD headers can be included in one build. */
#if !defined(EFR32_GPT_LLD_SHARED_DEFINED)
#define EFR32_GPT_LLD_SHARED_DEFINED

/**
 * @brief   GPT driver structure fields.
 * @note    Superset of TIMERv1 and LETIMERv1 fields: exactly one of
 *          @p timer / @p letimer is non-NULL, dispatching is done at
 *          run time.
 */
#define gpt_lld_driver_fields                                               \
  TIMER_TypeDef             *timer;      /* TIMERv1, NULL for LETIMER */   \
  LETIMER_TypeDef           *letimer;    /* LETIMERv1, NULL for TIMER */   \
  uint32_t                  index;                                         \
  uint32_t                  clock;


/**
 * @brief   Driver configuration structure.
 */
typedef struct {
  /**
   * @brief   Timer clock in Hz.
   */
  gptfreq_t                 frequency;
  /**
   * @brief   Timer callback pointer.
   */
  gptcallback_t             callback;
  /**
   * @brief   Callback argument.
   */
  void                      *callback_param;
} GPTConfig;

/**
 * @brief   Structure representing a GPT driver.
 */
struct GPTDriver {
  gptstate_t                state;
  const GPTConfig           *config;
  gpt_lld_driver_fields
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

#define gpt_lld_change_interval(gptp, interval) {                           \
  if ((gptp)->letimer != NULL) {                                            \
    (gptp)->letimer->COMP0 = (uint32_t)(interval) - 1U;                     \
  }                                                                         \
  else {                                                                    \
    (gptp)->timer->TOP = (uint32_t)(interval) - 1U;                         \
  }                                                                         \
}

#define gpt_lld_get_interval(gptp)                                          \
  ((gptp)->letimer != NULL ?                                                \
   ((gptcnt_t)((gptp)->letimer->COMP0 + 1U)) :                              \
   ((gptcnt_t)((gptp)->timer->TOP + 1U)))

#define gpt_lld_get_counter(gptp)                                           \
  ((gptp)->letimer != NULL ?                                                \
   ((gptcnt_t)((gptp)->letimer->CNT)) :                                     \
   ((gptcnt_t)((gptp)->timer->CNT)))

#define gpt_lld_get_timer(gptp)                                             \
  ((gptp)->letimer != NULL ? (void *)(gptp)->letimer : (void *)(gptp)->timer)

#endif /* EFR32_GPT_LLD_SHARED_DEFINED */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if EFR32_GPT_USE_TIMER0 && !defined(__DOXYGEN__)
extern GPTDriver GPTD1;
#endif
#if EFR32_GPT_USE_TIMER1 && !defined(__DOXYGEN__)
extern GPTDriver GPTD2;
#endif
#if EFR32_GPT_USE_TIMER2 && !defined(__DOXYGEN__)
extern GPTDriver GPTD3;
#endif
#if EFR32_GPT_USE_TIMER3 && !defined(__DOXYGEN__)
extern GPTDriver GPTD4;
#endif
#if EFR32_GPT_USE_TIMER4 && !defined(__DOXYGEN__)
extern GPTDriver GPTD5;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void gpt_lld_init(void);
  void gpt_lld_start(GPTDriver *gptp);
  void gpt_lld_stop(GPTDriver *gptp);
  void gpt_lld_start_timer(GPTDriver *gptp, gptcnt_t period);
  void gpt_lld_stop_timer(GPTDriver *gptp);
  void gpt_lld_polled_delay(GPTDriver *gptp, gptcnt_t interval);
  void gpt_lld_serve_interrupt(GPTDriver *gptp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_GPT == TRUE */

#endif /* HAL_GPT_LLD_H */

/** @} */
