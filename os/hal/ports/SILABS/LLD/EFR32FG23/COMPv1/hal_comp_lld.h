/*
    ChibiOS - Copyright (C) 2026 Xael South
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
        http://www.apache.org/licenses/LICENSE-2.0
*/

/**
 * @file    COMPv1/hal_comp_lld.h
 * @brief   Analog comparator (ACMP) LLD header.
 *
 * @addtogroup COMP
 * @{
 */

#ifndef HAL_COMP_LLD_H
#define HAL_COMP_LLD_H

#if (HAL_USE_COMP == TRUE) || defined(__DOXYGEN__)

#if !defined(EFR32_COMP_USE_ACMP0) || defined(__DOXYGEN__)
#define EFR32_COMP_USE_ACMP0    FALSE
#endif
#if !defined(EFR32_COMP_USE_ACMP1) || defined(__DOXYGEN__)
#define EFR32_COMP_USE_ACMP1    FALSE
#endif
#if !defined(EFR32_COMP_ACMP0_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_COMP_ACMP0_IRQ_PRIORITY   3
#endif
#if !defined(EFR32_COMP_ACMP1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_COMP_ACMP1_IRQ_PRIORITY   3
#endif

typedef enum {
  COMP_UNINIT = 0, COMP_STOP = 1, COMP_READY = 2
} compstate_t;

typedef void (*compcallback_t)(void *arg, uint32_t flags);

typedef struct {
  uint32_t  cfg;        /**< ACMP_CFG (bias, hysteresis, input range, accuracy). */
  uint32_t  ctrl;       /**< ACMP_CTRL (invert, etc.). */
  uint32_t  inputctrl;  /**< ACMP_INPUTCTRL (pos/neg input select, VREFDIV). */
  compcallback_t callback;
  void     *callback_arg;
} COMPConfig;

typedef struct {
  compstate_t       state;
  const COMPConfig *config;
} COMPDriver;

#if EFR32_COMP_USE_ACMP0 && !defined(__DOXYGEN__)
extern COMPDriver COMPD1;
#endif
#if EFR32_COMP_USE_ACMP1 && !defined(__DOXYGEN__)
extern COMPDriver COMPD2;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void comp_lld_init(void);
  void comp_lld_start(COMPDriver *cp, const COMPConfig *cfg);
  void comp_lld_stop(COMPDriver *cp);
  bool comp_lld_get_output(COMPDriver *cp);
  void comp_lld_serve_interrupt(COMPDriver *cp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_COMP */
#endif /* HAL_COMP_LLD_H */
/** @} */
