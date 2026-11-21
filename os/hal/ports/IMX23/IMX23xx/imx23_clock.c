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
 * @file    IMX23/IMX23xx/imx23_clock.c
 * @brief   i.MX23 CPU clock / VDDD voltage scaling (ported from the Linux
 *          mxs clock tree and the U-Boot SPL power init).
 *
 * @addtogroup IMX23
 * @{
 */

#include "hal.h"
#include "imx23_clock.h"

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   Speed profiles: 480 / 240 / 120 MHz.
 * @note    vddd_mV = 0 on the fastest profile so that whatever the bootstream
 *          programmed is kept; lower profiles scale VDDD down (25mV steps).
 */
const imx23_clock_profile_t imx23_clock_profiles[IMX23_CLOCK_PROFILE_COUNT] = {
  { 1U, 2U,    0U },   /* 480 MHz, HBUS 240 MHz, VDDD unchanged   */
  { 2U, 2U, 1200U },   /* 240 MHz, HBUS 120 MHz, VDDD 1.20V       */
  { 4U, 2U, 1050U },   /* 120 MHz, HBUS  60 MHz, VDDD 1.05V       */
};

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

static unsigned current_profile = 0U;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/* Wait until the CLKCTRL_CPU divider is not busy (RM: do not write while
   BUSY_REF_CPU / BUSY_REF_XTAL are set). */
static void clock_wait_not_busy(void) {
  volatile uint32_t *cpu = (volatile uint32_t *)IMX23_CLKCTRL_CPU;

  while ((*cpu & (IMX23_CLKCTRL_CPU_BUSY_REF_CPU |
                  IMX23_CLKCTRL_CPU_BUSY_REF_XTAL)) != 0U) {
    /* spin */
  }
}

/* Write a 6-bit divider field using SET/CLR so other bits are preserved. */
static void clock_set_div(volatile uint32_t *reg, uint32_t mask, uint8_t div) {
  *reg = mask;              /* CLR: clear the field */
  *(reg + 1U) = (uint32_t)div & mask; /* SET: write the new value */
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

uint32_t imx23_clock_cpu_hz(void) {
  volatile uint32_t *cpu = (volatile uint32_t *)IMX23_CLKCTRL_CPU;
  uint32_t div = *cpu & IMX23_CLKCTRL_CPU_DIV_CPU_MASK;

  if (div == 0U) {
    div = 1U;
  }
  return IMX23_PLL0_HZ / div;
}

unsigned imx23_clock_current_profile(void) {

  return current_profile;
}

uint32_t imx23_clock_set_profile(unsigned profile) {
  const imx23_clock_profile_t *p;
  volatile uint32_t *vddd = (volatile uint32_t *)IMX23_POWER_VDDDCTRL;

  if (profile >= IMX23_CLOCK_PROFILE_COUNT) {
    profile = IMX23_CLOCK_PROFILE_COUNT - 1U;
  }
  p = &imx23_clock_profiles[profile];

  /* Make sure we are running from PLL0 (BYPASS_CPU clear), not the XTAL. */
  *(volatile uint32_t *)(IMX23_CLKCTRL_CLKSEQ + 0x08U) =
      IMX23_CLKCTRL_CLKSEQ_BYPASS_CPU;   /* CLR */

  /* Optionally lower/raise the VDDD target first (25mV steps from 0.8V). */
  if (p->vddd_mV != 0U) {
    uint32_t trg = ((uint32_t)p->vddd_mV - IMX23_POWER_VDDD_BASE_MV) /
                   IMX23_POWER_VDDD_STEP_MV;
    if (trg > IMX23_POWER_VDDDCTRL_TRG_MASK) {
      trg = IMX23_POWER_VDDDCTRL_TRG_MASK;
    }
    *vddd = IMX23_POWER_VDDDCTRL_TRG_MASK;          /* CLR TRG field */
    *(vddd + 1U) = trg;                             /* SET new TRG */
  }

  /* Change the dividers only while the divider is idle. */
  clock_wait_not_busy();
  clock_set_div((volatile uint32_t *)IMX23_CLKCTRL_HBUS,
                IMX23_CLKCTRL_HBUS_DIV_MASK, p->div_hbus);
  clock_wait_not_busy();
  clock_set_div((volatile uint32_t *)IMX23_CLKCTRL_CPU,
                IMX23_CLKCTRL_CPU_DIV_CPU_MASK, p->div_cpu);
  clock_wait_not_busy();

  current_profile = profile;

  return imx23_clock_cpu_hz();
}

/** @} */
