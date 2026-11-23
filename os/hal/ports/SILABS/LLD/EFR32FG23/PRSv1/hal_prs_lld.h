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
 * @brief   EFR32FG23 PRS LLD for the generic hal_prs driver.
 *
 * @details Channel-based crossbar: 12 async channels (ASYNC_CH, low-power
 *          capable) and 4 sync channels (SYNC_CH, HFPERCLK domain). A channel
 *          holds a source (SOURCESEL|SIGSEL, re-exported here as PRS_GEN_*);
 *          consumers attach via CONSUMER_* registers (PRS_CON_*) and pins
 *          via GPIO->PRSROUTE[0]. Software events via ASYNC_SWPULSE/
 *          ASYNC_SWLEVEL, channel levels via ASYNC_PEEK/SYNC_PEEK, per-
 *          channel logic via FNSEL (AUXSEL picks the second channel).
 *
 * @addtogroup PRS
 * @{
 */

#ifndef HAL_PRS_LLD_H
#define HAL_PRS_LLD_H

#if (HAL_USE_PRS == TRUE) || defined(__DOXYGEN__)

#if !defined(EFR32_PRS_USE_PRS) || defined(__DOXYGEN__)
#define EFR32_PRS_USE_PRS       FALSE
#endif

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/** Number of async channels (low-power routes). */
#define PRS_NUM_ASYNC_CH        12U
/** Number of sync channels (cycle-accurate routes). */
#define PRS_NUM_SYNC_CH         4U

/* Pad mode for a PRS-driven output pin: peripheral outputs need an *_ALT
   pad mode on xG23, otherwise the GPIO DOUT wins. */
#define PRS_PAD_MODE_OUT        (PAL_EFR32_MODE_PUSHPULL_ALT | \
                                 PAL_EFR32_DOUT_FLOATING)

/*===========================================================================*/
/* Event generator (source) encodings — re-exported from the device header. */
/*===========================================================================*/

/* Async generators (combined SOURCESEL|SIGSEL, as PRS_ASYNC_* in the
   device header). Examples: */
#define PRS_GEN_TIMER0_CC0      PRS_ASYNC_TIMER0_CC0
#define PRS_GEN_TIMER0_UF       PRS_ASYNC_TIMER0_UF
#define PRS_GEN_LETIMER0_CH0    PRS_ASYNC_LETIMER0_CH0
#define PRS_GEN_LESENSE_DECOUT0 PRS_ASYNC_LESENSE_DECOUT0
#define PRS_GEN_LESENSE_DECCMP  PRS_ASYNC_LESENSE_DECCMP
#define PRS_GEN_GPIO_PIN0       PRS_ASYNC_GPIO_PIN0
/* Sync generators (sync encoding). */
#define PRS_GEN_SYNC_TIMER0_CC0 PRS_SYNC_TIMER0_CC0

/*===========================================================================*/
/* Event consumer encodings (CONSUMER_* registers, channel field).         */
/*===========================================================================*/

/* The channel number is written into the PRSSEL field (bits 0..3) of the
   selected CONSUMER_* register by prs_lld_connect_consumer(). */
#define PRS_CON_LETIMER0_START  0x01U
#define PRS_CON_LETIMER0_STOP   0x02U
#define PRS_CON_LETIMER0_CLEAR  0x03U
#define PRS_CON_LESENSE_START   0x04U
#define PRS_CON_IADC0_SINGLETRIGGER 0x05U
#define PRS_CON_IADC0_SCANTRIGGER   0x06U

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   PRS driver configuration (LLD part). No static config needed;
 *          routes are programmed at runtime.
 */
typedef struct {
  uint32_t reserved;
} PRSConfig;

struct PRSDriver {
  prsstate_t          state;
  const PRSConfig    *config;
};

#if (EFR32_PRS_USE_PRS == TRUE) || defined(__DOXYGEN__)
extern PRSDriver PRSD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif

  void     prs_lld_init(void);
  void     prs_lld_start(PRSDriver *pp);
  void     prs_lld_stop(PRSDriver *pp);
  void     prs_lld_connect_async(PRSDriver *pp, uint8_t ch, prs_source_t gen);
  void     prs_lld_connect_sync(PRSDriver *pp, uint8_t ch, prs_source_t gen);
  void     prs_lld_disconnect_async(PRSDriver *pp, uint8_t ch);
  void     prs_lld_disconnect_sync(PRSDriver *pp, uint8_t ch);
  void     prs_lld_set_logic(PRSDriver *pp, uint8_t ch, prs_fn_t fn,
                             uint8_t aux);
  void     prs_lld_connect_consumer(PRSDriver *pp, prs_consumer_t consumer,
                                    prs_path_t path, uint8_t ch);
  void     prs_lld_sw_pulse(PRSDriver *pp, uint16_t ch_mask);
  void     prs_lld_sw_level_set(PRSDriver *pp, uint16_t ch_mask);
  void     prs_lld_sw_level_clear(PRSDriver *pp, uint16_t ch_mask);
  uint32_t prs_lld_peek_async(PRSDriver *pp);
  uint32_t prs_lld_peek_sync(PRSDriver *pp);
  void     prs_lld_route_async_pin(PRSDriver *pp, uint8_t ch,
                                   ioportid_t port, ioportmask_t pad);
  void     prs_lld_route_sync_pin(PRSDriver *pp, uint8_t ch,
                                  ioportid_t port, ioportmask_t pad);
  void     prs_lld_unroute_async_pin(PRSDriver *pp, uint8_t ch);
  void     prs_lld_unroute_sync_pin(PRSDriver *pp, uint8_t ch);

#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_PRS */

#endif /* HAL_PRS_LLD_H */

/** @} */
