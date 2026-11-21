/*
    ChibiOS - Copyright (C) 2026 Xael South
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
        http://www.apache.org/licenses/LICENSE-2.0
*/

/**
 * @file    QEIv1/hal_qei_lld.c
 * @brief   Quadrature encoder (PCNT0) LLD source.
 *
 * @addtogroup QEI
 * @{
 */

#include "hal.h"

#if (HAL_USE_QEI == TRUE) || defined(__DOXYGEN__)

#if EFR32_QEI_USE_PCNT0 || defined(__DOXYGEN__)
QEIDriver QEID1;

OSAL_IRQ_HANDLER(EFR32_PCNT0_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  qei_lld_serve_interrupt(&QEID1);
  OSAL_IRQ_EPILOGUE();
}
#endif

void qei_lld_init(void) {
#if EFR32_QEI_USE_PCNT0
  QEID1.state = QEI_STOP; QEID1.config = NULL;
#endif
}

void qei_lld_start(QEIDriver *qp, const QEIConfig *cfg) {
  qp->config = cfg;
  CMU->CLKEN1_SET = CMU_CLKEN1_PCNT0;

  PCNT0->EN_CLR = PCNT_EN_EN;
  PCNT0->CFG = cfg->cfg;
  PCNT0->CTRL = cfg->ctrl;
  PCNT0->TOP = cfg->top;
  PCNT0->TOPB = cfg->top;
  PCNT0->CNT = 0;
  PCNT0->IF_CLR = _PCNT_IF_MASK;
  PCNT0->IEN_SET = PCNT_IEN_OF | PCNT_IEN_UF | PCNT_IEN_DIRCNG;
  PCNT0->EN_SET = PCNT_EN_EN;
  while ((PCNT0->SYNCBUSY & _PCNT_SYNCBUSY_MASK) != 0U) {}
  PCNT0->CMD_SET = PCNT_CMD_STARTCNT;

  nvicEnableVector(EFR32_PCNT0_NUMBER, EFR32_QEI_PCNT0_IRQ_PRIORITY);
  qp->state = QEI_READY;
}

void qei_lld_stop(QEIDriver *qp) {
  nvicDisableVector(EFR32_PCNT0_NUMBER);
  PCNT0->CMD_SET = PCNT_CMD_STOPCNT;
  PCNT0->IEN_CLR = _PCNT_IEN_MASK;
  PCNT0->EN_CLR = PCNT_EN_EN;
  while ((PCNT0->EN & PCNT_EN_DISABLING) == PCNT_EN_DISABLING) {}
  CMU->CLKEN1_CLR = CMU_CLKEN1_PCNT0;
  qp->state = QEI_STOP;
  qp->config = NULL;
}

int32_t qei_lld_get_count(QEIDriver *qp) {
  (void)qp;
  return (int32_t)PCNT0->CNT;
}

void qei_lld_set_count(QEIDriver *qp, int32_t count) {
  (void)qp;
  PCNT0->CNT = (uint32_t)count;
}

void qei_lld_serve_interrupt(QEIDriver *qp) {
  uint32_t isr = PCNT0->IF;
  PCNT0->IF_CLR = isr;
  if (qp->config && qp->config->callback)
    qp->config->callback(qp->config->callback_arg, isr);
}

#endif /* HAL_USE_QEI */
/** @} */
