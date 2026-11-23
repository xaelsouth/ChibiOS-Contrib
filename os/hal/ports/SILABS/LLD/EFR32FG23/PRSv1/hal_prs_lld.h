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
 * @file    PRSv1/hal_prs_lld.h
 * @brief   EFR32FG23 PRS (Peripheral Reflex System) LLD header.
 *
 * @details The xG23 PRS routes peripheral output signals to other
 *          peripherals (consumers) or to GPIO pins without CPU
 *          intervention. 12 asynchronous channels (ASYNC_CH[0..11]) and
 *          4 synchronous channels (SYNC_CH[0..3], clocked by HFPERCLK, usable
 *          in EM0/EM1 only) are available. Async channels support a logic
 *          function (FNSEL) combining the channel signal with an auxiliary
 *          input (AUXSEL).
 *
 *          Signals are selected with the pre-shifted combined macros from
 *          efr32fg23_prs_signals.h, e.g. PRS_ASYNC_TIMER0_CC0 or
 *          PRS_SYNC_GPIO_PIN0. Pin output goes through GPIO->PRSROUTE[0]
 *          (ASYNCHnROUTE/SYNCHnROUTE + ROUTEEN pen bit); the GPIO pad itself
 *          must be configured as push-pull output by the caller (pal).
 *
 *          PRS has no EN register and no interrupt; it only needs its bus
 *          clock (CMU CLKEN0.PRS). This is an LLD-only driver (no portable
 *          HAL PRS model exists); enable with EFR32_PRS_USE_PRS TRUE.
 *
 * @addtogroup PRS
 * @{
 */

#ifndef HAL_PRS_LLD_H
#define HAL_PRS_LLD_H

#if !defined(EFR32_PRS_USE_PRS) || defined(__DOXYGEN__)
#define EFR32_PRS_USE_PRS   FALSE
#endif

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/** Number of asynchronous PRS channels. */
#define EFR32_PRS_NUM_ASYNC_CH    PRS_ASYNC_CH_NUM   /* 12 */
/** Number of synchronous PRS channels. */
#define EFR32_PRS_NUM_SYNC_CH     PRS_SYNC_CH_NUM    /* 4  */

/** Pad mode for a PRS-driven output pin: on xG23 a peripheral output only
    reaches the pad when the pad MODE is an *_ALT variant; plain PUSHPULL
    lets the GPIO DOUT win and the PRS signal never appears on the pin. */
#define EFR32_PRS_PAD_MODE_OUT    (PAL_EFR32_MODE_PUSHPULL_ALT | \
                                   PAL_EFR32_DOUT_FLOATING)

#ifdef __cplusplus
extern "C" {
#endif

  void     prs_lld_init(void);
  void     prs_lld_start(void);
  void     prs_lld_stop(void);

  /* Async: signal = PRS_ASYNC_* combined macro (SOURCESEL|SIGSEL),
     passed through with FNSEL_A. */
  void     prs_lld_connect_async(uint8_t ch, uint32_t signal);
  /* Async with logic function: fn = PRS_ASYNC_CH_CTRL_FNSEL_* macro
     (e.g. PRS_ASYNC_CH_CTRL_FNSEL_NOT_A for inversion). */
  void     prs_lld_connect_async_fn(uint8_t ch, uint32_t signal, uint32_t fn);
  void     prs_lld_disconnect_async(uint8_t ch);

  /* Sync: signal = PRS_SYNC_* combined macro (sync encoding). */
  void     prs_lld_connect_sync(uint8_t ch, uint32_t signal);
  void     prs_lld_disconnect_sync(uint8_t ch);

  /* Software-generated signals on async channels (bit mask of channels). */
  void     prs_lld_sw_pulse(uint16_t ch_mask);
  void     prs_lld_sw_level_set(uint16_t ch_mask);
  void     prs_lld_sw_level_clear(uint16_t ch_mask);

  /* Channel value peek (read-only). */
  uint32_t prs_lld_peek_async(void);
  uint32_t prs_lld_peek_sync(void);

  /* Route a channel to a GPIO pin (caller sets the pad to push-pull out). */
  void     prs_lld_route_async_pin(uint8_t ch, ioportid_t port, ioportmask_t pad);
  void     prs_lld_route_sync_pin(uint8_t ch, ioportid_t port, ioportmask_t pad);
  void     prs_lld_unroute_async_pin(uint8_t ch);
  void     prs_lld_unroute_sync_pin(uint8_t ch);

#ifdef __cplusplus
}
#endif

#endif /* HAL_PRS_LLD_H */

/** @} */
