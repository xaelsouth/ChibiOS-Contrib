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
 * @file    hal_prs.h
 * @brief   Generic peripheral reflex/routing driver (upper layer).
 *
 * @details Portable abstraction of a peripheral signal crossbar, modelled
 *          on the channel-based hardware found on Silicon Labs (PRS),
 *          Microchip SAM/AVR (EVSYS) and others:
 *
 *            source (generator) --[channel]--> consumer (peripheral)
 *                                             \-> GPIO pin
 *
 *          Consumer-centric hardware (NXP TRGMUX/INPUTMUX) maps a "channel"
 *          onto one of the consumer's trigger slots.
 *
 *          Portable concepts:
 *          - prs_source_t   : event generator encoding (vendor specific,
 *                             e.g. PRS_GEN_TIMER0_CC0 in the LLD header)
 *          - prs_consumer_t : event consumer encoding (vendor specific)
 *          - channel index  : async (low-power capable) or sync channel
 *          - prs_fn_t       : optional per-channel logic function
 *          - software events (pulse/level) and channel peek
 *
 * @addtogroup PRS
 * @{
 */

#ifndef HAL_PRS_H_
#define HAL_PRS_H_

#include "hal.h"

#if (HAL_USE_PRS == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Route path: async channels keep working in low-power modes
 *          (EM2 on EFR32), sync channels are cycle-accurate (HFPERCLK).
 */
typedef enum {
  PRS_PATH_ASYNC = 0,   /**< Low-power capable route. */
  PRS_PATH_SYNC  = 1,   /**< Synchronous, cycle-accurate route. */
} prs_path_t;

/**
 * @brief   Event generator (source) encoding. Vendor specific values are
 *          provided by the LLD header as PRS_GEN_* macros.
 */
typedef uint32_t prs_source_t;

/**
 * @brief   Event consumer encoding. Vendor specific values are provided by
 *          the LLD header as PRS_CON_* macros.
 */
typedef uint32_t prs_consumer_t;

/**
 * @brief   Optional per-channel logic function combining the channel
 *          signal A with an auxiliary channel signal B (PRS FNSEL).
 */
typedef enum {
  PRS_FN_PASS_A = 0,    /**< A (plain pass-through). */
  PRS_FN_NOT_A,         /**< ~A */
  PRS_FN_AND,           /**< A & B */
  PRS_FN_OR,            /**< A | B */
  PRS_FN_XOR,           /**< A ^ B */
  PRS_FN_NAND,          /**< ~(A & B) */
  PRS_FN_NOR,           /**< ~(A | B) */
  PRS_FN_XNOR,          /**< ~(A ^ B) */
  PRS_FN_ZERO,          /**< constant 0 */
  PRS_FN_ONE,           /**< constant 1 */
} prs_fn_t;

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver state machine possible states.
 */
typedef enum {
  PRS_UNINIT = 0,                   /**< Not initialized.                    */
  PRS_STOP = 1,                     /**< Stopped.                            */
  PRS_READY = 2,                    /**< Ready.                              */
} prsstate_t;

/**
 * @brief   Type of a structure representing a PRS driver.
 */
typedef struct PRSDriver PRSDriver;

#include "hal_prs_lld.h"

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void     prsInit(void);
  void     prsObjectInit(PRSDriver *pp);
  void     prsStart(PRSDriver *pp, const PRSConfig *config);
  void     prsStop(PRSDriver *pp);

  /* Route a source to a channel (PRS: ASYNC_CH/SYNC_CH CTRL). */
  void     prsConnect(PRSDriver *pp, prs_path_t path, uint8_t ch,
                      prs_source_t gen);
  void     prsDisconnect(PRSDriver *pp, prs_path_t path, uint8_t ch);
  /* Optional logic function on an async channel (aux = second channel). */
  void     prsSetLogic(PRSDriver *pp, uint8_t ch, prs_fn_t fn, uint8_t aux);

  /* Route a channel to a peripheral consumer. */
  void     prsConnectConsumer(PRSDriver *pp, prs_consumer_t consumer,
                              prs_path_t path, uint8_t ch);

  /* Route a channel to a GPIO pin (pad mode: peripheral output, *_ALT). */
  void     prsRoutePin(PRSDriver *pp, prs_path_t path, uint8_t ch,
                       ioportid_t port, ioportmask_t pad);
  void     prsUnroutePin(PRSDriver *pp, prs_path_t path, uint8_t ch);

  /* Software events on async channels (bit mask of channels). */
  void     prsSwPulse(PRSDriver *pp, uint16_t ch_mask);
  void     prsSwLevelSet(PRSDriver *pp, uint16_t ch_mask);
  void     prsSwLevelClear(PRSDriver *pp, uint16_t ch_mask);

  /* Current channel levels. */
  uint32_t prsPeek(PRSDriver *pp, prs_path_t path);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_PRS */

#endif /* HAL_PRS_H_ */

/** @} */
