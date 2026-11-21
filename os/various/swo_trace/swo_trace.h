/*
    ChibiOS - Copyright (C) 2026 Xael South.

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
 * @file    swo_trace.h
 * @brief   SWO (Single Wire Output) / ITM trace stream headers.
 *
 * @addtogroup SWO_TRACE
 * @{
 */

#ifndef SWO_TRACE_H
#define SWO_TRACE_H

#include "ch.h"
#include "hal.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   ITM/DWT/TPI lock access key.
 */
#define SWO_TRACE_UNLOCK_KEY    0xC5ACCE55U

/**
 * @brief   TPI ACPR prescaler value for a given core clock and SWO baud.
 */
#define SWO_TRACE_PRESCALER(hclk, baud)  (((hclk) / (baud)) - 1U)

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   SWO trace driver configuration structure.
 */
typedef struct {
  /**
   * @brief   TPI ACPR prescaler register value.
   * @note    SWO baud = core clock / (prescaler + 1), use
   *          @p SWO_TRACE_PRESCALER(hclk, baud).
   */
  uint32_t                  prescaler;
  /**
   * @brief   ITM stimulus port enable bitmask (written to ITM->TER).
   * @note    Port 0 is the one used by chprintf()-style output.
   */
  uint32_t                  stim_ports;
} SWOConfig;

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   SWO trace output stream.
 * @note    Output-only: reads park the calling thread forever, writes drop
 *          bytes when no SWO reader is attached (FIFO full) so logging can
 *          never block the firmware.
 */
extern BaseSequentialStream SWOStream;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void swoTraceInit(const SWOConfig *config);
#ifdef __cplusplus
}
#endif

#endif /* SWO_TRACE_H */

/** @}*/
