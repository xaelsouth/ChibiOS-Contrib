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
 * @file    SYSTICKv4/hal_st_lld.c
 * @brief   EFR32FG23 BURTC-based ST driver low level source.
 *
 * @details Uses BURTC (32-bit up-counter, EM4GRPACLK, EM4-capable) as the
 *          ChibiOS system tick in freerunning mode. COMP is the tick
 *          alarm, OF handles the (rare) 32-bit overflow.
 *
 *          BURTC is shared with the BURTCv1 RTC calendar driver: this
 *          driver owns the BURTC interrupt vector and forwards RTC events
 *          to rtc_lld_serve_interrupt().
 *
 * @addtogroup ST
 * @{
 */

#include "hal.h"

#if (HAL_USE_RTC == TRUE) && defined(EFR32_RTC_USE_RTC1) && (EFR32_RTC_USE_RTC1 == TRUE) && !defined(__DOXYGEN__)
#include "hal_rtc_lld.h"
#endif

#if (OSAL_ST_MODE != OSAL_ST_MODE_NONE) || defined(__DOXYGEN__)

#if OSAL_ST_MODE == OSAL_ST_MODE_PERIODIC
#error "SYSTICKv4 (BURTC) supports only OSAL_ST_MODE_FREERUNNING"
#endif

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define ST_HANDLER      EFR32_BURTC_HANDLER
#define ST_NUMBER       EFR32_BURTC_NUMBER

/* BURTC clock — EM4GRPACLK (LFXO = 32768 Hz typically). The counter
   prescaler (CFG CNTPRESC) divides it down to OSAL_ST_FREQUENCY. */
#define ST_CLOCK_SRC    EFR32_BURTCCLK

#if (ST_CLOCK_SRC / OSAL_ST_FREQUENCY == 1U)
  #define EFR32_ST_PRESC    BURTC_CFG_CNTPRESC_DIV1
#elif (ST_CLOCK_SRC / OSAL_ST_FREQUENCY == 2U)
  #define EFR32_ST_PRESC    BURTC_CFG_CNTPRESC_DIV2
#elif (ST_CLOCK_SRC / OSAL_ST_FREQUENCY == 4U)
  #define EFR32_ST_PRESC    BURTC_CFG_CNTPRESC_DIV4
#elif (ST_CLOCK_SRC / OSAL_ST_FREQUENCY == 8U)
  #define EFR32_ST_PRESC    BURTC_CFG_CNTPRESC_DIV8
#elif (ST_CLOCK_SRC / OSAL_ST_FREQUENCY == 16U)
  #define EFR32_ST_PRESC    BURTC_CFG_CNTPRESC_DIV16
#elif (ST_CLOCK_SRC / OSAL_ST_FREQUENCY == 32U)
  #define EFR32_ST_PRESC    BURTC_CFG_CNTPRESC_DIV32
#elif (ST_CLOCK_SRC / OSAL_ST_FREQUENCY == 64U)
  #define EFR32_ST_PRESC    BURTC_CFG_CNTPRESC_DIV64
#elif (ST_CLOCK_SRC / OSAL_ST_FREQUENCY == 128U)
  #define EFR32_ST_PRESC    BURTC_CFG_CNTPRESC_DIV128
#elif (ST_CLOCK_SRC / OSAL_ST_FREQUENCY == 256U)
  #define EFR32_ST_PRESC    BURTC_CFG_CNTPRESC_DIV256
#else
  #error "EFR32_ST_PRESC is not obtainable"
#endif

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

OSAL_IRQ_HANDLER(ST_HANDLER) {
  OSAL_IRQ_PROLOGUE();
#if (HAL_USE_RTC == TRUE) && defined(EFR32_RTC_USE_RTC1) && (EFR32_RTC_USE_RTC1 == TRUE) && !defined(__DOXYGEN__)
  /* Forward BURTCv1 (RTC calendar) events first. */
  rtc_lld_serve_interrupt();
#endif
  st_lld_serve_interrupt();
  OSAL_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void st_lld_init(void) {

  osalDbgAssert((ST_CLOCK_SRC % OSAL_ST_FREQUENCY) == 0U,
                "clock rounding error");

  /* Enable the BURTC bus clock. */
  CMU->CLKEN0_SET = CMU_CLKEN0_BURTC;

  /* Select the EM4 group A clock source (BURTC runs from EM4GRPACLK).
     The reset default is LFRCO, which may be disabled — without a running
     clock SYNCBUSY never clears.  CH_CFG_ST_FREQUENCY must match the
     selected source (LFXO = 32768 Hz). */
#if EFR32_LFXO_ENABLED
  CMU->EM4GRPACLKCTRL = (CMU->EM4GRPACLKCTRL & ~_CMU_EM4GRPACLKCTRL_CLKSEL_MASK)
                        | CMU_EM4GRPACLKCTRL_CLKSEL_LFXO;
#elif EFR32_LFRCO_ENABLED
  CMU->EM4GRPACLKCTRL = (CMU->EM4GRPACLKCTRL & ~_CMU_EM4GRPACLKCTRL_CLKSEL_MASK)
                        | CMU_EM4GRPACLKCTRL_CLKSEL_LFRCO;
#elif EFR32_ULFRCO_ENABLED
  CMU->EM4GRPACLKCTRL = (CMU->EM4GRPACLKCTRL & ~_CMU_EM4GRPACLKCTRL_CLKSEL_MASK)
                        | CMU_EM4GRPACLKCTRL_CLKSEL_ULFRCO;
#endif

  /* CFG is only writable while the module is disabled (writing it with
     EN=1 bus-faults on Series 2 — same order as emlib BURTC_Init). */
  if ((BURTC->EN & BURTC_EN_EN) != 0U) {
    BURTC->EN_CLR = BURTC_EN_EN;
    while ((BURTC->EN & BURTC_EN_DISABLING) != 0U) {}
  }

  /* Counter prescaler + compare against CNT (not the top). DEBUGRUN keeps
     the counter running while the debugger has the CPU halted (RM 16.3.3),
     otherwise CNT freezes on every halt and rate measurements are bogus. */
  BURTC->CFG = (BURTC->CFG & ~(_BURTC_CFG_CNTPRESC_MASK | _BURTC_CFG_COMPTOP_MASK |
                                _BURTC_CFG_DEBUGRUN_MASK))
               | EFR32_ST_PRESC | BURTC_CFG_COMPTOP | BURTC_CFG_DEBUGRUN;

  /* Enable module (LF domain starts clocking).  NOTE: CFG above had to be
     written while disabled, but the async-domain registers (COMP, CNT)
     below must be written while enabled, each followed by a SYNCBUSY
     wait — writing them with EN=0 bus-faults. */
  BURTC->EN_SET = BURTC_EN_EN;
  while ((BURTC->EN & BURTC_EN_EN) == 0U) {}

  BURTC->IF_CLR = _BURTC_IF_MASK;
  BURTC->IEN = 0U;

  /* FREERUNNING mode: the counter runs freely and the kernel drives the
     alarm through COMP (st_lld_start_alarm/st_lld_set_alarm), so init
     must NOT arm COMP itself — enable only OF here. */
  BURTC->COMP = 0U;
  while ((BURTC->SYNCBUSY & BURTC_SYNCBUSY_COMP) != 0U) {}
  BURTC->IEN_SET = BURTC_IEN_OF;

  /* Start counter from 0. */
  BURTC->CNT = 0U;
  while ((BURTC->SYNCBUSY & BURTC_SYNCBUSY_CNT) != 0U) {}

  BURTC->CMD = BURTC_CMD_START;
  while ((BURTC->SYNCBUSY & BURTC_SYNCBUSY_START) != 0U) {}

  nvicEnableVector(ST_NUMBER, EFR32_BURTC_ST_IRQ_PRIORITY);
}

void st_lld_serve_interrupt(void) {
  uint32_t isr = BURTC->IF;

  if ((isr & BURTC_IF_COMP) != 0U) {
    BURTC->IF_CLR = BURTC_IF_COMP;
    /* FREERUNNING: COMP is the kernel alarm — do not re-arm here, the
       kernel sets the next alarm via stSetAlarm(). */
    osalSysLockFromISR();
    osalOsTimerHandlerI();
    osalSysUnlockFromISR();
  }

  if ((isr & BURTC_IF_OF) != 0U) {
    BURTC->IF_CLR = BURTC_IF_OF;
    /* 32-bit overflow — OSAL handles this via systime_t modulo arithmetic,
       no action needed here. */
  }
}

#endif /* OSAL_ST_MODE != OSAL_ST_MODE_NONE */

/** @} */
