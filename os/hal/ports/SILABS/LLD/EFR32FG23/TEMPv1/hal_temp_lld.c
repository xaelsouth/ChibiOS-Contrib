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

/* Correction polynomial (degC domain), ascending power coefficients,
   evaluated in Horner schema on the measured value on read.
   Zero-initialized (0,0,0,...); count==0 = not provided = raw passthrough. */
static float  temp_corr[EFR32_TEMP_CORR_MAX_COEFFS];
static size_t temp_corr_count;   /* 0 = no correction (all-zero coeffs) */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void temp_lld_init(void) {
  /* Nothing to configure: the EMU temperature sensor is always clocked
     (always-on domain) and starts measuring after reset. */
  temp_lld_clear_correction();
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
  if (temp_corr_count > 0U) {
    /* Horner schema on the measured value T: c0 + c1*T + ... + c[n-1]*T^(n-1) */
    float r = temp_corr[temp_corr_count - 1U];
    for (size_t i = temp_corr_count - 1U; i-- > 0U; ) {
      r = r * t + temp_corr[i];
    }
    t = r;
  }
  return t;
}

void temp_lld_set_correction(const float *coeffs, size_t count) {
  if (coeffs == NULL || count == 0U) {
    temp_lld_clear_correction();
    return;
  }
  if (count > EFR32_TEMP_CORR_MAX_COEFFS) {
    count = EFR32_TEMP_CORR_MAX_COEFFS;
  }
  for (size_t i = 0U; i < count; i++) {
    temp_corr[i] = coeffs[i];
  }
  temp_corr_count = count;
}

void temp_lld_clear_correction(void) {
  /* Default: all-zero coefficients = identity (no correction). */
  for (size_t i = 0U; i < EFR32_TEMP_CORR_MAX_COEFFS; i++) {
    temp_corr[i] = 0.0f;
  }
  temp_corr_count = 0U;
}

#endif /* EFR32_TEMP_USE_TEMP == TRUE */

/** @} */
