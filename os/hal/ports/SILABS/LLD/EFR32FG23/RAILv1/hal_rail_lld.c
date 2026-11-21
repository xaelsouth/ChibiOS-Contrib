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
 * @file    RAILv1/hal_rail_lld.c
 * @brief   EFR32FG23 RAIL glue: emlib compatibility shims + radio IRQ wrappers.
 *
 * @details The Gecko SDK RAIL library and emlib expect certain functions
 *          (CORE_EnterCritical, SystemSYSCLKGet, CMU_ClockEnable, etc.)
 *          to be provided by the application. This file implements them
 *          on top of ChibiOS primitives so the RAIL library links cleanly.
 *
 *          It also provides OSAL_IRQ_HANDLER wrappers for all radio-related
 *          interrupt vectors, dispatching to the RAIL library's own
 *          xxx_IRQHandler() functions.
 *
 * @addtogroup RAIL
 * @{
 */

#include "hal.h"

#if EFR32_RAIL_USE_RAIL || defined(__DOXYGEN__)

/*===========================================================================*/
/* emlib compatibility shims.                                                */
/*===========================================================================*/

/* emlib em_core expects these. */
typedef uint32_t CORE_irqState_t;

CORE_irqState_t CORE_EnterCritical(void) {
  return osalSysGetStatusAndLockX();
}

void CORE_ExitCritical(CORE_irqState_t irqState) {
  osalSysRestoreStatusX(irqState);
}

CORE_irqState_t CORE_EnterAtomic(void) {
  return osalSysGetStatusAndLockX();
}

void CORE_ExitAtomic(CORE_irqState_t irqState) {
  osalSysRestoreStatusX(irqState);
}

/* emlib em_system expects this. */
#if !defined(_SYSCFG_CHIPREV_PARTNUMBER_MASK)
typedef struct {
  uint8_t minor;
  uint8_t major;
  uint8_t family;
} SYSTEM_ChipRevision_TypeDef;
#else
typedef struct {
  uint8_t  minor;
  uint8_t  major;
  uint16_t partNumber;
} SYSTEM_ChipRevision_TypeDef;
#endif

void SYSTEM_ChipRevisionGet(SYSTEM_ChipRevision_TypeDef *rev) {
  CMU->CLKEN0_SET = CMU_CLKEN0_SYSCFG;
  uint32_t chiprev = SYSCFG->CHIPREV;

#if defined(_SYSCFG_CHIPREV_PARTNUMBER_MASK)
  rev->partNumber = (uint16_t)(((chiprev & SYSCFG_CHIPREV_PARTNUMBER1) >> 5)
                                | (chiprev & SYSCFG_CHIPREV_PARTNUMBER0));
#else
  rev->family = (uint8_t)((chiprev & _SYSCFG_CHIPREV_FAMILY_MASK)
                          >> _SYSCFG_CHIPREV_FAMILY_SHIFT);
#endif
  rev->major  = (uint8_t)((chiprev & _SYSCFG_CHIPREV_MAJOR_MASK)
                          >> _SYSCFG_CHIPREV_MAJOR_SHIFT);
  rev->minor  = (uint8_t)((chiprev & _SYSCFG_CHIPREV_MINOR_MASK)
                          >> _SYSCFG_CHIPREV_MINOR_SHIFT);
}

/* emlib em_cmu expects this typedef and function. */
typedef enum { cmuClock_NONE = 0 } CMU_Clock_TypeDef;

void CMU_ClockEnable(CMU_Clock_TypeDef clock, bool enable) {
  /* On FG23, all peripheral clocks are controlled via CLKEN0/CLKEN1
     registers, not via CMU_ClockEnable. This is a stub for RAIL
     compatibility — the RAIL library calls it with cmuClock_PRS. */
  (void)clock;
  (void)enable;
}

/* Clock getters — declared CC_WEAK so they can be overridden. */
CC_WEAK uint32_t SystemHFXOClockGet(void) {
  return EFR32_HFXO_FREQ;
}

CC_WEAK uint32_t SystemHFRCODPLLClockGet(void) {
  /* Read HFRCODPLL frequency from hardware calibration. */
  uint32_t ret = 0U;
  switch ((HFRCO0->CAL & _HFRCO_CAL_FREQRANGE_MASK)
          >> _HFRCO_CAL_FREQRANGE_SHIFT) {
  case 0:
    switch (HFRCO0->CAL & _HFRCO_CAL_CLKDIV_MASK) {
    case HFRCO_CAL_CLKDIV_DIV1: ret = 4000000UL; break;
    case HFRCO_CAL_CLKDIV_DIV2: ret = 2000000UL; break;
    case HFRCO_CAL_CLKDIV_DIV4: ret = 1000000UL; break;
    default: break;
    }
    break;
  case 3:  ret = 7000000UL;  break;
  case 6:  ret = 13000000UL; break;
  case 7:  ret = 16000000UL; break;
  case 8:  ret = 19000000UL; break;
  case 10: ret = 26000000UL; break;
  case 11: ret = 32000000UL; break;
  case 12: ret = 38000000UL; break;
  case 13: ret = 48000000UL; break;
  case 14: ret = 56000000UL; break;
  case 15: ret = 64000000UL; break;
  case 16: ret = 80000000UL; break;
  default: break;
  }
  return ret;
}

CC_WEAK uint32_t SystemSYSCLKGet(void) {
  switch (CMU->SYSCLKCTRL & _CMU_SYSCLKCTRL_CLKSEL_MASK) {
  case _CMU_SYSCLKCTRL_CLKSEL_HFRCODPLL:
    return SystemHFRCODPLLClockGet();
#if EFR32_HFXO_FREQ > 0U
  case _CMU_SYSCLKCTRL_CLKSEL_HFXO:
    return EFR32_HFXO_FREQ;
#endif
#if EFR32_CLKIN0_FREQ > 0U
  case _CMU_SYSCLKCTRL_CLKSEL_CLKIN0:
    return EFR32_CLKIN0_FREQ;
#endif
  case _CMU_SYSCLKCTRL_CLKSEL_FSRCO:
    return EFR32_FSRCO_FREQ;
  default:
    osalDbgAssert(false, "Unknown SYSCLK source");
    return 0U;
  }
}

#if defined(SL_TRUSTZONE_NONSECURE)
uint32_t sli_tz_syscfg_read_chiprev_register(void) {
  return SYSCFG->CHIPREV;
}
#endif

/*===========================================================================*/
/* Radio IRQ handler wrappers.                                               */
/*===========================================================================*/

OSAL_IRQ_HANDLER(EFR32_RFECA0_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  RFECA0_IRQHandler();
  OSAL_IRQ_EPILOGUE();
}

OSAL_IRQ_HANDLER(EFR32_RFECA1_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  RFECA1_IRQHandler();
  OSAL_IRQ_EPILOGUE();
}

OSAL_IRQ_HANDLER(EFR32_SYSRTC_SEQ_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  SYSRTC_SEQ_IRQHandler();
  OSAL_IRQ_EPILOGUE();
}

OSAL_IRQ_HANDLER(EFR32_AGC_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  AGC_IRQHandler();
  OSAL_IRQ_EPILOGUE();
}

OSAL_IRQ_HANDLER(EFR32_BUFC_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  BUFC_IRQHandler();
  OSAL_IRQ_EPILOGUE();
}

OSAL_IRQ_HANDLER(EFR32_EMUDG_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  EMUDG_IRQHandler();
  OSAL_IRQ_EPILOGUE();
}

OSAL_IRQ_HANDLER(EFR32_FRC_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  FRC_IRQHandler();
  OSAL_IRQ_EPILOGUE();
}

OSAL_IRQ_HANDLER(EFR32_FRC_PRI_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  FRC_PRI_IRQHandler();
  OSAL_IRQ_EPILOGUE();
}

OSAL_IRQ_HANDLER(EFR32_HOSTMAILBOX_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  HOSTMAILBOX_IRQHandler();
  OSAL_IRQ_EPILOGUE();
}

OSAL_IRQ_HANDLER(EFR32_MODEM_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  MODEM_IRQHandler();
  OSAL_IRQ_EPILOGUE();
}

OSAL_IRQ_HANDLER(EFR32_PROTIMER_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  PROTIMER_IRQHandler();
  OSAL_IRQ_EPILOGUE();
}

OSAL_IRQ_HANDLER(EFR32_RAC_RSM_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  RAC_RSM_IRQHandler();
  OSAL_IRQ_EPILOGUE();
}

OSAL_IRQ_HANDLER(EFR32_RAC_SEQ_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  RAC_SEQ_IRQHandler();
  OSAL_IRQ_EPILOGUE();
}

OSAL_IRQ_HANDLER(EFR32_SYNTH_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  SYNTH_IRQHandler();
  OSAL_IRQ_EPILOGUE();
}

#endif /* EFR32_RAIL_USE_RAIL */

/** @} */
