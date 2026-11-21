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
 * @file    SYSRTCv1/hal_sysrtc_lld.h
 * @brief   EFR32FG23 SYSRTC low-level driver header.
 *
 * @details SYSRTC is a 32-bit counter running from an LF clock (LFXO/LFRCO/
 *          ULFRCO). It provides 2 compare (alarm) channels and 1 capture
 *          channel in group 0, plus an overflow flag. All events can
 *          invoke callbacks from the SEQ interrupt.
 *
 * @addtogroup EFR32_SYSRTC
 * @{
 */

#ifndef HAL_SYSRTC_LLD_H
#define HAL_SYSRTC_LLD_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Number of compare (alarm) channels.
 */
#define EFR32_SYSRTC_NUM_COMPARE    2U

/**
 * @brief   Number of capture channels.
 */
#define EFR32_SYSRTC_NUM_CAPTURE    1U

/**
 * @brief   SYSRTC clock source selection.
 */
#define EFR32_SYSRTC_CLKSEL_LFRCO   1U
#define EFR32_SYSRTC_CLKSEL_LFXO    2U
#define EFR32_SYSRTC_CLKSEL_ULFRCO  3U

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

#if !defined(EFR32_SYSRTC_USE_SYSRTC) || defined(__DOXYGEN__)
#define EFR32_SYSRTC_USE_SYSRTC     FALSE
#endif

#if !defined(EFR32_SYSRTC_CLK_SOURCE) || defined(__DOXYGEN__)
#define EFR32_SYSRTC_CLK_SOURCE     EFR32_SYSRTC_CLKSEL_LFXO
#endif

#if !defined(EFR32_SYSRTC_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_SYSRTC_IRQ_PRIORITY   4
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   SYSRTC event types.
 */
typedef enum {
  EFR32_SYSRTC_EVT_OVF   = 0,   /**< Counter overflow (2^32 ticks). */
  EFR32_SYSRTC_EVT_CMP0  = 1,   /**< Compare channel 0 match.       */
  EFR32_SYSRTC_EVT_CMP1  = 2,   /**< Compare channel 1 match.       */
  EFR32_SYSRTC_EVT_CAP0  = 3,   /**< Capture channel 0 triggered.   */
} efr32_sysrtc_evt_t;

/**
 * @brief   SYSRTC event callback type.
 *
 * @param[in] evt       the event that occurred
 * @param[in] value     for CMP events: the compare value; for CAP: captured CNT
 * @param[in] arg       user argument
 */
typedef void (*efr32_sysrtc_cb_t)(efr32_sysrtc_evt_t evt, uint32_t value, void *arg);

/**
 * @brief   SYSRTC driver structure.
 */
typedef struct {
  /**
   * @brief   Driver state: 0 = stopped, 1 = running.
   */
  uint32_t                  state;
  /**
   * @brief   Callback for compare/capture/overflow events, or NULL.
   */
  efr32_sysrtc_cb_t         callback;
  /**
   * @brief   Callback argument.
   */
  void                      *callback_arg;
} SYSRTCDriver;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if EFR32_SYSRTC_USE_SYSRTC && !defined(__DOXYGEN__)
extern SYSRTCDriver SYSRTCD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void sysrtc_lld_init(void);
  void sysrtc_lld_start(SYSRTCDriver *drvp, efr32_sysrtc_cb_t cb, void *arg);
  void sysrtc_lld_stop(SYSRTCDriver *drvp);

  /* Counter access. */
  uint32_t sysrtc_lld_get_counter(void);
  void sysrtc_lld_set_counter(uint32_t value);

  /* Compare (alarm) channels. channel: 0 or 1. */
  void sysrtc_lld_set_compare(SYSRTCDriver *drvp, uint32_t channel, uint32_t value);
  uint32_t sysrtc_lld_get_compare(SYSRTCDriver *drvp, uint32_t channel);
  void sysrtc_lld_enable_compare(SYSRTCDriver *drvp, uint32_t channel);
  void sysrtc_lld_disable_compare(SYSRTCDriver *drvp, uint32_t channel);

  /* Capture channel (channel: 0). */
  void sysrtc_lld_enable_capture(SYSRTCDriver *drvp, uint32_t channel);
  void sysrtc_lld_disable_capture(SYSRTCDriver *drvp, uint32_t channel);
  uint32_t sysrtc_lld_get_capture(SYSRTCDriver *drvp, uint32_t channel);

  /* Overflow interrupt. */
  void sysrtc_lld_enable_ovf_irq(SYSRTCDriver *drvp);
  void sysrtc_lld_disable_ovf_irq(SYSRTCDriver *drvp);

  void sysrtc_lld_serve_interrupt(SYSRTCDriver *drvp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_SYSRTC_LLD_H */

/** @} */
