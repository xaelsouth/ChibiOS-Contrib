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
 * @file    TIMERv1/hal_timer_irq.c
 * @brief   Shared TIMER interrupt handlers for PWM, GPT and ICU drivers.
 *
 * @details Each TIMER IRQ handler dispatches to the active driver (PWM, GPT
 *          or ICU) for that timer instance.
 *
 * @addtogroup TIMER
 * @{
 */

#include "hal.h"

#if (HAL_USE_PWM == TRUE) || (HAL_USE_GPT == TRUE) || (HAL_USE_ICU == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#if HAL_USE_PWM == TRUE
#if EFR32_PWM_USE_TIMER0 || defined(__DOXYGEN__)
extern PWMDriver PWMD1;
#endif
#if EFR32_PWM_USE_TIMER1 || defined(__DOXYGEN__)
extern PWMDriver PWMD2;
#endif
#if EFR32_PWM_USE_TIMER2 || defined(__DOXYGEN__)
extern PWMDriver PWMD3;
#endif
#if EFR32_PWM_USE_TIMER3 || defined(__DOXYGEN__)
extern PWMDriver PWMD4;
#endif
#if EFR32_PWM_USE_TIMER4 || defined(__DOXYGEN__)
extern PWMDriver PWMD5;
#endif
#endif
#if HAL_USE_GPT == TRUE
#if EFR32_GPT_USE_TIMER0 || defined(__DOXYGEN__)
extern GPTDriver GPTD1;
#endif
#if EFR32_GPT_USE_TIMER1 || defined(__DOXYGEN__)
extern GPTDriver GPTD2;
#endif
#if EFR32_GPT_USE_TIMER2 || defined(__DOXYGEN__)
extern GPTDriver GPTD3;
#endif
#if EFR32_GPT_USE_TIMER3 || defined(__DOXYGEN__)
extern GPTDriver GPTD4;
#endif
#if EFR32_GPT_USE_TIMER4 || defined(__DOXYGEN__)
extern GPTDriver GPTD5;
#endif
#endif
#if HAL_USE_ICU == TRUE
#if EFR32_ICU_USE_TIMER0 || defined(__DOXYGEN__)
extern ICUDriver ICUD1;
#endif
#if EFR32_ICU_USE_TIMER1 || defined(__DOXYGEN__)
extern ICUDriver ICUD2;
#endif
#if EFR32_ICU_USE_TIMER2 || defined(__DOXYGEN__)
extern ICUDriver ICUD3;
#endif
#if EFR32_ICU_USE_TIMER3 || defined(__DOXYGEN__)
extern ICUDriver ICUD4;
#endif
#if EFR32_ICU_USE_TIMER4 || defined(__DOXYGEN__)
extern ICUDriver ICUD5;
#endif
#endif

/*===========================================================================*/
/* Shared IRQ handlers.                                                      */
/*===========================================================================*/

#define TIMER_IRQ_DISPATCH(idx, pwm_en, gpt_en, icu_en)                    \
  OSAL_IRQ_PROLOGUE();                                                      \
  if (pwm_en)       { pwm_lld_serve_interrupt(&PWMD##idx); }               \
  else if (gpt_en)  { gpt_lld_serve_interrupt(&GPTD##idx); }               \
  else if (icu_en)  { icu_lld_serve_interrupt(&ICUD##idx); }               \
  OSAL_IRQ_EPILOGUE()

#if (EFR32_PWM_USE_TIMER0 || EFR32_GPT_USE_TIMER0 || EFR32_ICU_USE_TIMER0) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_TIMER1_HANDLER) {
  TIMER_IRQ_DISPATCH(1, EFR32_PWM_USE_TIMER0, EFR32_GPT_USE_TIMER0, EFR32_ICU_USE_TIMER0);
}
#endif
#if (EFR32_PWM_USE_TIMER1 || EFR32_GPT_USE_TIMER1 || EFR32_ICU_USE_TIMER1) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_TIMER2_HANDLER) {
  TIMER_IRQ_DISPATCH(2, EFR32_PWM_USE_TIMER1, EFR32_GPT_USE_TIMER1, EFR32_ICU_USE_TIMER1);
}
#endif
#if (EFR32_PWM_USE_TIMER2 || EFR32_GPT_USE_TIMER2 || EFR32_ICU_USE_TIMER2) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_TIMER3_HANDLER) {
  TIMER_IRQ_DISPATCH(3, EFR32_PWM_USE_TIMER2, EFR32_GPT_USE_TIMER2, EFR32_ICU_USE_TIMER2);
}
#endif
#if (EFR32_PWM_USE_TIMER3 || EFR32_GPT_USE_TIMER3 || EFR32_ICU_USE_TIMER3) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_TIMER4_HANDLER) {
  TIMER_IRQ_DISPATCH(4, EFR32_PWM_USE_TIMER3, EFR32_GPT_USE_TIMER3, EFR32_ICU_USE_TIMER3);
}
#endif
#if (EFR32_PWM_USE_TIMER4 || EFR32_GPT_USE_TIMER4 || EFR32_ICU_USE_TIMER4) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_TIMER5_HANDLER) {
  TIMER_IRQ_DISPATCH(5, EFR32_PWM_USE_TIMER4, EFR32_GPT_USE_TIMER4, EFR32_ICU_USE_TIMER4);
}
#endif

#endif /* HAL_USE_PWM || HAL_USE_GPT || HAL_USE_ICU */

/** @} */
