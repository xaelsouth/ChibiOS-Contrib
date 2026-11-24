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
 *          (LFXO/LFRCO/ULFRCO). On FG23 it is NOT used as the system tick
 *          (SYSTICKv2/SYSRTC is the default), so this driver can use it as
 *          a standalone GPT timer. Claim it with
 *          EFR32_LETIMER_GPT_USE_LETIMER0=TRUE in mcuconf.h.
 *
 * @addtogroup GPT
 * @{
 */

#ifndef HAL_LETIMER_LLD_H
#define HAL_LETIMER_LLD_H

#if (HAL_USE_GPT == TRUE) || defined(__DOXYGEN__)

/* Shared GPTDriver / GPTConfig / gpt_lld_driver_fields / macros come from
   TIMERv1/hal_gpt_lld.h (always on the include path). The struct is a
   superset holding both ->timer and ->letimer pointers. */
#include "hal_gpt_lld.h"

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
/* External declarations.                                                    */
/*===========================================================================*/

#if EFR32_LETIMER_GPT_USE_LETIMER0 && !defined(__DOXYGEN__)
extern GPTDriver GPTD_LETIMER0;
#define GPTD_LETIMER GPTD_LETIMER0
#endif

#ifdef __cplusplus
extern "C" {
#endif
  /* LETIMER-specific implementations, dispatched from the generic
     gpt_lld_* functions in TIMERv1/hal_gpt_lld.c when gptp->letimer != NULL. */
  void letimer_gpt_lld_init(void);
  void letimer_gpt_lld_start(GPTDriver *gptp);
  void letimer_gpt_lld_stop(GPTDriver *gptp);
  void letimer_gpt_lld_start_timer(GPTDriver *gptp, gptcnt_t period);
  void letimer_gpt_lld_stop_timer(GPTDriver *gptp);
  void letimer_gpt_lld_polled_delay(GPTDriver *gptp, gptcnt_t interval);
  void letimer_gpt_lld_serve_interrupt(GPTDriver *gptp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_GPT == TRUE */

#endif /* HAL_LETIMER_LLD_H */

/** @} */
