/*
    ChibiOS - Copyright (C) 2026 Xael South
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
        http://www.apache.org/licenses/LICENSE-2.0
*/

/**
 * @file    COMPv1/hal_comp_lld.c
 * @brief   Analog comparator (ACMP) LLD source.
 *
 * @addtogroup COMP
 * @{
 */

#include "hal.h"

#if (HAL_USE_COMP == TRUE) || defined(__DOXYGEN__)

static ACMP_TypeDef * const comp_periph[2] = { ACMP0, ACMP1 };
static const uint32_t comp_clken[2] = { CMU_CLKEN1_ACMP0, CMU_CLKEN1_ACMP1 };
static const uint32_t comp_irqn[2] = { EFR32_ACMP1_NUMBER, EFR32_ACMP2_NUMBER };
static const uint32_t comp_irqprio[2] = {
  EFR32_COMP_ACMP0_IRQ_PRIORITY, EFR32_COMP_ACMP1_IRQ_PRIORITY
};

#if EFR32_COMP_USE_ACMP0 || defined(__DOXYGEN__)
COMPDriver COMPD1;
#endif
#if EFR32_COMP_USE_ACMP1 || defined(__DOXYGEN__)
COMPDriver COMPD2;
#endif

static uint32_t comp_idx_of(COMPDriver *cp) {
#if EFR32_COMP_USE_ACMP1
  if (cp == &COMPD2) return 1U;
#else
  (void)cp;
#endif
  return 0U;
}

static COMPDriver *comp_get(uint32_t idx) {
#if EFR32_COMP_USE_ACMP0
  if (idx == 0) return &COMPD1;
#endif
#if EFR32_COMP_USE_ACMP1
  if (idx == 1) return &COMPD2;
#endif
  return NULL;
}

#if EFR32_COMP_USE_ACMP0 || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_ACMP1_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  comp_lld_serve_interrupt(&COMPD1);
  OSAL_IRQ_EPILOGUE();
}
#endif
#if EFR32_COMP_USE_ACMP1 || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_ACMP2_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  comp_lld_serve_interrupt(&COMPD2);
  OSAL_IRQ_EPILOGUE();
}
#endif

void comp_lld_init(void) {
#if EFR32_COMP_USE_ACMP0
  COMPD1.state = COMP_STOP; COMPD1.config = NULL;
#endif
#if EFR32_COMP_USE_ACMP1
  COMPD2.state = COMP_STOP; COMPD2.config = NULL;
#endif
}

void comp_lld_start(COMPDriver *cp) {
  uint32_t idx = comp_idx_of(cp);
  ACMP_TypeDef *a = comp_periph[idx];
  const COMPConfig *cfg = cp->config;

  CMU->CLKEN1_SET = comp_clken[idx];

  a->EN_CLR = ACMP_EN_EN;
  while ((a->EN & ACMP_EN_DISABLING) == ACMP_EN_DISABLING) {}
  a->CFG = cfg->cfg;
  a->CTRL = cfg->ctrl;
  a->EN_SET = ACMP_EN_EN;
  /* On xG23 INPUTCTRL must only be written while the ACMP is enabled,
     writing it while disabled bus-faults. */
  a->INPUTCTRL = cfg->inputctrl;
  a->IF_CLR = _ACMP_IF_MASK;
  a->IEN_SET = ACMP_IEN_RISE | ACMP_IEN_FALL;
  while ((a->STATUS & ACMP_STATUS_ACMPRDY) == 0U) {}

  nvicEnableVector(comp_irqn[idx], comp_irqprio[idx]);
  cp->state = COMP_READY;
}

void comp_lld_enable(COMPDriver *cp) {
  uint32_t idx = comp_idx_of(cp);
  comp_periph[idx]->IEN_SET = ACMP_IEN_RISE | ACMP_IEN_FALL;
}

void comp_lld_disable(COMPDriver *cp) {
  uint32_t idx = comp_idx_of(cp);
  comp_periph[idx]->IEN_CLR = _ACMP_IEN_MASK;
}

void comp_lld_stop(COMPDriver *cp) {
  uint32_t idx = comp_idx_of(cp);
  ACMP_TypeDef *a = comp_periph[idx];

  nvicDisableVector(comp_irqn[idx]);
  a->IEN_CLR = _ACMP_IEN_MASK;
  a->EN_CLR = ACMP_EN_EN;
  while ((a->EN & ACMP_EN_DISABLING) == ACMP_EN_DISABLING) {}
  CMU->CLKEN1_CLR = comp_clken[idx];
  cp->state = COMP_STOP;
}

bool comp_lld_get_output(COMPDriver *cp) {
  uint32_t idx = comp_idx_of(cp);
  return (comp_periph[idx]->STATUS & ACMP_STATUS_ACMPOUT) != 0U;
}

void comp_lld_serve_interrupt(COMPDriver *cp) {
  uint32_t idx = comp_idx_of(cp);
  ACMP_TypeDef *a = comp_periph[idx];
  uint32_t isr = a->IF;
  a->IF_CLR = isr;

  (void)isr;
  if (cp->config && cp->config->cb)
    _comp_isr_invoke_cb(cp);
}

#endif /* HAL_USE_COMP */
/** @} */
