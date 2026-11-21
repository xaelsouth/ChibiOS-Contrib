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
 * @file    IMX23/IMX23xx/hal_st_lld.h
 * @brief   i.MX23 ST subsystem low level driver header.
 *
 * @details TIMROT timer 0 is used as a free-running 16-bit down-counter
 *          (software extended to 32 bits via its wrap interrupt), timer 1
 *          as the one-shot alarm. Both tick at IMX23_ST_FREQ.
 *
 * @addtogroup IMX23
 * @{
 */

#ifndef HAL_ST_LLD_H
#define HAL_ST_LLD_H

#include "imx23.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/* Timers run from the 32kHz crystal (as Linux does on the i.MX23). */
#if !defined(IMX23_ST_PRESCALE) || defined(__DOXYGEN__)
#define IMX23_ST_PRESCALE       IMX23_TIMCTRL_PRESCALE_DIV_BY_1
#endif
#define IMX23_ST_SELECT         IMX23_TIMCTRL_SELECT_32KHZ_XTAL

/* ST counter frequency: must match CH_CFG_ST_FREQUENCY. */
#define IMX23_ST_FREQ           IMX23_TIMROT_TICK_HZ

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
  systime_t st_lld_get_counter(void);
  void st_lld_start_alarm(systime_t time);
  void st_lld_stop_alarm(void);
  void st_lld_set_alarm(systime_t time);
  systime_t st_lld_get_alarm(void);
  bool st_lld_is_alarm_active(void);
#ifdef __cplusplus
}
#endif

#endif /* HAL_ST_LLD_H */

/** @} */
