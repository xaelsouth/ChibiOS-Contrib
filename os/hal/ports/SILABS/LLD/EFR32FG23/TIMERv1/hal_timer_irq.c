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

/* Fallback defines when the corresponding LLD header is not included
   (driver disabled in halconf). */
#if !defined(EFR32_PWM_USE_TIMER0)
#define EFR32_PWM_USE_TIMER0  FALSE
#define EFR32_PWM_USE_TIMER1  FALSE
#define EFR32_PWM_USE_TIMER2  FALSE
#define EFR32_PWM_USE_TIMER3  FALSE
#define EFR32_PWM_USE_TIMER4  FALSE
#endif
#if !defined(EFR32_GPT_USE_TIMER0)
#define EFR32_GPT_USE_TIMER0  FALSE
#define EFR32_GPT_USE_TIMER1  FALSE
#define EFR32_GPT_USE_TIMER2  FALSE
#define EFR32_GPT_USE_TIMER3  FALSE
#define EFR32_GPT_USE_TIMER4  FALSE
#endif
#if !defined(EFR32_ICU_USE_TIMER0)
#define EFR32_ICU_USE_TIMER0  FALSE
#define EFR32_ICU_USE_TIMER1  FALSE
#define EFR32_ICU_USE_TIMER2  FALSE
#define EFR32_ICU_USE_TIMER3  FALSE
#define EFR32_ICU_USE_TIMER4  FALSE
#endif

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

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

/*===========================================================================*/
/* Shared IRQ handlers.                                                      */
/*===========================================================================*/

#define TIMER_IRQ(vect, idx, pwm_en, gpt_en, icu_en)                        \
OSAL_IRQ_HANDLER(vect) {                                                    \
  OSAL_IRQ_PROLOGUE();                                                      \
  TIMER_IRQ_SELECT(idx, pwm_en, gpt_en, icu_en)                             \
  OSAL_IRQ_EPILOGUE();                                                      \
}

/* Preprocessor selection: only the enabled driver's symbol is referenced. */
#if EFR32_PWM_USE_TIMER0 || defined(__DOXYGEN__)
#define TIMER_IRQ_SELECT_0  pwm_lld_serve_interrupt(&PWMD1);
#elif EFR32_GPT_USE_TIMER0
#define TIMER_IRQ_SELECT_0  gpt_lld_serve_interrupt(&GPTD1);
#elif EFR32_ICU_USE_TIMER0
#define TIMER_IRQ_SELECT_0  icu_lld_serve_interrupt(&ICUD1);
#else
#define TIMER_IRQ_SELECT_0
#endif
#if EFR32_PWM_USE_TIMER1 || defined(__DOXYGEN__)
#define TIMER_IRQ_SELECT_1  pwm_lld_serve_interrupt(&PWMD2);
#elif EFR32_GPT_USE_TIMER1
#define TIMER_IRQ_SELECT_1  gpt_lld_serve_interrupt(&GPTD2);
#elif EFR32_ICU_USE_TIMER1
#define TIMER_IRQ_SELECT_1  icu_lld_serve_interrupt(&ICUD2);
#else
#define TIMER_IRQ_SELECT_1
#endif
#if EFR32_PWM_USE_TIMER2 || defined(__DOXYGEN__)
#define TIMER_IRQ_SELECT_2  pwm_lld_serve_interrupt(&PWMD3);
#elif EFR32_GPT_USE_TIMER2
#define TIMER_IRQ_SELECT_2  gpt_lld_serve_interrupt(&GPTD3);
#elif EFR32_ICU_USE_TIMER2
#define TIMER_IRQ_SELECT_2  icu_lld_serve_interrupt(&ICUD3);
#else
#define TIMER_IRQ_SELECT_2
#endif
#if EFR32_PWM_USE_TIMER3 || defined(__DOXYGEN__)
#define TIMER_IRQ_SELECT_3  pwm_lld_serve_interrupt(&PWMD4);
#elif EFR32_GPT_USE_TIMER3
#define TIMER_IRQ_SELECT_3  gpt_lld_serve_interrupt(&GPTD4);
#elif EFR32_ICU_USE_TIMER3
#define TIMER_IRQ_SELECT_3  icu_lld_serve_interrupt(&ICUD4);
#else
#define TIMER_IRQ_SELECT_3
#endif
#if EFR32_PWM_USE_TIMER4 || defined(__DOXYGEN__)
#define TIMER_IRQ_SELECT_4  pwm_lld_serve_interrupt(&PWMD5);
#elif EFR32_GPT_USE_TIMER4
#define TIMER_IRQ_SELECT_4  gpt_lld_serve_interrupt(&GPTD5);
#elif EFR32_ICU_USE_TIMER4
#define TIMER_IRQ_SELECT_4  icu_lld_serve_interrupt(&ICUD5);
#else
#define TIMER_IRQ_SELECT_4
#endif

#define TIMER_IRQ_SELECT(idx, pwm_en, gpt_en, icu_en)  TIMER_IRQ_SELECT_##idx

#if (EFR32_PWM_USE_TIMER0 || EFR32_GPT_USE_TIMER0 || EFR32_ICU_USE_TIMER0) || defined(__DOXYGEN__)
TIMER_IRQ(EFR32_TIMER1_HANDLER, 0, EFR32_PWM_USE_TIMER0, EFR32_GPT_USE_TIMER0, EFR32_ICU_USE_TIMER0)
#endif
#if (EFR32_PWM_USE_TIMER1 || EFR32_GPT_USE_TIMER1 || EFR32_ICU_USE_TIMER1) || defined(__DOXYGEN__)
TIMER_IRQ(EFR32_TIMER2_HANDLER, 1, EFR32_PWM_USE_TIMER1, EFR32_GPT_USE_TIMER1, EFR32_ICU_USE_TIMER1)
#endif
#if (EFR32_PWM_USE_TIMER2 || EFR32_GPT_USE_TIMER2 || EFR32_ICU_USE_TIMER2) || defined(__DOXYGEN__)
TIMER_IRQ(EFR32_TIMER3_HANDLER, 2, EFR32_PWM_USE_TIMER2, EFR32_GPT_USE_TIMER2, EFR32_ICU_USE_TIMER2)
#endif
#if (EFR32_PWM_USE_TIMER3 || EFR32_GPT_USE_TIMER3 || EFR32_ICU_USE_TIMER3) || defined(__DOXYGEN__)
TIMER_IRQ(EFR32_TIMER4_HANDLER, 3, EFR32_PWM_USE_TIMER3, EFR32_GPT_USE_TIMER3, EFR32_ICU_USE_TIMER3)
#endif
#if (EFR32_PWM_USE_TIMER4 || EFR32_GPT_USE_TIMER4 || EFR32_ICU_USE_TIMER4) || defined(__DOXYGEN__)
TIMER_IRQ(EFR32_TIMER5_HANDLER, 4, EFR32_PWM_USE_TIMER4, EFR32_GPT_USE_TIMER4, EFR32_ICU_USE_TIMER4)
#endif

#endif /* HAL_USE_PWM || HAL_USE_GPT || HAL_USE_ICU */

/** @} */
