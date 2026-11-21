/*
    ChibiOS - Copyright (C) 2026 Xael South
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
        http://www.apache.org/licenses/LICENSE-2.0
*/

/**
 * @file    QEIv1/hal_qei_lld.h
 * @brief   Quadrature encoder interface (PCNT0) LLD header.
 *
 * @addtogroup QEI
 * @{
 */

#ifndef HAL_QEI_LLD_H
#define HAL_QEI_LLD_H

#if (HAL_USE_QEI == TRUE) || defined(__DOXYGEN__)

#if !defined(EFR32_QEI_USE_PCNT0) || defined(__DOXYGEN__)
#define EFR32_QEI_USE_PCNT0     FALSE
#endif
#if !defined(EFR32_QEI_PCNT0_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_QEI_PCNT0_IRQ_PRIORITY    3
#endif

typedef enum {
  QEI_UNINIT = 0, QEI_STOP = 1, QEI_READY = 2
} qeistate_t;

typedef void (*qeicallback_t)(void *arg, uint32_t flags);

typedef struct {
  uint32_t  cfg;        /**< PCNT_CFG (mode, filter, hysteresis). */
  uint32_t  ctrl;       /**< PCNT_CTRL (edge, count event, direction). */
  uint32_t  top;        /**< PCNT_TOP (counter max). */
  qeicallback_t callback;
  void     *callback_arg;
} QEIConfig;

typedef struct {
  qeistate_t        state;
  const QEIConfig  *config;
} QEIDriver;

#if EFR32_QEI_USE_PCNT0 && !defined(__DOXYGEN__)
extern QEIDriver QEID1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void qei_lld_init(void);
  void qei_lld_start(QEIDriver *qp, const QEIConfig *cfg);
  void qei_lld_stop(QEIDriver *qp);
  int32_t qei_lld_get_count(QEIDriver *qp);
  void qei_lld_set_count(QEIDriver *qp, int32_t count);
  void qei_lld_serve_interrupt(QEIDriver *qp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_QEI */
#endif /* HAL_QEI_LLD_H */
/** @} */
