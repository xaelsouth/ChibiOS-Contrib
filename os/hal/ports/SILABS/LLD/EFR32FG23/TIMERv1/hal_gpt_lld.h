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

/**
 * @brief   GPT driver structure fields.
 */
#define gpt_lld_driver_fields                                               \
  TIMER_TypeDef             *timer;                                         \
  uint32_t                  index;                                         \
  gptcallback_t             callback;                                      \
  void                      *callback_param

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

#define gpt_lld_change_interval(gptp, interval) {                           \
  (gptp)->timer->TOP = (uint32_t)(interval) - 1U;                           \
}

#define gpt_lld_get_interval(gptp) ((gptcnt_t)((gptp)->timer->TOP + 1U))

#define gpt_lld_get_counter(gptp) ((gptcnt_t)((gptp)->timer->CNT))

#define gpt_lld_get_timer(gptp) ((gptp)->timer)

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
  void gpt_lld_start(GPTDriver *gptp, const GPTConfig *config);
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
