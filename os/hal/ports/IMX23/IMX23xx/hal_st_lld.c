/*
    ChibiOS - Copyright (C) 2006-2026 Xael South.

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
 * @file    IMX23/IMX23xx/hal_st_lld.c
 * @brief   i.MX23 ST subsystem low level driver code (TIMROT).
 *
 * @addtogroup IMX23
 * @{
 */

#include "hal.h"
#include "icoll.h"

#if (OSAL_ST_MODE != OSAL_ST_MODE_NONE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

#define ST_TIMER_COUNTER        0U      /* TIMROT0: free-running counter    */
#define ST_TIMER_ALARM          1U      /* TIMROT1: one-shot alarm          */

#define ST_HW_MAX               0xFFFFU /* 16-bit timers on the i.MX23      */

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/* High word of the 32-bit software-extended counter. */
static volatile uint32_t st_counter_high;

/* Shadow of the alarm timer control register (IRQ bit is write-0-to-clear). */
static uint32_t st_alarm_ctrl;

/* Current alarm time and status. */
static systime_t st_alarm_time;
static bool st_alarm_active;

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

static inline void st_clear_irq(unsigned timer) {

  /* TIMCTRL IRQ bit is cleared by writing 0 to it. */
  IMX23_REG(IMX23_TIMROT_TIMCTRL(timer)) =
      IMX23_REG(IMX23_TIMROT_TIMCTRL(timer)) & ~IMX23_TIMCTRL_IRQ;
}

/**
 * @brief   Programs the alarm timer for the current alarm time.
 */
static void st_program_alarm(void) {
  systime_t now;
  uint32_t delta;

  now = st_lld_get_counter();
  delta = (uint32_t)(st_alarm_time - now);
  if (delta == 0U) {
    delta = 1U;
  }
  if (delta > ST_HW_MAX) {
    delta = ST_HW_MAX;      /* re-armed by the ST layer when it fires early */
  }

  /* One-shot: no RELOAD, count down from delta, IRQ at zero. */
  IMX23_REG(IMX23_TIMROT_TIMCOUNT(ST_TIMER_ALARM)) = delta;
  st_alarm_ctrl = IMX23_TIMCTRL_IRQ_EN | IMX23_TIMCTRL_UPDATE |
                  IMX23_ST_PRESCALE | IMX23_ST_SELECT;
  IMX23_REG(IMX23_TIMROT_TIMCTRL(ST_TIMER_ALARM)) = st_alarm_ctrl;
}

/*===========================================================================*/
/* Module interrupt handlers.                                                */
/*===========================================================================*/

/* TIMROT0: counter wrap, bump the high word. */
CH_IRQ_HANDLER(IMX23_TIMROT0_IRQHandler) {

  CH_IRQ_PROLOGUE();

  st_clear_irq(ST_TIMER_COUNTER);
  st_counter_high++;

  CH_IRQ_EPILOGUE();
}

/* TIMROT1: alarm fired. */
CH_IRQ_HANDLER(IMX23_TIMROT1_IRQHandler) {

  CH_IRQ_PROLOGUE();

  st_clear_irq(ST_TIMER_ALARM);
  st_alarm_active = false;

  osalSysLockFromISR();
  osalOsTimerHandlerI();
  osalSysUnlockFromISR();

  CH_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level ST driver initialization.
 *
 * @notapi
 */
void st_lld_init(void) {

  st_counter_high = 0U;
  st_alarm_time   = 0U;
  st_alarm_active = false;

  /* Reset the TIMROT block and ungate its clock. */
  IMX23_REG(IMX23_TIMROT_ROTCTRL) = IMX23_TIMROT_ROTCTRL_SFTRST;
  while ((IMX23_REG(IMX23_TIMROT_ROTCTRL) & IMX23_TIMROT_ROTCTRL_SFTRST) != 0U) {
  }
  IMX23_REG_CLR(IMX23_TIMROT_ROTCTRL, IMX23_TIMROT_ROTCTRL_CLKGATE);

  /* Timer 0: free-running 16-bit down-counter, wrap IRQ enabled. */
  IMX23_REG(IMX23_TIMROT_TIMCOUNT(ST_TIMER_COUNTER)) = ST_HW_MAX;
  IMX23_REG(IMX23_TIMROT_TIMCTRL(ST_TIMER_COUNTER)) =
      IMX23_TIMCTRL_IRQ_EN | IMX23_TIMCTRL_RELOAD | IMX23_TIMCTRL_UPDATE |
      IMX23_ST_PRESCALE | IMX23_ST_SELECT;

  /* Timer 1: alarm, stopped for now. */
  IMX23_REG(IMX23_TIMROT_TIMCTRL(ST_TIMER_ALARM)) =
      IMX23_ST_PRESCALE | IMX23_ST_SELECT;

  icoll_register_handler(IMX23_IRQ_TIMROT0, IMX23_TIMROT0_IRQHandler);
  icoll_register_handler(IMX23_IRQ_TIMROT1, IMX23_TIMROT1_IRQHandler);
  icoll_irq_set_priority(IMX23_IRQ_TIMROT1, 3U);
  icoll_irq_enable(IMX23_IRQ_TIMROT0);
  icoll_irq_enable(IMX23_IRQ_TIMROT1);
}

/**
 * @brief   Returns the time counter value.
 *
 * @return              The counter value (32-bit, ticks at IMX23_ST_FREQ).
 *
 * @notapi
 */
systime_t st_lld_get_counter(void) {
  uint32_t h1, c, h2;

  do {
    h1 = st_counter_high;
    c  = ST_HW_MAX - IMX23_TIMCOUNT_RUNNING(
                      IMX23_REG(IMX23_TIMROT_TIMCOUNT(ST_TIMER_COUNTER)));
    h2 = st_counter_high;
  } while (h1 != h2);

  return ((systime_t)h1 << 16) | (systime_t)c;
}

/**
 * @brief   Starts the alarm.
 *
 * @param[in] time      the time to be set for the first alarm
 *
 * @notapi
 */
void st_lld_start_alarm(systime_t time) {

  st_alarm_time   = time;
  st_alarm_active = true;
  st_program_alarm();
}

/**
 * @brief   Stops the alarm interrupt.
 *
 * @notapi
 */
void st_lld_stop_alarm(void) {

  st_alarm_ctrl &= ~IMX23_TIMCTRL_IRQ_EN;
  IMX23_REG(IMX23_TIMROT_TIMCTRL(ST_TIMER_ALARM)) = st_alarm_ctrl;
  st_alarm_active = false;
}

/**
 * @brief   Sets the alarm time.
 *
 * @param[in] time      the time to be set for the next alarm
 *
 * @notapi
 */
void st_lld_set_alarm(systime_t time) {

  st_alarm_time = time;
  st_program_alarm();
}

/**
 * @brief   Returns the current alarm time.
 *
 * @return              The currently set alarm time.
 *
 * @notapi
 */
systime_t st_lld_get_alarm(void) {

  return st_alarm_time;
}

/**
 * @brief   Determines if the alarm is active.
 *
 * @return              The alarm status.
 *
 * @notapi
 */
bool st_lld_is_alarm_active(void) {

  return st_alarm_active;
}

#endif /* OSAL_ST_MODE != OSAL_ST_MODE_NONE */

/** @} */
