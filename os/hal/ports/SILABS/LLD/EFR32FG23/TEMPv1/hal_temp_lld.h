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
 * @file    TEMPv1/hal_temp_lld.h
 * @brief   EFR32FG23 die temperature sensor LLD header.
 *
 * @details On xG23 the on-chip temperature sensor is part of the EMU
 *          (Energy Management Unit), always-on domain: the raw value is in
 *          EMU->TEMP as Kelvin x 4 (11 bits, bits 0-1 = TEMPLSB fraction).
 *          The factory calibration is baked into the sensor, so the simple
 *          conversion gives the datasheet accuracy (grade dependent, see
 *          DEVINFO->PKGINFO TEMPGRADE; +/-2 degC class devices exist).
 *
 *          This is an LLD-only driver (no portable HAL temperature model
 *          exists); enable with EFR32_TEMP_USE_TEMP TRUE in halconf.
 *
 * @addtogroup TEMP
 * @{
 */

#ifndef HAL_TEMP_LLD_H
#define HAL_TEMP_LLD_H

#if !defined(EFR32_TEMP_USE_TEMP) || defined(__DOXYGEN__)
#define EFR32_TEMP_USE_TEMP     FALSE
#endif

#ifdef __cplusplus
extern "C" {
#endif

  void     temp_lld_init(void);
  void     temp_lld_start(void);
  uint32_t temp_lld_read_raw(void);
  float    temp_lld_read_celsius(void);
  float    temp_lld_read_kelvin(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_TEMP_LLD_H */

/** @} */
