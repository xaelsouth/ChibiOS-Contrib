/*
    ChibiOS - Copyright (C) 2023 Xael South

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
 * @file    SYSTICKv1/hal_st_lld.h
 * @brief   ARM Cortex-M core SysTick based ST low level driver header.
 * @details This header is designed to be include-able without having to
 *          include other files from the HAL.
 *
 *          Optional driver, NOT included by any platform.mk by default.
 *          Select it by building with EFR32_USE_SYSTICKV1=yes (see the
 *          platform.mk of the MCU) instead of the default SYSTICKv3.
 *
 *          Both OSAL_ST_MODE_PERIODIC and OSAL_ST_MODE_FREERUNNING are
 *          supported:
 *          - PERIODIC:     SysTick fires at OSAL_ST_FREQUENCY, the ISR
 *                          calls the OS timer handler on every tick.
 *          - FREERUNNING:  SysTick fires at OSAL_ST_FREQUENCY and a
 *                          software 32-bit counter is incremented in the
 *                          ISR; alarms are checked in the same ISR with
 *                          1 tick precision (SysTick has no compare
 *                          register, so this is how the alarm is emulated).
 *
 *          Note: the core SysTick is clocked from HCLK and stops in
 *          EM2/EM3 — for a low-power system tick use SYSTICKv3 (LETIMER,
 *          runs in EM2) or SYSTICKv2 (SYSRTC, runs in EM4) instead.
 *
 * @addtogroup ST
 * @{
 */

#ifndef HAL_ST_LLD_H
#define HAL_ST_LLD_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   SysTick counter width in bits.
 */
#define EFR32_SYSTICK_WIDTH                     24

/**
 * @brief   SysTick counter maximum value (2^24 - 1).
 */
#define EFR32_SYSTICK_CK_MAX                    0x00FFFFFFU

/**
 * @brief   ST Alarms number.
 */
#define ST_LLD_NUM_ALARMS                       1

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   SysTick timer IRQ priority.
 */
#if !defined(EFR32_ST_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_ST_IRQ_PRIORITY                   4
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING
  /* The free running counter is a 32 bit software counter. */
  #if (OSAL_ST_RESOLUTION != 32) && !defined(__DOXYGEN__)
    #error "SYSTICKv1 free running mode requires CH_CFG_ST_RESOLUTION == 32"
  #endif
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                     */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void st_lld_init(void);
  void st_lld_serve_interrupt(void);
#if (OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING) || defined(__DOXYGEN__)
  /* Free running software counter and alarm state, maintained by the
     SysTick ISR in hal_st_lld.c. */
  extern volatile uint32_t st_lld_sw_counter;
  extern volatile uint32_t st_lld_sw_alarm;
  extern volatile bool     st_lld_sw_alarm_active;
#endif
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Driver inline functions.                                                  */
/*===========================================================================*/

#if (OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING) || defined(__DOXYGEN__)

/**
 * @brief   Returns the time counter value.
 *
 * @return              The counter value.
 *
 * @notapi
 */
static inline systime_t st_lld_get_counter(void) {

  /* 32 bit aligned read is atomic on Cortex-M. */
  return (systime_t)st_lld_sw_counter;
}

/**
 * @brief   Starts the alarm.
 * @note    Makes sure that no spurious alarms are triggered after
 *          this call.
 *
 * @param[in] abstime   the time to be set for the first alarm
 *
 * @notapi
 */
static inline void st_lld_start_alarm(systime_t abstime) {

  st_lld_sw_alarm = (uint32_t)abstime;
  st_lld_sw_alarm_active = true;
}

/**
 * @brief   Stops the alarm interrupt.
 *
 * @notapi
 */
static inline void st_lld_stop_alarm(void) {

  st_lld_sw_alarm_active = false;
}

/**
 * @brief   Sets the alarm time.
 *
 * @param[in] abstime   the time to be set for the next alarm
 *
 * @notapi
 */
static inline void st_lld_set_alarm(systime_t abstime) {

  st_lld_start_alarm(abstime);
}

/**
 * @brief   Returns the current alarm time.
 *
 * @return              The currently set alarm time.
 *
 * @notapi
 */
static inline systime_t st_lld_get_alarm(void) {

  return (systime_t)st_lld_sw_alarm;
}

/**
 * @brief   Determines if the alarm is active.
 *
 * @return              The alarm status.
 * @retval false        if the alarm is not active.
 * @retval true         is the alarm is active
 *
 * @notapi
 */
static inline bool st_lld_is_alarm_active(void) {

  return st_lld_sw_alarm_active;
}

#endif /* OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING */

#endif /* HAL_ST_LLD_H */

/** @} */
