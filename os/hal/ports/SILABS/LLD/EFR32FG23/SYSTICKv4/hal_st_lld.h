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
 * @file    SYSTICKv4/hal_st_lld.h
 * @brief   EFR32FG23 BURTC-based ST driver low level header.
 *
 * @details Uses the BURTC 32-bit up-counter (running from EM4GRPACLK =
 *          LFXO/LFRCO/ULFRCO) as the ChibiOS system tick. Compared to
 *          SYSTICKv3 (LETIMER0, 24-bit):
 *          - full 32-bit counter, no software extension needed
 *          - works in EM4 (deepest sleep) — BURTC is in the always-on domain
 *          - single compare channel (COMP) used as the tick alarm
 *
 *          BURTC is shared with the BURTCv1 RTC calendar driver: SYSTICKv4
 *          owns the BURTC interrupt vector and forwards RTC events to
 *          rtc_lld_serve_interrupt() (see EFR32_ST_OWNS_BURTC below).
 *
 *          Select with EFR32_USE_SYSTICKV4=yes (platform.mk includes
 *          SYSTICKv4/driver.mk instead of SYSTICKv3/driver.mk).
 *
 * @addtogroup ST
 * @{
 */

#ifndef HAL_ST_LLD_H
#define HAL_ST_LLD_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/* BURTC CNT is full 32-bit. */
#if (OSAL_ST_RESOLUTION == 32)
  #define ST_INIT_VALUE                        0xFFFFFFFFUL
#else
  #define ST_INIT_VALUE                        0x0000FFFFUL
#endif

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

#if !defined(EFR32_BURTC_ST_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_BURTC_ST_IRQ_PRIORITY         4
#endif

/* SYSTICKv4 owns the BURTC interrupt vector (EFR32_BURTC_HANDLER).
   When BURTCv1 (RTC) is also enabled it must NOT define its own handler;
   instead this handler forwards events via rtc_lld_serve_interrupt(). */
#define EFR32_ST_OWNS_BURTC                 1

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void st_lld_init(void);
  void st_lld_serve_interrupt(void);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Driver inline functions.                                                  */
/*===========================================================================*/

#if (OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING) || defined(__DOXYGEN__)

/**
 * @brief   Returns the time counter value (counts UP from 0).
 */
static inline systime_t st_lld_get_counter(void) {

  return (systime_t)BURTC->CNT;
}

/**
 * @brief   Starts the alarm at abstime.
 */
static inline void st_lld_start_alarm(systime_t abstime) {

  BURTC->IEN_CLR = BURTC_IEN_COMP;
  #if (OSAL_ST_RESOLUTION == 32)
  BURTC->COMP = (uint32_t)abstime;
  #else
  BURTC->COMP = (uint16_t)abstime;
  #endif
  while ((BURTC->SYNCBUSY & BURTC_SYNCBUSY_COMP) != 0U) {}
  BURTC->IF_CLR = BURTC_IF_COMP;
  BURTC->IEN_SET = BURTC_IEN_COMP;
}

static inline void st_lld_stop_alarm(void) {

  BURTC->IEN_CLR = BURTC_IEN_COMP;
}

static inline void st_lld_set_alarm(systime_t abstime) {

  st_lld_start_alarm(abstime);
}

static inline systime_t st_lld_get_alarm(void) {

  return (systime_t)BURTC->COMP;
}

static inline bool st_lld_is_alarm_active(void) {

  return (BURTC->IEN & BURTC_IEN_COMP) != 0U;
}

/* BURTC has a single compare channel. */
#define ST_LLD_NUM_ALARMS                     1

#endif /* OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING */

#endif /* HAL_ST_LLD_H */

/** @} */
