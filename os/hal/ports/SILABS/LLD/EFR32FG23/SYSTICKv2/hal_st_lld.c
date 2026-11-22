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
 * @file    SYSTICKv2/hal_st_lld.c
 * @brief   EFR32FG23 SYSRTC-based ST driver low level source.
 *
 * @details Uses SYSRTC (32-bit counter, LF clock, EM4-capable) as the
 *          ChibiOS system tick in freerunning mode. CMP0 is the tick
 *          alarm, CMP1 is a secondary alarm, OVF handles 32-bit overflow.
 *
 * @addtogroup ST
 * @{
 */

#include "hal.h"

#if defined(EFR32_SYSRTC_USE_SYSRTC) && (EFR32_SYSRTC_USE_SYSRTC == TRUE) && !defined(__DOXYGEN__)
#include "hal_sysrtc_lld.h"
#endif

#if (OSAL_ST_MODE != OSAL_ST_MODE_NONE) || defined(__DOXYGEN__)

#if OSAL_ST_MODE == OSAL_ST_MODE_PERIODIC
#error "SYSTICKv2 (SYSRTC0) supports only OSAL_ST_MODE_FREERUNNING"
#endif

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define ST_HANDLER      EFR32_SYSRTC_APP_HANDLER
#define ST_NUMBER       EFR32_SYSRTC_APP_NUMBER

/* SYSRTC clock — from EM23GRPACLK or LFACLK depending on config.
   Typically LFXO = 32768 Hz. The prescaler for SYSRTC is fixed at
   the clock source — we use the raw LF clock and let OSAL_ST_FREQUENCY
   match it. */

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

OSAL_IRQ_HANDLER(ST_HANDLER) {
  OSAL_IRQ_PROLOGUE();
#if defined(EFR32_SYSRTC_USE_SYSRTC) && (EFR32_SYSRTC_USE_SYSRTC == TRUE) && !defined(__DOXYGEN__)
  /* Forward SYSRTCv1 events first (it must not touch CMP0/OVF, see below). */
  sysrtc_lld_serve_interrupt(&SYSRTCD1);
#endif
  st_lld_serve_interrupt();
  OSAL_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void st_lld_init(void) {
  /* Enable SYSRTC clock. */
  CMU->CLKEN0_SET = CMU_CLKEN0_SYSRTC0;

  /* Select the LF clock source for SYSRTC. The reset default is LFRCO,
     which may be disabled — without a running clock SYNCBUSY never clears.
     CH_CFG_ST_FREQUENCY must match the selected source (LFXO = 32768 Hz). */
#if EFR32_LFXO_ENABLED
  CMU->SYSRTC0CLKCTRL = (CMU->SYSRTC0CLKCTRL & ~_CMU_SYSRTC0CLKCTRL_CLKSEL_MASK)
                        | CMU_SYSRTC0CLKCTRL_CLKSEL_LFXO;
#elif EFR32_LFRCO_ENABLED
  CMU->SYSRTC0CLKCTRL = (CMU->SYSRTC0CLKCTRL & ~_CMU_SYSRTC0CLKCTRL_CLKSEL_MASK)
                        | CMU_SYSRTC0CLKCTRL_CLKSEL_LFRCO;
#elif EFR32_ULFRCO_ENABLED
  CMU->SYSRTC0CLKCTRL = (CMU->SYSRTC0CLKCTRL & ~_CMU_SYSRTC0CLKCTRL_CLKSEL_MASK)
                        | CMU_SYSRTC0CLKCTRL_CLKSEL_ULFRCO;
#endif

  /* Reset and configure SYSRTC.
     NOTE: on Series 2 the LF-domain synchronization (SYNCBUSY) only runs
     while the module is enabled, so EN must be set before writing CNT. */
  SYSRTC0->EN_CLR = SYSRTC_EN_EN;
  while (SYSRTC0->EN & SYSRTC_EN_DISABLING) {}

  SYSRTC0->SWRST = SYSRTC_SWRST_SWRST;
  while (SYSRTC0->SWRST & SYSRTC_SWRST_RESETTING) {}

  /* Enable module (LF domain starts clocking). */
  SYSRTC0->EN_SET = SYSRTC_EN_EN;
  while ((SYSRTC0->EN & SYSRTC_EN_EN) == 0U) {}

  SYSRTC0->GRP0_IF_CLR = _SYSRTC_GRP0_IF_MASK;
  SYSRTC0->GRP0_IEN = 0U;

  /* FREERUNNING mode: the counter runs freely and the kernel drives alarms
     through CMP0 (st_lld_start_alarm/st_lld_set_alarm), so init must NOT
     arm CMP0 itself — leave CMP0 to the kernel, enable only OVF here. */
  SYSRTC0->GRP0_CTRL_SET = _SYSRTC_GRP0_CTRL_CMP0EN_MASK;
  SYSRTC0->GRP0_CMP0VALUE = 0U;
  while (SYSRTC0->GRP0_SYNCBUSY & _SYSRTC_GRP0_SYNCBUSY_MASK) {}
  SYSRTC0->GRP0_IEN_SET = SYSRTC_GRP0_IEN_OVF;

  /* Start counter from 0. */
  SYSRTC0->CNT = 0U;
  while (SYSRTC0->SYNCBUSY & SYSRTC_SYNCBUSY_CNT) {}

  SYSRTC0->CMD = SYSRTC_CMD_START;

  nvicEnableVector(ST_NUMBER, EFR32_SYSRTC_ST_IRQ_PRIORITY);
}

void st_lld_serve_interrupt(void) {
  uint32_t isr = SYSRTC0->GRP0_IF;

  if ((isr & SYSRTC_GRP0_IF_CMP0) != 0U) {
    SYSRTC0->GRP0_IF_CLR = SYSRTC_GRP0_IF_CMP0;
    /* FREERUNNING: CMP0 is the kernel alarm — do not re-arm here, the
       kernel sets the next alarm via stSetAlarm(). */
    osalSysLockFromISR();
    osalOsTimerHandlerI();
    osalSysUnlockFromISR();
  }

  if ((isr & SYSRTC_GRP0_IF_OVF) != 0U) {
    SYSRTC0->GRP0_IF_CLR = SYSRTC_GRP0_IF_OVF;
    /* Overflow — the 32-bit counter wrapped. OSAL handles this via
       systime_t arithmetic, no action needed here. */
  }
}

#endif /* OSAL_ST_MODE != OSAL_ST_MODE_NONE */

/** @} */
