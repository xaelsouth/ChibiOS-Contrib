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
 * @brief   Shared TIMER interrupt handlers for PWM and GPT drivers.
 *
 * @details Each TIMER IRQ handler dispatches to the active driver (PWM or
 *          GPT) for that timer instance. This avoids duplicate symbol
 *          definitions when both HAL_USE_PWM and HAL_USE_GPT are enabled.
 *
 * @addtogroup TIMER
 * @{
 */

#include "hal.h"

#if (HAL_USE_PWM == TRUE) || (HAL_USE_GPT == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#if HAL_USE_PWM == TRUE
extern PWMDriver PWMD1, PWMD2, PWMD3, PWMD4, PWMD5;
#endif
#if HAL_USE_GPT == TRUE
extern GPTDriver GPTD1, GPTD2, GPTD3, GPTD4, GPTD5;
#endif

/*===========================================================================*/
/* Shared IRQ handlers.                                                      */
/*===========================================================================*/

#define TIMER_IRQ_DISPATCH(idx, pwm_en, gpt_en)                            \
  OSAL_IRQ_PROLOGUE();                                                      \
  if (pwm_en) { pwm_lld_serve_interrupt(&PWMD##idx); }                     \
  else if (gpt_en) { gpt_lld_serve_interrupt(&GPTD##idx); }                 \
  OSAL_IRQ_EPILOGUE()

#if (EFR32_PWM_USE_TIMER0 || EFR32_GPT_USE_TIMER0) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(TIMER0_IRQHandler) {
  TIMER_IRQ_DISPATCH(1, EFR32_PWM_USE_TIMER0, EFR32_GPT_USE_TIMER0);
}
#endif
#if (EFR32_PWM_USE_TIMER1 || EFR32_GPT_USE_TIMER1) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(TIMER1_IRQHandler) {
  TIMER_IRQ_DISPATCH(2, EFR32_PWM_USE_TIMER1, EFR32_GPT_USE_TIMER1);
}
#endif
#if (EFR32_PWM_USE_TIMER2 || EFR32_GPT_USE_TIMER2) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(TIMER2_IRQHandler) {
  TIMER_IRQ_DISPATCH(3, EFR32_PWM_USE_TIMER2, EFR32_GPT_USE_TIMER2);
}
#endif
#if (EFR32_PWM_USE_TIMER3 || EFR32_GPT_USE_TIMER3) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(TIMER3_IRQHandler) {
  TIMER_IRQ_DISPATCH(4, EFR32_PWM_USE_TIMER3, EFR32_GPT_USE_TIMER3);
}
#endif
#if (EFR32_PWM_USE_TIMER4 || EFR32_GPT_USE_TIMER4) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(TIMER4_IRQHandler) {
  TIMER_IRQ_DISPATCH(5, EFR32_PWM_USE_TIMER4, EFR32_GPT_USE_TIMER4);
}
#endif

#endif /* HAL_USE_PWM || HAL_USE_GPT */

/** @} */
