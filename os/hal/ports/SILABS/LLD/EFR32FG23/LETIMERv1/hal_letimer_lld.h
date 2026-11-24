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

#if !defined(EFR32_LETIMER_WIN_USE_LETIMER0) || defined(__DOXYGEN__)
#define EFR32_LETIMER_WIN_USE_LETIMER0    FALSE
#endif

#if !defined(EFR32_LETIMER_WIN_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_LETIMER_WIN_IRQ_PRIORITY    4
#endif

#if defined(EFR32_LETIMER_GPT_USE_LETIMER0) && (EFR32_LETIMER_GPT_USE_LETIMER0 == TRUE) && \
    (EFR32_LETIMER_WIN_USE_LETIMER0 == TRUE)
#error "LETIMER0 cannot be claimed by both the GPT driver and the window API"
#endif

/**
 * @brief   Window-end callback, invoked from the LETIMER0 underflow ISR.
 */
typedef void (*letimer_window_cb_t)(void);

#if (EFR32_LETIMER_WIN_USE_LETIMER0 == TRUE) || defined(__DOXYGEN__)
/*===========================================================================*/
/* PRS-started one-shot window (independent of HAL_USE_GPT).                 */
/*===========================================================================*/

/**
 * @brief   Arm a PRS-started one-shot window on LETIMER0.
 * @details Configures LETIMER0 in REPMODE_ONESHOT with PRS start enabled on
 *          the async channel selected by the SIGROUTE consumer route
 *          (SIGROUTE_CON_LETIMER0_START). Each PRS pulse starts the window;
 *          while running, further START pulses are ignored (no retrigger).
 *          On underflow (window end) the UF interrupt fires and @p cb is
 *          called from ISR context — the natural place to snapshot a pulse
 *          counter (e.g. PCNT0) plus the system time into a ring buffer.
 *
 * @param[in] ticks         window length in EM23GRPACLK ticks (24-bit)
 * @param[in] cb            window-end callback (ISR context), may be NULL
 */
void letimer_lld_window_start(uint32_t ticks, letimer_window_cb_t cb);

/**
 * @brief   Disarm the LETIMER0 window (stops the timer, frees the vector).
 */
void letimer_lld_window_stop(void);

/**
 * @brief   Restart the window if it is not running.
 * @details Race fix for the UF handler: a PRS START pulse that arrives in
 *          the same LF cycle as the underflow can be ignored (the timer
 *          still looked RUNNING). The caller detects such a late pulse
 *          (e.g. a PCNT increment after its CNT snapshot) and calls this;
 *          it issues CMD START only when the timer is stopped, so a window
 *          that a PRS pulse already restarted is not disturbed. START while
 *          running is a hardware no-op anyway.
 */
void letimer_lld_window_kick(void);
#endif

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
