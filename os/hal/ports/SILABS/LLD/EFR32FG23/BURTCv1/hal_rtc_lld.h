/*
    ChibiOS - Copyright (C) 2024 Xael South

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
 * @file    hal_rtc_lld.h
 * @brief   EFR32 RTC subsystem low level driver header.
 *
 * @addtogroup RTC
 * @{
 */

#ifndef HAL_RTC_LLD_H
#define HAL_RTC_LLD_H

#if (HAL_USE_RTC == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    Implementation capabilities
 * @{
 */
/**
 * @brief   Callback support int the driver.
 */
#define RTC_SUPPORTS_CALLBACKS      TRUE

/**
 * @brief   Number of alarms available.
 */
#define RTC_ALARMS                  EFR32_RTC_NUM_ALARMS

/**
 * @brief   Presence of a local persistent storage.
 */
#define RTC_HAS_STORAGE             FALSE
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    EFR32 configuration options
 * @{
 */
/**
 * @brief   RTCD1 driver enable switch.
 * @details If set to @p TRUE the support for RTC1 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(EFR32_RTC_USE_RTC1) || defined(__DOXYGEN__)
#define EFR32_RTC_USE_RTC1          TRUE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

#if (RTC_SUPPORTS_CALLBACKS == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Type of an RTC event.
 */
typedef enum {
  RTC_EVENT_TIME_SET    = 0,            /**< New time set.                   */
  RTC_EVENT_ALARM       = 1,            /**< Wakeup.                         */
  RTC_EVENT_TS_OVF      = 2             /**< Time stamp overflow.            */
} rtcevent_t;

/**
 * @brief   Type of a generic RTC callback.
 */
typedef void (*rtccb_t)(RTCDriver* rtcp, rtcevent_t evt);
#endif

#if (RTC_ALARMS > 0) || defined(__DOXYGEN__)
/**
 * @brief   Type of a structure representing an RTC alarm time stamp.
 */
typedef struct {
  uint32_t tv_sec;  /**< Seconds. */
  uint32_t tv_usec; /**< Microseconds. */
} RTCAlarm;
#endif

/**
 * @brief   Persistent RTC driver state kept in backup RAM.
 * @details Allocated as one chunk at EFR32_RTC_BURAM_BASE inside the BURAM1
 *          block (32 bytes), so the BURAM allocation bitmap marks the whole
 *          region at once and no magic per-field offsets are needed.
 */
typedef struct {
  /*
   * Every member is volatile on purpose (CMSIS-style: the qualifier lives
   * on the data, not on the pointer): this struct lives in BURAM, which is
   * shared between contexts and persists across resets/EM4 —
   *   - ovf_counter is incremented in the BURTC interrupt
   *     (rtc_lld_serve_interrupt) while rtc_lld_get_time reads it from
   *     thread context between two CNT reads; without volatile the
   *     compiler could cache it in a register and miss an overflow;
   *   - tv_sec/tv_msec/dstflag may also be written by a previous firmware
   *     run or the bootloader, i.e. the memory can change behind the
   *     compiler's back.
   * Member-level volatile keeps the accesses volatile no matter how the
   * struct is reached (even through a non-volatile pointer cast).
   */
  volatile uint32_t ovf_counter;  /**< BURTC overflow counter (high word of the 64-bit tick). */
  volatile uint32_t tv_sec;       /**< Calendar anchor: seconds since RTC_BASE_YEAR. */
  volatile uint32_t tv_msec;      /**< Calendar anchor: millisecond remainder. */
  volatile uint32_t dstflag;      /**< DST flag. */
} efr32_rtc_buram_state_t;

/** Byte offset of the RTC state inside BURAM1. */
#define EFR32_RTC_BURAM_BASE    0U

/**
 * @brief   Implementation-specific @p RTCDriver fields.
 */
#define rtc_lld_driver_fields                                                     \
  volatile efr32_rtc_buram_state_t *buram_state; /**< Persistent state in BURAM. */ \
  rtccb_t                  callback       /**< Callback function. */

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if (EFR32_RTC_USE_RTC1 == TRUE) && !defined(__DOXYGEN__)
extern RTCDriver RTCD1;
#endif

#ifdef __cplusplus
extern "C"
{
#endif
void rtc_lld_init(void);
void rtc_lld_serve_interrupt(void);   /* shared with SYSTICKv4 (BURTC systick) */
void rtc_lld_set_time(RTCDriver* rtcp, const RTCDateTime* timespec);
void rtc_lld_get_time(RTCDriver* rtcp, RTCDateTime* timespec);
#if RTC_ALARMS > 0
void rtc_lld_set_alarm(RTCDriver* rtcp,
                       rtcalarm_t alarm,
                       const RTCAlarm* alarmspec);
void rtc_lld_get_alarm(RTCDriver* rtcp,
                       rtcalarm_t alarm,
                       RTCAlarm* alarmspec);
#endif
#if RTC_SUPPORTS_CALLBACKS == TRUE
void rtc_lld_set_callback(RTCDriver* rtcp, rtccb_t callback);
#endif
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_RTC == TRUE */

#endif /* HAL_RTC_LLD_H */

/** @} */