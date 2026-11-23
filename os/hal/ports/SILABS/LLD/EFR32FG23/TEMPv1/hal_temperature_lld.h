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
 * @file    TEMPv1/hal_temperature_lld.h
 * @brief   EFR32FG23 temperature sensor LLD for the generic
 *          hal_temperature driver (upper layer).
 *
 * @details On xG23 the on-chip temperature sensor is part of the EMU
 *          (Energy Management Unit), always-on domain: the raw value is in
 *          EMU->TEMP as Kelvin x 4 (11 bits, bits 0-1 = TEMPLSB fraction).
 *          The factory calibration is baked into the sensor, so the simple
 *          conversion gives the datasheet accuracy (grade dependent, see
 *          DEVINFO->PKGINFO TEMPGRADE; +/-2 degC class devices exist).
 *
 *          This LLD provides the raw reading; the correction polynomial is
 *          applied by the upper layer (hal_temperature.c, Horner schema).
 *
 * @addtogroup TEMPERATURE
 * @{
 */

#ifndef HAL_TEMPERATURE_LLD_H
#define HAL_TEMPERATURE_LLD_H

#if (HAL_USE_TEMPERATURE == TRUE) || defined(__DOXYGEN__)

#if !defined(EFR32_TEMP_USE_TEMP) || defined(__DOXYGEN__)
#define EFR32_TEMP_USE_TEMP     FALSE
#endif

/* Per-instance enables. The xG23 has one physical die sensor (EMU->TEMP),
   but multiple TEMPERATUREDriver objects are supported — each object has
   its own state and correction curve (e.g. different calibrations of the
   same sensor for different consumers). TEMP1 defaults to the master
   switch EFR32_TEMP_USE_TEMP. */
#if !defined(EFR32_TEMP_USE_TEMP1) || defined(__DOXYGEN__)
#define EFR32_TEMP_USE_TEMP1    EFR32_TEMP_USE_TEMP
#endif
#if !defined(EFR32_TEMP_USE_TEMP2) || defined(__DOXYGEN__)
#define EFR32_TEMP_USE_TEMP2    FALSE
#endif

/* tempstate_t and the forward declaration of TEMPERATUREDriver come from
   the upper hal_temperature.h. TEMPERATURE_CORR_MAX_COEFFS likewise. */

/**
 * @brief   Temperature sensor configuration (LLD part).
 * @details The correction polynomial itself is applied by the upper layer.
 */
typedef struct {
  const float *corr;      /**< Ascending Horner coefficients (c0..cn), or NULL. */
  size_t       corr_num;  /**< Number of coefficients (0 = no correction). */
} TEMPERATUREConfig;

/**
 * @brief   Structure representing a temperature sensor driver.
 */
struct TEMPERATUREDriver {
  tempstate_t              state;
  const TEMPERATUREConfig *config;
  /* Correction storage, managed by the upper layer. */
  float                    corr[TEMPERATURE_CORR_MAX_COEFFS];
  size_t                   corr_num;
};

#if (EFR32_TEMP_USE_TEMP1 == TRUE) || defined(__DOXYGEN__)
extern TEMPERATUREDriver TEMPD1;
#endif
#if (EFR32_TEMP_USE_TEMP2 == TRUE) || defined(__DOXYGEN__)
extern TEMPERATUREDriver TEMPD2;
#endif

#ifdef __cplusplus
extern "C" {
#endif

  void     temp_lld_init(void);
  void     temp_lld_start(TEMPERATUREDriver *tp);
  void     temp_lld_stop(TEMPERATUREDriver *tp);
  uint32_t temp_lld_read_raw(TEMPERATUREDriver *tp);
  float    temp_lld_read_celsius(TEMPERATUREDriver *tp);

#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_TEMPERATURE */

#endif /* HAL_TEMPERATURE_LLD_H */

/** @} */
