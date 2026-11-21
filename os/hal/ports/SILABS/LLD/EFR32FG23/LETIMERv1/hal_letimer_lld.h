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
 * @file    LETIMERv1/hal_letimer_lld.h
 * @brief   EFR32FG23 LETIMER low-level driver header (GPT-compatible).
 *
 * @details LETIMER0 is a 24-bit low-energy timer clocked from EM23GRPACLK
 *          (LFXO/LFRCO/ULFRCO). It can be used either as the system tick
 *          (via SYSTICKv3) or as a standalone GPT timer (via this driver).
 *          Use EFR32_LETIMER_GPT_USE_LETIMER0 to claim it for GPT.
 *
 *          Do NOT enable both SYSTICKv3 (EFR32_ST_USE_TIMER=1) and this
 *          GPT driver on LETIMER0 simultaneously — they conflict.
 *
 * @addtogroup GPT
 * @{
 */

#ifndef HAL_LETIMER_LLD_H
#define HAL_LETIMER_LLD_H

#if (HAL_USE_GPT == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

#if !defined(EFR32_LETIMER_GPT_USE_LETIMER0) || defined(__DOXYGEN__)
#define EFR32_LETIMER_GPT_USE_LETIMER0    FALSE
#endif

#if !defined(EFR32_LETIMER_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_LETIMER_IRQ_PRIORITY        4
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

#define gpt_lld_driver_fields                                               \
  LETIMER_TypeDef           *letimer;                                       \
  gptcallback_t             callback

#define gpt_lld_change_interval(gptp, interval) {                           \
  (gptp)->letimer->COMP0 = (uint32_t)(interval) - 1U;                      \
}

#define gpt_lld_get_interval(gptp) ((gptcnt_t)((gptp)->letimer->COMP0 + 1U))

#define gpt_lld_get_counter(gptp) ((gptcnt_t)((gptp)->letimer->CNT))

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if EFR32_LETIMER_GPT_USE_LETIMER0 && !defined(__DOXYGEN__)
extern GPTDriver GPTD_LETIMER0;
#define GPTD_LETIMER GPTD_LETIMER0
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

#endif /* HAL_LETIMER_LLD_H */

/** @} */
