/*
    ChibiOS - Copyright (C) 2026 Giovanni Di Sirio.

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
 * @file    IMX23/IMX23xx/imx23_clock.h
 * @brief   i.MX23 CPU clock / VDDD voltage scaling.
 *
 * @addtogroup IMX23
 * @{
 */

#ifndef IMX23_CLOCK_H
#define IMX23_CLOCK_H

#include "imx23.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   CPU speed profiles.
 * @note    PLL0 is fixed at 480MHz; CPU = 480MHz / div_cpu,
 *          HBUS = 480MHz / div_hbus. vddd_mV == 0 means "leave unchanged".
 */
typedef struct {
  uint8_t  div_cpu;    /**< HW_CLKCTRL_CPU.DIV_CPU                          */
  uint8_t  div_hbus;   /**< HW_CLKCTRL_HBUS.DIV                             */
  uint16_t vddd_mV;    /**< VDDD target in mV, 0 = keep current             */
} imx23_clock_profile_t;

#define IMX23_CLOCK_PROFILE_COUNT   3

extern const imx23_clock_profile_t imx23_clock_profiles[IMX23_CLOCK_PROFILE_COUNT];

/**
 * @brief   Returns the current CPU frequency in Hz.
 */
uint32_t imx23_clock_cpu_hz(void);

/**
 * @brief   Returns the index of the currently active profile (best match).
 */
unsigned imx23_clock_current_profile(void);

/**
 * @brief   Switches CPU/HBUS dividers and optionally VDDD.
 *
 * @param[in] profile   profile index (0..IMX23_CLOCK_PROFILE_COUNT-1)
 * @return              the new CPU frequency in Hz.
 */
uint32_t imx23_clock_set_profile(unsigned profile);

#ifdef __cplusplus
}
#endif

#endif /* IMX23_CLOCK_H */

/** @} */
