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

/* compstate_t, compcallback_t and the forward declaration of COMPDriver
   come from the upper hal_comp.h. */

typedef struct {
  compcallback_t cb;    /**< Notification callback (upper-layer contract). */
  uint32_t  cfg;        /**< ACMP_CFG (bias, hysteresis, input range, accuracy). */
  uint32_t  ctrl;       /**< ACMP_CTRL (invert, etc.). */
  uint32_t  inputctrl;  /**< ACMP_INPUTCTRL (pos/neg input select, VREFDIV). */
} COMPConfig;

struct COMPDriver {
  compstate_t       state;
  const COMPConfig *config;
};

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
  void comp_lld_start(COMPDriver *cp);
  void comp_lld_stop(COMPDriver *cp);
  void comp_lld_enable(COMPDriver *cp);
  void comp_lld_disable(COMPDriver *cp);
  bool comp_lld_get_output(COMPDriver *cp);
  void comp_lld_serve_interrupt(COMPDriver *cp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_COMP */
#endif /* HAL_COMP_LLD_H */
/** @} */
