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

/* qeistate_t, qeicallback_t and the QEIDriver forward declaration come from
   the upper hal_qei.h, which includes this header. */

/** Counter type (PCNT CNT is 16-bit on xG23, sign-extended here). */
typedef int32_t qeicnt_t;
/** Counter delta type. */
typedef int32_t qeidelta_t;

/** Counter limits (PCNT CNT is 16-bit on xG23). */
#define QEI_COUNT_MIN   ((qeicnt_t)-32768)
#define QEI_COUNT_MAX   ((qeicnt_t)32767)

/** Overflow callback type. */
typedef void (*qeioverflowcb_t)(QEIDriver *qeip, qeidelta_t delta);

typedef struct {
  uint32_t  cfg;        /**< PCNT_CFG (mode, filter, hysteresis). */
  uint32_t  ctrl;       /**< PCNT_CTRL (edge, count event, direction). */
  uint32_t  top;        /**< PCNT_TOP (counter max). */
  uint8_t   s0prs;      /**< PRS async channel for S0IN (0..11), 0xFF = GPIO pin. */
  uint8_t   s1prs;      /**< PRS async channel for S1IN (0..11), 0xFF = GPIO pin. */
  qeicnt_t  min;        /**< Min count (informational for upper layer). */
  qeicnt_t  max;        /**< Max count (informational for upper layer). */
  qeioverflow_t overflow;       /**< Overflow handling mode. */
  qeioverflowcb_t overflow_cb;  /**< Overflow callback. */
  qeicallback_t callback;
} QEIConfig;

struct QEIDriver {
  qeistate_t        state;
  const QEIConfig  *config;
  qeicnt_t          last;   /**< Last sampled count (upper layer delta calc). */
};

#if EFR32_QEI_USE_PCNT0 && !defined(__DOXYGEN__)
extern QEIDriver QEID1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void qei_lld_init(void);
  void qei_lld_start(QEIDriver *qp);
  void qei_lld_stop(QEIDriver *qp);
  void qei_lld_enable(QEIDriver *qp);
  void qei_lld_disable(QEIDriver *qp);
  int32_t qei_lld_get_count(QEIDriver *qp);
  void qei_lld_set_count(QEIDriver *qp, int32_t count);
  void qei_lld_serve_interrupt(QEIDriver *qp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_QEI */
#endif /* HAL_QEI_LLD_H */
/** @} */
