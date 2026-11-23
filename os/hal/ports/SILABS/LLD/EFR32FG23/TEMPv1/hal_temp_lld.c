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
 * @file    TEMPv1/hal_temp_lld.c
 * @brief   EFR32FG23 temperature sensor LLD (EMU->TEMP) for the generic
 *          hal_temperature driver.
 *
 * @addtogroup TEMPERATURE
 * @{
 */

#include "hal.h"
#include "hal_temperature_lld.h"

#if (HAL_USE_TEMPERATURE == TRUE) && (EFR32_TEMP_USE_TEMP == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if (EFR32_TEMP_USE_TEMP1 == TRUE) || defined(__DOXYGEN__)
TEMPERATUREDriver TEMPD1;
#endif
#if (EFR32_TEMP_USE_TEMP2 == TRUE) || defined(__DOXYGEN__)
TEMPERATUREDriver TEMPD2;
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void temp_lld_init(void) {
  /* Nothing to configure: the EMU temperature sensor is always clocked
     (always-on domain) and starts measuring after reset. */
}

void temp_lld_start(TEMPERATUREDriver *tp) {
  (void)tp;
  /* Wait for the first temperature conversion to complete. */
  volatile uint32_t sp = 1000000UL;
  while (((EMU->STATUS & EMU_STATUS_FIRSTTEMPDONE) == 0U) && (--sp != 0U)) {
  }
}

void temp_lld_stop(TEMPERATUREDriver *tp) {
  (void)tp;
  /* Always-on sensor: nothing to stop. */
}

uint32_t temp_lld_read_raw(TEMPERATUREDriver *tp) {
  (void)tp;
  /* EMU->TEMP: bits 2..10 = TEMP, bits 0..1 = TEMPLSB; value = Kelvin x 4.
     Read twice until stable (the sensor updates asynchronously). */
  uint32_t val1, val2;
  const uint32_t mask = _EMU_TEMP_TEMP_MASK | _EMU_TEMP_TEMPLSB_MASK;
  do {
    val1 = EMU->TEMP & mask;
    val2 = EMU->TEMP & mask;
  } while (val1 != val2);
  return val1 >> _EMU_TEMP_TEMPLSB_SHIFT;
}

float temp_lld_read_celsius(TEMPERATUREDriver *tp) {
  /* Uncorrected, factory-calibrated value. */
  return ((float)temp_lld_read_raw(tp) / 4.0f) - 273.15f;
}

#endif /* HAL_USE_TEMPERATURE && EFR32_TEMP_USE_TEMP */

/** @} */
