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
 * @file    SIGROUTEv1/hal_sigroute_lld.h
 * @brief   EFR32FG23 PRS hardware LLD for the generic hal_sigroute driver.
 *
 * @details Channel-based crossbar: 12 async channels (ASYNC_CH, low-power
 *          capable) and 4 sync channels (SYNC_CH, HFPERCLK domain). A channel
 *          holds a source (SOURCESEL|SIGSEL, re-exported here as SIGROUTE_GEN_*);
 *          consumers attach via CONSUMER_* registers (SIGROUTE_CON_*) and pins
 *          via GPIO->PRSROUTE[0]. Software events via ASYNC_SWPULSE/
 *          ASYNC_SWLEVEL, channel levels via ASYNC_PEEK/SYNC_PEEK, per-
 *          channel logic via FNSEL (AUXSEL picks the second channel).
 *          NOTE: despite the SIGROUTEv1 name, this LLD drives the
 *          Silicon Labs PRS (Peripheral Reflex System) peripheral —
 *          all register/field macros (PRS->, PRS_ASYNC_*, PRS_SYNC_*,
 *          _PRS_*, CMU_CLKEN0_PRS, GPIO_PRS_*) are PRS hardware names.
 *
 * @addtogroup SIGROUTE
 * @{
 */

#ifndef HAL_SIGROUTE_LLD_H
#define HAL_SIGROUTE_LLD_H

#if (HAL_USE_SIGROUTE == TRUE) || defined(__DOXYGEN__)

#if !defined(EFR32_SIGROUTE_USE_PRS) || defined(__DOXYGEN__)
#define EFR32_SIGROUTE_USE_PRS       FALSE
#endif

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/** Number of async channels (low-power routes). */
#define SIGROUTE_NUM_ASYNC_CH        12U
/** Number of sync channels (cycle-accurate routes). */
#define SIGROUTE_NUM_SYNC_CH         4U

/* Pad mode for a PRS-driven output pin: peripheral outputs need an *_ALT
   pad mode on xG23, otherwise the GPIO DOUT wins. */
#define SIGROUTE_PAD_MODE_OUT        (PAL_EFR32_MODE_PUSHPULL_ALT | \
                                 PAL_EFR32_DOUT_FLOATING)

/*===========================================================================*/
/* Event generator (source) encodings — re-exported from the device header. */
/*===========================================================================*/

/* Async generators (combined SOURCESEL|SIGSEL, as PRS_ASYNC_* in the
   device header). Examples: */
#define SIGROUTE_GEN_TIMER0_CC0      PRS_ASYNC_TIMER0_CC0
#define SIGROUTE_GEN_TIMER0_UF       PRS_ASYNC_TIMER0_UF
#define SIGROUTE_GEN_LETIMER0_CH0    PRS_ASYNC_LETIMER0_CH0
#define SIGROUTE_GEN_LESENSE_DECOUT0 PRS_ASYNC_LESENSE_DECOUT0
#define SIGROUTE_GEN_LESENSE_DECCMP  PRS_ASYNC_LESENSE_DECCMP
#define SIGROUTE_GEN_GPIO_PIN0       PRS_ASYNC_GPIO_PIN0
/* Sync generators (sync encoding). */
#define SIGROUTE_GEN_SYNC_TIMER0_CC0 PRS_SYNC_TIMER0_CC0

/*===========================================================================*/
/* Event consumer encodings (CONSUMER_* registers, channel field).         */
/*===========================================================================*/

/* The channel number is written into the PRSSEL field (bits 0..3) of the
   selected CONSUMER_* register by sigroute_lld_connect_consumer(). */
#define SIGROUTE_CON_LETIMER0_START  0x01U
#define SIGROUTE_CON_LETIMER0_STOP   0x02U
#define SIGROUTE_CON_LETIMER0_CLEAR  0x03U
#define SIGROUTE_CON_LESENSE_START   0x04U
#define SIGROUTE_CON_IADC0_SINGLETRIGGER 0x05U
#define SIGROUTE_CON_IADC0_SCANTRIGGER   0x06U

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   SIGROUTE driver configuration (LLD part). No static config needed;
 *          routes are programmed at runtime.
 */
typedef struct {
  uint32_t reserved;
} SIGROUTEConfig;

struct SIGROUTEDriver {
  sigroutestate_t          state;
  const SIGROUTEConfig    *config;
};

#if (EFR32_SIGROUTE_USE_PRS == TRUE) || defined(__DOXYGEN__)
extern SIGROUTEDriver SIGROUTED1;
#endif

#ifdef __cplusplus
extern "C" {
#endif

  void     sigroute_lld_init(void);
  void     sigroute_lld_start(SIGROUTEDriver *pp);
  void     sigroute_lld_stop(SIGROUTEDriver *pp);
  void     sigroute_lld_connect_async(SIGROUTEDriver *pp, uint8_t ch, sigroute_source_t gen);
  void     sigroute_lld_connect_sync(SIGROUTEDriver *pp, uint8_t ch, sigroute_source_t gen);
  void     sigroute_lld_disconnect_async(SIGROUTEDriver *pp, uint8_t ch);
  void     sigroute_lld_disconnect_sync(SIGROUTEDriver *pp, uint8_t ch);
  void     sigroute_lld_set_logic(SIGROUTEDriver *pp, uint8_t ch, sigroute_fn_t fn,
                             uint8_t aux);
  void     sigroute_lld_connect_consumer(SIGROUTEDriver *pp, sigroute_consumer_t consumer,
                                    sigroute_path_t path, uint8_t ch);
  void     sigroute_lld_sw_pulse(SIGROUTEDriver *pp, uint16_t ch_mask);
  void     sigroute_lld_sw_level_set(SIGROUTEDriver *pp, uint16_t ch_mask);
  void     sigroute_lld_sw_level_clear(SIGROUTEDriver *pp, uint16_t ch_mask);
  uint32_t sigroute_lld_peek_async(SIGROUTEDriver *pp);
  uint32_t sigroute_lld_peek_sync(SIGROUTEDriver *pp);
  void     sigroute_lld_route_async_pin(SIGROUTEDriver *pp, uint8_t ch,
                                   ioportid_t port, ioportmask_t pad);
  void     sigroute_lld_route_sync_pin(SIGROUTEDriver *pp, uint8_t ch,
                                  ioportid_t port, ioportmask_t pad);
  void     sigroute_lld_unroute_async_pin(SIGROUTEDriver *pp, uint8_t ch);
  void     sigroute_lld_unroute_sync_pin(SIGROUTEDriver *pp, uint8_t ch);

#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SIGROUTE */

#endif /* HAL_SIGROUTE_LLD_H */

/** @} */
