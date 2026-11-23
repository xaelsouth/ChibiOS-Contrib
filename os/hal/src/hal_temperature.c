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
 * @file    hal_temperature.c
 * @brief   Generic temperature sensor driver (upper layer).
 *
 * @addtogroup TEMPERATURE
 * @{
 */

#include "hal_temperature.h"

#if (HAL_USE_TEMPERATURE == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void tempInit(void) {
  temp_lld_init();
}

void tempObjectInit(TEMPERATUREDriver *tp) {
  tp->state    = TEMP_STOP;
  tp->config   = NULL;
  tp->corr_num = 0U;
  for (size_t i = 0U; i < TEMPERATURE_CORR_MAX_COEFFS; i++) {
    tp->corr[i] = 0.0f;
  }
}

void tempStart(TEMPERATUREDriver *tp, const TEMPERATUREConfig *config) {
  osalDbgCheck(tp != NULL);

  if (tp->state == TEMP_STOP) {
    temp_lld_start(tp);
  }
  tp->config = config;

  /* Correction polynomial from config (NULL/0 = no correction). */
  tempClearCorrection(tp);
  if ((config != NULL) && (config->corr != NULL) && (config->corr_num > 0U)) {
    tempSetCorrection(tp, config->corr, config->corr_num);
  }

  tp->state = TEMP_READY;
}

void tempStop(TEMPERATUREDriver *tp) {
  osalDbgCheck(tp != NULL);

  if (tp->state == TEMP_READY) {
    temp_lld_stop(tp);
    tp->state  = TEMP_STOP;
    tp->config = NULL;
  }
}

/**
 * @brief   Reads the temperature in degC, applying the correction
 *          polynomial (Horner schema) if one is set.
 */
float tempReadCelsius(TEMPERATUREDriver *tp) {
  float t = temp_lld_read_celsius(tp);   /* measured value, uncorrected */
  if (tp->corr_num > 0U) {
    /* Horner on the measured value: c0 + c1*T + ... + c[n-1]*T^(n-1) */
    float r = tp->corr[tp->corr_num - 1U];
    for (size_t i = tp->corr_num - 1U; i-- > 0U; ) {
      r = r * t + tp->corr[i];
    }
    t = r;
  }
  return t;
}

void tempSetCorrection(TEMPERATUREDriver *tp, const float *coeffs,
                       size_t count) {
  if ((coeffs == NULL) || (count == 0U)) {
    tempClearCorrection(tp);
    return;
  }
  if (count > TEMPERATURE_CORR_MAX_COEFFS) {
    count = TEMPERATURE_CORR_MAX_COEFFS;
  }
  for (size_t i = 0U; i < count; i++) {
    tp->corr[i] = coeffs[i];
  }
  tp->corr_num = count;
}

void tempClearCorrection(TEMPERATUREDriver *tp) {
  /* Default: all-zero coefficients, count 0 = no correction applied. */
  for (size_t i = 0U; i < TEMPERATURE_CORR_MAX_COEFFS; i++) {
    tp->corr[i] = 0.0f;
  }
  tp->corr_num = 0U;
}

#endif /* HAL_USE_TEMPERATURE */

/** @} */
