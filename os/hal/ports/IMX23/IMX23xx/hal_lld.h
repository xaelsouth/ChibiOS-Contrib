/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

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
 * @file    IMX23/IMX23xx/hal_lld.h
 * @brief   i.MX23 HAL subsystem low level driver header.
 *
 * @addtogroup IMX23
 * @{
 */

#ifndef HAL_LLD_H
#define HAL_LLD_H

#include "imx23.h"
#include "icoll.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Defines the support for realtime counters in the HAL.
 */
#define HAL_IMPLEMENTS_COUNTERS             FALSE

/**
 * @brief   Platform name.
 */
#define PLATFORM_NAME                       "i.MX23 IMX23xx"

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a clock point.
 */
typedef enum {
  IMX23_CLKPT_XTAL = 0,         /* 24MHz crystal                          */
  IMX23_CLKPT_CPU,              /* ARM926 CPU clock (board define)        */
  IMX23_CLKPT_HBUS,             /* AHB/HBUS clock                         */
  IMX23_CLKPT_DUART,            /* PL011 debug UART clock (24MHz)         */
  IMX23_CLKPT_TIMROT            /* TIMROT "always tick" clock (24MHz)     */
} imx23_clkpt_t;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  uint32_t hal_lld_get_clock_point(imx23_clkpt_t clkpt);
  void hal_lld_init(void);
#ifdef __cplusplus
}
#endif

#endif /* HAL_LLD_H */

/** @} */
