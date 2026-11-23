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
 * @file    hal_sigroute.h
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
 *          - sigroute_source_t   : event generator encoding (vendor specific,
 *                             e.g. SIGROUTE_GEN_TIMER0_CC0 in the LLD header)
 *          - sigroute_consumer_t : event consumer encoding (vendor specific)
 *          - channel index  : async (low-power capable) or sync channel
 *          - sigroute_fn_t       : optional per-channel logic function
 *          - software events (pulse/level) and channel peek
 *
 * @addtogroup SIGROUTE
 * @{
 */

#ifndef HAL_PRS_H_
#define HAL_PRS_H_

#include "hal.h"

#if (HAL_USE_SIGROUTE == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Route path: async channels keep working in low-power modes
 *          (EM2 on EFR32), sync channels are cycle-accurate (HFPERCLK).
 */
typedef enum {
  SIGROUTE_PATH_ASYNC = 0,   /**< Low-power capable route. */
  SIGROUTE_PATH_SYNC  = 1,   /**< Synchronous, cycle-accurate route. */
} sigroute_path_t;

/**
 * @brief   Event generator (source) encoding. Vendor specific values are
 *          provided by the LLD header as SIGROUTE_GEN_* macros.
 */
typedef uint32_t sigroute_source_t;

/**
 * @brief   Event consumer encoding. Vendor specific values are provided by
 *          the LLD header as SIGROUTE_CON_* macros.
 */
typedef uint32_t sigroute_consumer_t;

/**
 * @brief   Optional per-channel logic function combining the channel
 *          signal A with an auxiliary channel signal B (hardware FNSEL).
 */
typedef enum {
  SIGROUTE_FN_PASS_A = 0,    /**< A (plain pass-through). */
  SIGROUTE_FN_NOT_A,         /**< ~A */
  SIGROUTE_FN_AND,           /**< A & B */
  SIGROUTE_FN_OR,            /**< A | B */
  SIGROUTE_FN_XOR,           /**< A ^ B */
  SIGROUTE_FN_NAND,          /**< ~(A & B) */
  SIGROUTE_FN_NOR,           /**< ~(A | B) */
  SIGROUTE_FN_XNOR,          /**< ~(A ^ B) */
  SIGROUTE_FN_ZERO,          /**< constant 0 */
  SIGROUTE_FN_ONE,           /**< constant 1 */
} sigroute_fn_t;

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver state machine possible states.
 */
typedef enum {
  SIGROUTE_UNINIT = 0,                   /**< Not initialized.                    */
  SIGROUTE_STOP = 1,                     /**< Stopped.                            */
  SIGROUTE_READY = 2,                    /**< Ready.                              */
} sigroutestate_t;

/**
 * @brief   Type of a structure representing a SIGROUTE driver.
 */
typedef struct SIGROUTEDriver SIGROUTEDriver;

#include "hal_sigroute_lld.h"

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void     sigrouteInit(void);
  void     sigrouteObjectInit(SIGROUTEDriver *pp);
  void     sigrouteStart(SIGROUTEDriver *pp, const SIGROUTEConfig *config);
  void     sigrouteStop(SIGROUTEDriver *pp);

  /* Route a source to a channel (hardware: ASYNC_CH/SYNC_CH CTRL). */
  void     sigrouteConnect(SIGROUTEDriver *pp, sigroute_path_t path, uint8_t ch,
                      sigroute_source_t gen);
  void     sigrouteDisconnect(SIGROUTEDriver *pp, sigroute_path_t path, uint8_t ch);
  /* Optional logic function on an async channel (aux = second channel). */
  void     sigrouteSetLogic(SIGROUTEDriver *pp, uint8_t ch, sigroute_fn_t fn, uint8_t aux);

  /* Route a channel to a peripheral consumer. */
  void     sigrouteConnectConsumer(SIGROUTEDriver *pp, sigroute_consumer_t consumer,
                              sigroute_path_t path, uint8_t ch);

  /* Route a channel to a GPIO pin (pad mode: peripheral output, *_ALT). */
  void     sigrouteRoutePin(SIGROUTEDriver *pp, sigroute_path_t path, uint8_t ch,
                       ioportid_t port, ioportmask_t pad);
  void     sigrouteUnroutePin(SIGROUTEDriver *pp, sigroute_path_t path, uint8_t ch);

  /* Software events on async channels (bit mask of channels). */
  void     sigrouteSwPulse(SIGROUTEDriver *pp, uint16_t ch_mask);
  void     sigrouteSwLevelSet(SIGROUTEDriver *pp, uint16_t ch_mask);
  void     sigrouteSwLevelClear(SIGROUTEDriver *pp, uint16_t ch_mask);

  /* Current channel levels. */
  uint32_t sigroutePeek(SIGROUTEDriver *pp, sigroute_path_t path);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SIGROUTE */

#endif /* HAL_PRS_H_ */

/** @} */
