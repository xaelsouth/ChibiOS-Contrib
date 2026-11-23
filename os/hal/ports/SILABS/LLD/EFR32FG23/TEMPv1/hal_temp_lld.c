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
 * @brief   EFR32FG23 die temperature sensor LLD (EMU->TEMP).
 *
 * @addtogroup TEMP
 * @{
 */

#include "hal.h"
#include "hal_temp_lld.h"

#if (EFR32_TEMP_USE_TEMP == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/* Quadratic correction curve (degC domain), applied on read if valid. */
static temp_corr_t temp_corr;
static bool        temp_corr_valid;

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void temp_lld_init(void) {
  /* Nothing to configure: the EMU temperature sensor is always clocked
     (always-on domain) and starts measuring after reset. */
  temp_corr_valid = false;
  temp_corr.quad = 0.0f;
  temp_corr.lin  = 1.0f;
  temp_corr.offs = 0.0f;
}

void temp_lld_start(void) {
  /* Wait for the first temperature conversion to complete. */
  volatile uint32_t sp = 1000000UL;
  while (((EMU->STATUS & EMU_STATUS_FIRSTTEMPDONE) == 0U) && (--sp != 0U)) {
  }
}

uint32_t temp_lld_read_raw(void) {
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

float temp_lld_read_kelvin(void) {
  return (float)temp_lld_read_raw() / 4.0f;
}

float temp_lld_read_celsius(void) {
  float t = temp_lld_read_kelvin() - 273.15f;
  if (temp_corr_valid) {
    t = temp_corr.quad * t * t + temp_corr.lin * t + temp_corr.offs;
  }
  return t;
}

void temp_lld_set_correction(const temp_corr_t *corr) {
  if (corr == NULL) {
    temp_lld_clear_correction();
    return;
  }
  temp_corr = *corr;
  temp_corr_valid = true;
}

void temp_lld_clear_correction(void) {
  temp_corr_valid = false;
  temp_corr.quad = 0.0f;
  temp_corr.lin  = 1.0f;
  temp_corr.offs = 0.0f;
}

#endif /* EFR32_TEMP_USE_TEMP == TRUE */

/** @} */
