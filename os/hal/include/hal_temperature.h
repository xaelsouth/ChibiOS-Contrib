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
 * @file    hal_temperature.h
 * @brief   Generic temperature sensor driver (upper layer).
 *
 * @details Portable temperature sensor interface in ChibiOS driver style
 *          (like hal_comp/hal_crc): the port LLD (hal_temperature_lld.h)
 *          provides the raw sensor reading, this layer adds a correction
 *          polynomial of any degree, evaluated in Horner schema on the
 *          measured value:
 *            T_corrected = ((c[n-1]*T + c[n-2])*T + ...) + c0
 *          If no coefficients are set, the measured value is returned
 *          unchanged.
 *
 * @addtogroup TEMPERATURE
 * @{
 */

#ifndef HAL_TEMPERATURE_H_
#define HAL_TEMPERATURE_H_

#include "hal.h"

#if (HAL_USE_TEMPERATURE == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/** Maximum number of correction coefficients (polynomial degree + 1). */
#if !defined(TEMPERATURE_CORR_MAX_COEFFS) || defined(__DOXYGEN__)
#define TEMPERATURE_CORR_MAX_COEFFS     8
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver state machine possible states.
 */
typedef enum {
  TEMP_UNINIT = 0,                   /**< Not initialized.                   */
  TEMP_STOP = 1,                     /**< Stopped.                           */
  TEMP_READY = 2,                    /**< Ready.                             */
} tempstate_t;

/**
 * @brief   Type of a structure representing a temperature sensor driver.
 */
typedef struct TEMPERATUREDriver TEMPERATUREDriver;

#include "hal_temperature_lld.h"

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Reads the raw sensor value (LLD units, e.g. Kelvin x 4).
 *
 * @param[in] tp        pointer to the @p TEMPERATUREDriver object
 */
#define tempReadRaw(tp)                 temp_lld_read_raw(tp)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void     tempInit(void);
  void     tempObjectInit(TEMPERATUREDriver *tp);
  void     tempStart(TEMPERATUREDriver *tp, const TEMPERATUREConfig *config);
  void     tempStop(TEMPERATUREDriver *tp);
  float    tempReadCelsius(TEMPERATUREDriver *tp);
  void     tempSetCorrection(TEMPERATUREDriver *tp, const float *coeffs,
                             size_t count);
  void     tempClearCorrection(TEMPERATUREDriver *tp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_TEMPERATURE */

#endif /* HAL_TEMPERATURE_H_ */

/** @} */
