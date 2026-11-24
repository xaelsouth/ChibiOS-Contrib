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
 * @file    SYSTICKv2/hal_st_lld.h
 * @brief   EFR32FG23 SYSRTC-based ST driver low level header.
 *
 * @details Uses the SYSRTC 32-bit counter (running from LFXO/LFRCO/ULFRCO)
 *          as the ChibiOS system tick. Properties:
 *          - 32-bit counter (no 24-bit LETIMER wrap problem)
 *          - Works in EM2/EM3/EM4 — SYSRTC is in the always-on domain
 *          - Has overflow interrupt for long-running time tracking
 *          - 2 compare channels (CMP0 for tick, CMP1 for secondary alarm)
 *
 *          This is the default systick selected by the FG23 platform.mk.
 *          The core SysTick (SYSTICKv1) can be selected instead with
 *          EFR32_USE_SYSTICKV1=yes. LETIMER0 is not used as a tick and stays
 *          available for LETIMERv1 (GPT).
 *
 * @addtogroup ST
 * @{
 */

#ifndef HAL_ST_LLD_H
#define HAL_ST_LLD_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/* SYSRTC CNT is full 32-bit. */
#if (OSAL_ST_RESOLUTION == 32)
  #define ST_INIT_VALUE                        0xFFFFFFFFUL
#else
  #define ST_INIT_VALUE                        0x0000FFFFUL
#endif

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

#if !defined(EFR32_SYSRTC_ST_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_SYSRTC_ST_IRQ_PRIORITY         4
#endif

/* SYSTICKv2 owns the SYSRTC_SEQ interrupt vector (Vector for SYSRTC_SEQ_IRQn).
   When SYSRTCv1 is also enabled it must NOT define its own SEQ handler;
   instead this handler forwards events via sysrtc_lld_serve_interrupt(). */
#define EFR32_ST_OWNS_SYSRTC_SEQ            1

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
  /* SYSRTC CNT counts up. We return it directly as elapsed ticks. */
  return (systime_t)SYSRTC0->CNT;
}

/**
 * @brief   Starts the alarm at abstime.
 */
static inline void st_lld_start_alarm(systime_t abstime) {
  SYSRTC0->GRP0_IEN_CLR = SYSRTC_GRP0_IEN_CMP0;
  #if (OSAL_ST_RESOLUTION == 32)
  SYSRTC0->GRP0_CMP0VALUE = (uint32_t)abstime;
  #else
  SYSRTC0->GRP0_CMP0VALUE = (uint16_t)abstime;
  #endif
  SYSRTC0->GRP0_IF_CLR = SYSRTC_GRP0_IF_CMP0;
  SYSRTC0->GRP0_IEN_SET = SYSRTC_GRP0_IEN_CMP0;
}

static inline void st_lld_stop_alarm(void) {
  SYSRTC0->GRP0_IEN_CLR = SYSRTC_GRP0_IEN_CMP0;
}

static inline void st_lld_set_alarm(systime_t abstime) {
  st_lld_start_alarm(abstime);
}

static inline systime_t st_lld_get_alarm(void) {
  return (systime_t)SYSRTC0->GRP0_CMP0VALUE;
}

static inline bool st_lld_is_alarm_active(void) {
  return (SYSRTC0->GRP0_IEN & SYSRTC_GRP0_IEN_CMP0) != 0U;
}

#define ST_LLD_NUM_ALARMS                     2

#if (ST_LLD_NUM_ALARMS > 1) || defined(__DOXYGEN__)

static inline void st_lld_start_alarm_n(unsigned alarm, systime_t abstime) {
  (void)alarm;
  SYSRTC0->GRP0_IEN_CLR = SYSRTC_GRP0_IEN_CMP1;
  #if (OSAL_ST_RESOLUTION == 32)
  SYSRTC0->GRP0_CMP1VALUE = (uint32_t)abstime;
  #else
  SYSRTC0->GRP0_CMP1VALUE = (uint16_t)abstime;
  #endif
  SYSRTC0->GRP0_IF_CLR = SYSRTC_GRP0_IF_CMP1;
  SYSRTC0->GRP0_IEN_SET = SYSRTC_GRP0_IEN_CMP1;
}

static inline void st_lld_stop_alarm_n(unsigned alarm) {
  (void)alarm;
  SYSRTC0->GRP0_IEN_CLR = SYSRTC_GRP0_IEN_CMP1;
}

static inline void st_lld_set_alarm_n(unsigned alarm, systime_t abstime) {
  (void)alarm;
  st_lld_start_alarm_n(alarm, abstime);
}

static inline systime_t st_lld_get_alarm_n(unsigned alarm) {
  (void)alarm;
  return (systime_t)SYSRTC0->GRP0_CMP1VALUE;
}

static inline bool st_lld_is_alarm_active_n(unsigned alarm) {
  (void)alarm;
  return (SYSRTC0->GRP0_IEN & SYSRTC_GRP0_IEN_CMP1) != 0U;
}

#endif /* ST_LLD_NUM_ALARMS > 1 */

#endif /* OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING */

#endif /* HAL_ST_LLD_H */

/** @} */
