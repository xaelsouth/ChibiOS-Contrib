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
 * @file    hal_rtc_lld.c
 * @brief   EFR32 RTC subsystem low level driver source.
 *
 * @addtogroup RTC
 * @{
 */

#include "hal.h"
#include "hal_sysrtc_lld.h"

#if (RTC_ALARMS > 0) || defined(__DOXYGEN__)
static void rtc_lld_sysrtc_cb(efr32_sysrtc_evt_t evt, uint32_t value, void *arg);
#endif

#if (HAL_USE_RTC == TRUE) || defined(__DOXYGEN__)

#if (HAL_USE_RTC == TRUE) && (!defined(HAL_USE_BURAM) || (HAL_USE_BURAM != TRUE))
#error "EFR32 BURTC RTC driver requires HAL_USE_BURAM=TRUE (overflow counter stored in backup RAM)"
#endif

#if (HAL_USE_RTC == TRUE) && (HAL_USE_BURAM == TRUE) || defined(__DOXYGEN__)

#include "hal_buram.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/**
 * EFR32_RTCCLK_DIV is more or less arbitrary. The smaller the value
 * the often overflow interrupt will occur. To avoid frequent occurance
 * of interrupts set the value as high as possible.
 */

#if EFR32_BURTCCLK == 1000U /* Clock source is ULFRCO. */
#define EFR32_RTCCLK_DIV 1U
#define EFR32_BURTC_CFG_CNTPRESC  BURTC_CFG_CNTPRESC_DIV1
#elif EFR32_BURTCCLK == 32768U /* Clock source is LFXO or LFRCO. */
#define EFR32_RTCCLK_DIV 1U
#define EFR32_BURTC_CFG_CNTPRESC  BURTC_CFG_CNTPRESC_DIV1
#else
#error "EFR32_RTCCLK_DIV is not obtainable"
#endif

#define EFR32_RTCCLK                (EFR32_BURTCCLK / EFR32_RTCCLK_DIV)

/* COMP period in BURTCCLK ticks — the calendar tick (1 s at 32768 Hz).
   Must divide 2^32 so the mod-2^32 COMP re-arm stays aligned across OF
   wraps; 32768 = 2^15 does, 1000 (ULFRCO) does not — use LFXO/LFRCO. */
#define EFR32_RTC_COMP_PERIOD       EFR32_RTCCLK

#define EFR32_RTC_OVERFLOW_VALUE    ((_BURTC_CNT_CNT_MASK >> _BURTC_CNT_CNT_SHIFT) + 1UL)

#define EFR32_RTC_OVERFLOW_INTERVAL (EFR32_RTC_OVERFLOW_VALUE / EFR32_RTCCLK)

#define EFR32_RTC_OVERFLOW_INTERVAL_REMAINDER \
                                    (EFR32_RTC_OVERFLOW_VALUE % EFR32_RTCCLK)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief RTC driver identifier.
 */
#if (EFR32_RTC_USE_RTC1 == TRUE) && !defined(__DOXYGEN__)
RTCDriver RTCD1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

static const int rtc_day_offset_by_month[2][12] = {
  [0] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
  [1] = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 }
};

static const int rtc_day_of_the_week[7] = {
  RTC_DAY_SUNDAY, RTC_DAY_MONDAY, RTC_DAY_TUESDAY, RTC_DAY_WEDNESDAY,
  RTC_DAY_THURSDAY, RTC_DAY_FRIDAY, RTC_DAY_SATURDAY
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

#define isleap(y) ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)

/**
 * @brief   Return day of week according by implementing Zeller's congruence.
 *
 * @param[in] year      Year, e.g. 1969
 * @param[in] month     Months 1..12.
 * @param[in] day       Day of the month 1..31
 * @return              Day of week 0 .. 6, Sun .. Sat.
 *
 */
__STATIC_INLINE int _rtc_lld_wday(int year, int month, int day) {

  int adjustment, mm, yy, wday;

  adjustment = (14 - month) / 12;
  mm = month + 12 * adjustment - 2;
  yy = year - adjustment;

  /* wday in range from 0 .. 6: Sun to Sat */
  wday = (day + (13 * mm - 1) / 5 +
          yy + yy / 4 - yy / 100 + yy / 400) % 7;

  return wday;
}

__STATIC_INLINE void _rtc_lld_to_timespec_slow(uint32_t tv_sec, uint32_t tv_msec, RTCDateTime* timespec, uint32_t dstflag) {

  int year = RTC_BASE_YEAR;
  int day = tv_sec / 86400;

  /* Seconds left after all days are subtracted. */
  tv_sec = tv_sec % 86400;

  /* Calculate year in 4 years steps. */
  do {
    /* A 4-year block [year, year+4) contains exactly one leap year
       unless it spans a century non-leap year (e.g. 2100). */
    int days_in_4years = 4 * 365;
    if (isleap(year) || isleap(year + 1) || isleap(year + 2) || isleap(year + 3))
      days_in_4years += 1;

    if (day < days_in_4years)
      break;

    day -= days_in_4years;
    year += 4;
  }
  while (true);

  /* Further calculate year in 1 years steps. */
  do {
    int days_in_1year = isleap(year) ? 366 : 365;

    if (day < days_in_1year)
      break;

    day -= days_in_1year;
    year += 1;
  }
  while (true);

  /* Calculate month. */
  int month = 0;
  const int *day_offset_by_month = isleap(year) ?
                                   &rtc_day_offset_by_month[1][0] :
                                   &rtc_day_offset_by_month[0][0];  
  for (int end = 12; month != end;) {
    int mid = (month + end) / 2;

    if (day_offset_by_month[mid] <= day) {
      month = mid + 1;
    }
    else {
      end = mid;
    }
  }

  day -= day_offset_by_month[month - 1];

  day += 1; /* 0 .. 30 -> 1 .. 31 */;

  timespec->millisecond = tv_sec * 1000U + tv_msec;
  timespec->dstflag = dstflag;
  timespec->year = year  - RTC_BASE_YEAR;
  timespec->month = month;
  timespec->day = day;
  int dayofweek = _rtc_lld_wday(year, month, day);
  /* wday 0 .. 6: Sun .. Sat -> 1 .. 7: Mon .. Sun */
  timespec->dayofweek = rtc_day_of_the_week[dayofweek];
}

__STATIC_INLINE void _rtc_lld_to_timespec_approx(uint32_t tv_sec, uint32_t tv_msec, RTCDateTime* timespec, uint32_t dstflag) {

  int day = tv_sec / 86400;

  /* Seconds left after subtracting of all days. */
  tv_sec -= day * 86400;

  /* Every fourth year from 1980 to 2096 is leap year, so we can use that
     formula for the next 70 years. */
  int year = (day * 4) / (365 * 4 + 1);

  day -= year * 365 + (year + 3) / 4;

  year += RTC_BASE_YEAR;

  /* Calculate month. */
  const int *day_offset_by_month = isleap(year) ?
                                   &rtc_day_offset_by_month[1][0] :
                                   &rtc_day_offset_by_month[0][0];
  int month = 0;
  for (int end = 12; month != end;) {
    int mid = (month + end) / 2;

    if (day_offset_by_month[mid] <= day) {
      month = mid + 1;
    }
    else {
      end = mid;
    }
  }

  day -= day_offset_by_month[month - 1];

  day += 1; /* 0 .. 30 -> 1 .. 31 */;

  timespec->millisecond = tv_sec * 1000U + tv_msec;
  timespec->dstflag = dstflag;
  timespec->year = year - RTC_BASE_YEAR;
  timespec->month = month;
  timespec->day = day;
  int dayofweek = _rtc_lld_wday(year, month, day);
  /* wday 0 .. 6: Sun .. Sat -> 1 .. 7: Mon .. Sun */
  timespec->dayofweek = rtc_day_of_the_week[dayofweek];
}

__STATIC_INLINE void _rtc_lld_to_timespec(uint32_t tv_sec, uint32_t tv_msec, RTCDateTime* timespec, uint32_t dstflag) {

  return _rtc_lld_to_timespec_slow(tv_sec, tv_msec, timespec, dstflag);
}

/**
 * @brief   Store the calendar anchor for a new time.
 * @details BURTC is the backup-domain timekeeper only (the systick is
 *          SYSRTC0/SYSTICKv2), so setting the calendar may freely restart
 *          CNT: clear it, zero the overflow counter, store the anchor.
 */
__STATIC_INLINE void _rtc_lld_store_time(RTCDriver *rtcp, uint32_t tv_sec,
                                         uint32_t tv_msec, uint32_t dstflag) {

  syssts_t sts;

  sts = osalSysGetStatusAndLockX();

  /* Clear all counters resetting the overflow counter as well. */
  BURTC->CNT_CLR = _BURTC_CNT_CNT_MASK;

  rtcp->buram_state->ovf_counter = 0U;
  rtcp->buram_state->comp_cnt = 0U;
  rtcp->buram_state->tv_sec = tv_sec;
  rtcp->buram_state->tv_msec = tv_msec;
  rtcp->buram_state->dstflag = dstflag;

  while ((BURTC->SYNCBUSY & _BURTC_SYNCBUSY_MASK) != 0U);

  /* Re-anchor the COMP calendar tick (CNT was just cleared, comp_cnt = 0). */
  BURTC->COMP = EFR32_RTC_COMP_PERIOD;
  while ((BURTC->SYNCBUSY & BURTC_SYNCBUSY_COMP) != 0U);

  if (rtcp->callback != NULL) {
    rtcp->callback(rtcp, RTC_EVENT_TIME_SET);
  }

  osalSysRestoreStatusX(sts);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   RTC common interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(EFR32_BURTC_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  rtc_lld_serve_interrupt();

  OSAL_IRQ_EPILOGUE();
}

/**
 * @brief   BURTC interrupt event processing.
 *
 * @isr
 */
void rtc_lld_serve_interrupt(void) {

  uint32_t isr;

  isr = BURTC->IF & _BURTC_IF_MASK;
  BURTC->IF_CLR = isr;

  if ((isr & _BURTC_IF_OF_MASK) == BURTC_IF_OF) {
    #if EFR32_RTC_USE_RTC1 == TRUE
    /* Increment overflow counter in order of tracking time. */
    RTCD1.buram_state->ovf_counter += 1U;

    if (RTCD1.callback != NULL) {
      RTCD1.callback(&RTCD1, RTC_EVENT_TS_OVF);
    }
    #endif
  }

  if ((isr & _BURTC_IF_COMP_MASK) == BURTC_IF_COMP) {
    #if EFR32_RTC_USE_RTC1 == TRUE
    /* Calendar tick: one COMP period (EFR32_RTC_COMP_PERIOD = 1 s at 32768 Hz)
       elapsed. Re-arm COMP from the BURAM software counter only — CNT reads
       lag the real counter on this part, so the calendar never reads CNT.
       With period = 2^15 the mod-2^32 relation COMP = comp_cnt*period stays
       aligned across OF wraps. */
    RTCD1.buram_state->comp_cnt += 1U;
    while ((BURTC->SYNCBUSY & BURTC_SYNCBUSY_COMP) != 0U) { }
    BURTC->COMP = RTCD1.buram_state->comp_cnt * EFR32_RTC_COMP_PERIOD;
    #endif
  }
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Enable access to registers.
 *
 * @notapi
 */
void rtc_lld_init(void) {

#if EFR32_RTC_USE_RTC1 == TRUE
  /* RTC object initialization. */
  rtcObjectInit(&RTCD1);

  /* Callback initially disabled. */
  RTCD1.callback = NULL;

  /* Get the persistent state from backup RAM, which survives restart.
     One allocation for the whole efr32_rtc_buram_state_t at
     EFR32_RTC_BURAM_BASE — the BURAM bitmap marks the whole chunk. */
  RTCD1.buram_state = buramAllocateAtI(&BURAMD1, EFR32_RTC_BURAM_BASE,
                                       sizeof(efr32_rtc_buram_state_t));
  osalDbgCheck(RTCD1.buram_state != NULL);

  #if (RTC_ALARMS > 0) && !defined(__DOXYGEN__)
  /* Alarm channel lives on SYSRTC CMP1. sysrtc_lld_start() is a no-op for
     the counter when SYSTICKv2 already started it; it only registers the
     callback (CMP1/CAP0 channels are left alone). */
  sysrtc_lld_start(&SYSRTCD1, rtc_lld_sysrtc_cb, NULL);
  #endif
#endif

  /* Enable clock. */
  CMU->CLKEN0_SET = CMU_CLKEN0_BURTC;

  /* After reboot the BURTC must remain enabled.
     Check this before changing its configuration. */
  if ((BURTC->EN & _BURTC_EN_MASK) == BURTC_EN_EN) {

    /* Disable BURTC if cfg prescaler is not set as wanted. */
    if ((BURTC->CFG & _BURTC_CFG_CNTPRESC_MASK) != EFR32_BURTC_CFG_CNTPRESC) {
      BURTC->CMD_CLR = BURTC_CMD_STOP;
      while ((BURTC->SYNCBUSY & _BURTC_SYNCBUSY_MASK) != 0U);

      BURTC->EN_CLR = BURTC_EN_EN;
      while ((BURTC->EN & _BURTC_EN_DISABLING_MASK) == BURTC_EN_DISABLING);
    }
  }

  /* If calendar has not been initialized yet, then proceed with the
     initial setup.*/
  if ((BURTC->EN & _BURTC_EN_MASK) != BURTC_EN_EN) {

    /* Note that the CFG register can only be written when BURTC
       is disabled. DEBUGRUN keeps the counter running while the
       debugger has the CPU halted (RM 16.3.3). */
    BURTC->CFG = (BURTC->CFG & ~(_BURTC_CFG_CNTPRESC_MASK | _BURTC_CFG_DEBUGRUN_MASK))
                 | EFR32_BURTC_CFG_CNTPRESC | BURTC_CFG_DEBUGRUN;

    /* Enable BURTC. */
    BURTC->EN_SET = BURTC_EN_EN;

    /* Clear counter. */
    BURTC->CNT_CLR = _BURTC_CNT_CNT_MASK;

    #if EFR32_RTC_USE_RTC1 == TRUE
    RTCD1.buram_state->ovf_counter = 0U;
    RTCD1.buram_state->comp_cnt = 0U;
    RTCD1.buram_state->tv_sec = 0U;
    RTCD1.buram_state->tv_msec = 0U;
    RTCD1.buram_state->dstflag = 0U;
    RTCD1.buram_state->magic = EFR32_RTC_BURAM_MAGIC;
    #endif

    /* Arm the COMP calendar tick (CNT was just cleared, comp_cnt = 0). */
    BURTC->COMP = EFR32_RTC_COMP_PERIOD;
    while ((BURTC->SYNCBUSY & BURTC_SYNCBUSY_COMP) != 0U);

    /* Clear interrupt flags. */
    BURTC->IF_CLR = _BURTC_IF_MASK;

    /* Enable interrupts: OF (64-bit extension) + COMP (calendar tick). */
    BURTC->IEN_CLR = _BURTC_IEN_MASK;
    BURTC->IEN_SET = BURTC_IEN_OF | BURTC_IEN_COMP;
  }
  else {

    /* BURTC was already running (warm reset): the backup-domain state in
       BURAM survives, but it may be garbage after a power-on of the backup
       domain — validate it with the magic word and re-anchor if invalid.
       CNT itself keeps counting, only the calendar anchor is reset. */
    #if EFR32_RTC_USE_RTC1 == TRUE
    if (RTCD1.buram_state->magic != EFR32_RTC_BURAM_MAGIC) {
      RTCD1.buram_state->ovf_counter = 0U;
      RTCD1.buram_state->comp_cnt = 0U;
      RTCD1.buram_state->tv_sec = 0U;
      RTCD1.buram_state->tv_msec = 0U;
      RTCD1.buram_state->dstflag = 0U;
      RTCD1.buram_state->magic = EFR32_RTC_BURAM_MAGIC;
    }
    #endif

    /* Clear interrupt flags and make sure both calendar interrupts are
       enabled (backup-domain IEN survives warm resets, but be explicit). */
    BURTC->IF_CLR = _BURTC_IF_MASK;
    BURTC->IEN_SET = BURTC_IEN_OF | BURTC_IEN_COMP;

    /* If BURTC was enabled before, the interrupts ae still enabled
       but inactive because BURTC is not started yet. */
  }

  /* Start BURTC. */
  BURTC->CMD_SET = BURTC_CMD_START;
  while ((BURTC->SYNCBUSY & _BURTC_SYNCBUSY_MASK) != 0U);

  /* IRQ vectors permanently assigned to this driver. */
  nvicEnableVector(EFR32_BURTC_NUMBER, EFR32_BURTC_PRIORITY);
}

__STATIC_INLINE void rtc_lld_set_time_slow(RTCDriver* rtcp, const RTCDateTime* timespec) {

  uint32_t tv_sec, tv_msec;

  /* Number of leap corrections to apply up to end of last year. */
  const int leap_days_to_rtc_base_year = (RTC_BASE_YEAR - 1) / 4 -
                                         (RTC_BASE_YEAR - 1) / 100 +
                                         (RTC_BASE_YEAR - 1) / 400;

  const int last_year = (RTC_BASE_YEAR - 1) + timespec->year;

  const int leap_days_to_last_year = last_year / 4 -
                                     last_year / 100 +
                                     last_year / 400 -
                                     leap_days_to_rtc_base_year;

  const int month = timespec->month - 1; /* 1 .. 12 -> 0 .. 11 */

  int day = timespec->day - 1; /* 1 .. 31 -> 0 .. 30 */

  const int *day_offset_by_month = isleap(RTC_BASE_YEAR + timespec->year) ?
                                   &rtc_day_offset_by_month[1][0] :
                                   &rtc_day_offset_by_month[0][0];

  day += timespec->year * 365 + leap_days_to_last_year +
         day_offset_by_month[month];

  tv_sec = day * 86400 + timespec->millisecond / 1000;
  tv_msec = timespec->millisecond % 1000;

  _rtc_lld_store_time(rtcp, tv_sec, tv_msec, timespec->dstflag);
}

__STATIC_INLINE void rtc_lld_set_time_approx(RTCDriver *rtcp, const RTCDateTime *timespec) {

  uint32_t tv_sec, tv_msec;

  int year = timespec->year;

  const int *day_offset_by_month = isleap(RTC_BASE_YEAR + year) ?
                                   &rtc_day_offset_by_month[1][0] :
                                   &rtc_day_offset_by_month[0][0];

  int month = timespec->month - 1; /* 1 .. 12 -> 0 .. 11 */

  int day = timespec->day - 1; /* 1 .. 31 -> 0 .. 30 */

  day += year * 365 + (year + 3) / 4 + day_offset_by_month[month];

  tv_sec = day * 86400 + timespec->millisecond / 1000;
  tv_msec = timespec->millisecond % 1000;

  _rtc_lld_store_time(rtcp, tv_sec, tv_msec, timespec->dstflag);
}

/**
 * @brief   Set current time.
 * @note    Fractional part of second will be silently ignored.
 * @note    The function can be called from any context.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] timespec  pointer to a @p RTCDateTime structure
 *
 * @notapi
 */
void rtc_lld_set_time(RTCDriver *rtcp, const RTCDateTime *timespec) {

  return rtc_lld_set_time_slow(rtcp, timespec);
}

/**
 * @brief   Get current time.
 * @note    The function can be called from any context.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[out] timespec pointer to a @p RTCDateTime structure
 *
 * @notapi
 */
void rtc_lld_get_time(RTCDriver* rtcp, RTCDateTime* timespec) {

  syssts_t sts;
  uint32_t ovf_counter, comp_cnt, comp_cnt2;
  uint32_t tv_sec, tv_msec, dstflag;

  /* Entering a reentrant critical zone.*/
  sts = osalSysGetStatusAndLockX();

  /* Seconds come from BURAM state only (written from ISR context):
     ovf_counter from the OF interrupt, comp_cnt from the COMP interrupt —
     never from CNT, whose reads lag the real counter when the LF domain
     sits idle. Sub-second ms (if EFR32_RTC_HAS_SUBSECONDS) comes from one
     CNT read — safe here because the 1 Hz COMP tick keeps the LF sync path
     warm (verified on target: reads then track the real 32768 Hz rate).
     Re-read if a COMP fired mid-read so seconds and ms stay consistent. */
  do {
    ovf_counter = rtcp->buram_state->ovf_counter;
    comp_cnt    = rtcp->buram_state->comp_cnt;
#if EFR32_RTC_HAS_SUBSECONDS
    uint32_t cnt = BURTC->CNT;
#endif
    comp_cnt2   = rtcp->buram_state->comp_cnt;
#if EFR32_RTC_HAS_SUBSECONDS
    if (comp_cnt2 == comp_cnt) {
      tv_msec = rtcp->buram_state->tv_msec + (cnt % EFR32_RTCCLK) * 1000U / EFR32_RTCCLK;
    } else {
      tv_msec = rtcp->buram_state->tv_msec;
    }
#else
    tv_msec = rtcp->buram_state->tv_msec;
#endif
  } while (comp_cnt2 != comp_cnt);

  tv_sec  = rtcp->buram_state->tv_sec;
  dstflag = rtcp->buram_state->dstflag;

  /* Leaving a reentrant critical zone.*/
  osalSysRestoreStatusX(sts);

  tv_sec += (ovf_counter * EFR32_RTC_OVERFLOW_INTERVAL) +
            (ovf_counter * EFR32_RTC_OVERFLOW_INTERVAL_REMAINDER) / EFR32_RTCCLK +
            comp_cnt;

  tv_sec += tv_msec / 1000U;
  tv_msec = tv_msec % 1000U;

  _rtc_lld_to_timespec(tv_sec, tv_msec, timespec, dstflag);
}


/**
 * @brief   Enables or disables RTC callbacks.
 * @details This function enables or disables callbacks, use a @p NULL pointer
 *          in order to disable a callback.
 * @note    The function can be called from any context.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] callback  callback function pointer or @p NULL
 *
 * @notapi
 */

#if (RTC_ALARMS > 0) || defined(__DOXYGEN__)
#if !defined(EFR32_SYSRTC_USE_SYSRTC) || (EFR32_SYSRTC_USE_SYSRTC == 0)
#error "BURTCv1 alarms are implemented on SYSRTC CMP1 — define EFR32_SYSRTC_USE_SYSRTC TRUE"
#endif

/**
 * @brief   SYSRTC CMP1 event -> RTC alarm callback.
 * @details CMP0/OVF of SYSRTC group 0 belong to SYSTICKv2; CMP1 is the RTC
 *          alarm. The SEQ vector is shared: SYSTICKv2 forwards to
 *          sysrtc_lld_serve_interrupt(), which calls this.
 *
 * @isr
 */
static void rtc_lld_sysrtc_cb(efr32_sysrtc_evt_t evt, uint32_t value, void *arg) {
  (void)value;
  (void)arg;

  if (evt == EFR32_SYSRTC_EVT_CMP1) {
    /* One-shot: disable the compare channel. */
    sysrtc_lld_disable_compare(&SYSRTCD1, 1U);

    #if EFR32_RTC_USE_RTC1 == TRUE
    if (RTCD1.callback != NULL) {
      RTCD1.callback(&RTCD1, RTC_EVENT_ALARM);
    }
    #endif
  }
}

/**
 * @brief   Set alarm time (relative offset from now).
 * @details Implemented on SYSRTC CMP1: the compare value is
 *          CNT + offset, where CNT ticks at CH_CFG_ST_FREQUENCY (the
 *          SYSTICKv2/SYSRTC tick rate).
 *
 * @param[in] rtcp       pointer to RTC driver structure
 * @param[in] alarm      alarm number, must be 0
 * @param[in] alarmspec  pointer to an @p RTCAlarm structure, NULL to disable
 *
 * @notapi
 */
void rtc_lld_set_alarm(RTCDriver* rtcp, rtcalarm_t alarm,
                       const RTCAlarm* alarmspec) {
  syssts_t sts;

  (void)rtcp;
  osalDbgAssert(alarm == 0, "unknown alarm");

  /* Entering a reentrant critical zone.*/
  sts = osalSysGetStatusAndLockX();

  if (alarmspec != NULL) {
    uint32_t cnt = sysrtc_lld_get_counter();
    uint32_t delta = alarmspec->tv_sec * CH_CFG_ST_FREQUENCY +
                     (uint32_t)(((uint64_t)alarmspec->tv_usec * CH_CFG_ST_FREQUENCY) / 1000000ULL);
    sysrtc_lld_set_compare(&SYSRTCD1, 1U, cnt + delta);   /* wraps naturally */
    sysrtc_lld_enable_compare(&SYSRTCD1, 1U);
  }
  else {
    sysrtc_lld_disable_compare(&SYSRTCD1, 1U);
  }

  /* Leaving a reentrant critical zone.*/
  osalSysRestoreStatusX(sts);
}

/**
 * @brief   Get alarm time (remaining offset from now).
 *
 * @notapi
 */
void rtc_lld_get_alarm(RTCDriver* rtcp, rtcalarm_t alarm,
                       RTCAlarm* alarmspec) {
  syssts_t sts;
  uint32_t cnt, comp;

  (void)rtcp;
  osalDbgAssert(alarm == 0, "unknown alarm");

  sts = osalSysGetStatusAndLockX();

  comp = sysrtc_lld_get_compare(&SYSRTCD1, 1U);
  cnt  = sysrtc_lld_get_counter();

  osalSysRestoreStatusX(sts);

  uint32_t diff = comp - cnt;   /* wraps naturally */

  alarmspec->tv_sec  = diff / CH_CFG_ST_FREQUENCY;
  alarmspec->tv_usec = (uint32_t)(((uint64_t)(diff % CH_CFG_ST_FREQUENCY) * 1000000ULL) / CH_CFG_ST_FREQUENCY);
}
#endif /* RTC_ALARMS > 0 */

void rtc_lld_set_callback(RTCDriver* rtcp, rtccb_t callback) {

  syssts_t sts;

  /* Entering a reentrant critical zone.*/
  sts = osalSysGetStatusAndLockX();

  rtcp->callback = callback;

  /* Leaving a reentrant critical zone.*/
  osalSysRestoreStatusX(sts);
}

#endif /* HAL_USE_RTC && HAL_USE_BURAM */

#endif /* HAL_USE_RTC */

/** @} */
