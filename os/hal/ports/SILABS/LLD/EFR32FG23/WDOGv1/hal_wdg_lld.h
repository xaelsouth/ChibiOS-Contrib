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
 * @file    WDOGv1/hal_wdg_lld.h
 * @brief   WDG Driver subsystem low level driver header.
 *
 * @addtogroup WDG
 * @{
 */

#ifndef HAL_WDG_LLD_H
#define HAL_WDG_LLD_H

#if (HAL_USE_WDG == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    WDOG clock source selection (CMU_WDOGxCLKCTRL_CLKSEL)
 * @{
 */
#define EFR32_WDOG_CLKSEL_LFRCO         CMU_WDOG0CLKCTRL_CLKSEL_LFRCO
#define EFR32_WDOG_CLKSEL_LFXO          CMU_WDOG0CLKCTRL_CLKSEL_LFXO
#define EFR32_WDOG_CLKSEL_ULFRCO        CMU_WDOG0CLKCTRL_CLKSEL_ULFRCO
#define EFR32_WDOG_CLKSEL_HCLKDIV1024   CMU_WDOG0CLKCTRL_CLKSEL_HCLKDIV1024
/** @} */

/**
 * @name    WDOG timeout period selectors (CFG_PERSEL)
 * @details Each selector N gives a timeout of 2^(N+3) WDOG clock cycles.
 *          At 32768 Hz (LFRCO/LFXO): SEL0 = 244 us ... SEL15 = 8 s.
 *          At 1000 Hz (ULFRCO): SEL0 = 8 ms ... SEL15 = 262 s.
 * @{
 */
#define EFR32_WDOG_PERSEL_0             (0U << _WDOG_CFG_PERSEL_SHIFT)   /**< 2^3   cycles */
#define EFR32_WDOG_PERSEL_1             (1U << _WDOG_CFG_PERSEL_SHIFT)   /**< 2^4   cycles */
#define EFR32_WDOG_PERSEL_2             (2U << _WDOG_CFG_PERSEL_SHIFT)   /**< 2^5   cycles */
#define EFR32_WDOG_PERSEL_3             (3U << _WDOG_CFG_PERSEL_SHIFT)   /**< 2^6   cycles */
#define EFR32_WDOG_PERSEL_4             (4U << _WDOG_CFG_PERSEL_SHIFT)   /**< 2^7   cycles */
#define EFR32_WDOG_PERSEL_5             (5U << _WDOG_CFG_PERSEL_SHIFT)   /**< 2^8   cycles */
#define EFR32_WDOG_PERSEL_6             (6U << _WDOG_CFG_PERSEL_SHIFT)   /**< 2^9   cycles */
#define EFR32_WDOG_PERSEL_7             (7U << _WDOG_CFG_PERSEL_SHIFT)   /**< 2^10  cycles */
#define EFR32_WDOG_PERSEL_8             (8U << _WDOG_CFG_PERSEL_SHIFT)   /**< 2^11  cycles */
#define EFR32_WDOG_PERSEL_9             (9U << _WDOG_CFG_PERSEL_SHIFT)   /**< 2^12  cycles */
#define EFR32_WDOG_PERSEL_10            (10U << _WDOG_CFG_PERSEL_SHIFT)  /**< 2^13  cycles */
#define EFR32_WDOG_PERSEL_11            (11U << _WDOG_CFG_PERSEL_SHIFT)  /**< 2^14  cycles */
#define EFR32_WDOG_PERSEL_12            (12U << _WDOG_CFG_PERSEL_SHIFT)  /**< 2^15  cycles */
#define EFR32_WDOG_PERSEL_13            (13U << _WDOG_CFG_PERSEL_SHIFT)  /**< 2^16  cycles */
#define EFR32_WDOG_PERSEL_14            (14U << _WDOG_CFG_PERSEL_SHIFT)  /**< 2^17  cycles */
#define EFR32_WDOG_PERSEL_15            (15U << _WDOG_CFG_PERSEL_SHIFT)  /**< 2^18  cycles */
/** @} */

/**
 * @name    WDOG warning interrupt selectors (CFG_WARNSEL)
 * @details Warning fires at a fraction of the timeout period.
 * @{
 */
#define EFR32_WDOG_WARNSEL_DIS          (0U << _WDOG_CFG_WARNSEL_SHIFT)  /**< Disabled       */
#define EFR32_WDOG_WARNSEL_SEL1         (1U << _WDOG_CFG_WARNSEL_SHIFT)  /**< 1/2 of period  */
#define EFR32_WDOG_WARNSEL_SEL2         (2U << _WDOG_CFG_WARNSEL_SHIFT)  /**< 3/4 of period  */
#define EFR32_WDOG_WARNSEL_SEL3         (3U << _WDOG_CFG_WARNSEL_SHIFT)  /**< 7/8 of period  */
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   WDOG1 driver enable switch (hardware WDOG0).
 * @note    The default is @p FALSE.
 */
#if !defined(EFR32_WDG_USE_WDOG0) || defined(__DOXYGEN__)
#define EFR32_WDG_USE_WDOG0             FALSE
#endif

/**
 * @brief   WDOG2 driver enable switch (hardware WDOG1).
 * @note    The default is @p FALSE.
 */
#if !defined(EFR32_WDG_USE_WDOG1) || defined(__DOXYGEN__)
#define EFR32_WDG_USE_WDOG1             FALSE
#endif

/**
 * @brief   WDOG0 interrupt priority.
 */
#if !defined(EFR32_WDG_WDOG0_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_WDG_WDOG0_IRQ_PRIORITY    2
#endif

/**
 * @brief   WDOG1 interrupt priority.
 */
#if !defined(EFR32_WDG_WDOG1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_WDG_WDOG1_IRQ_PRIORITY    2
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !EFR32_WDG_USE_WDOG0 && !EFR32_WDG_USE_WDOG1
#error "WDG driver activated but no WDOG peripheral assigned"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a structure representing a WDG driver.
 */
typedef struct WDGDriver WDGDriver;

/**
 * @brief   Driver configuration structure.
 */
typedef struct {
  /**
   * @brief   WDOG clock source (CMU_WDOGxCLKCTRL_CLKSEL value).
   * @details One of @p EFR32_WDOG_CLKSEL_*.
   */
  uint32_t                  clksel;
  /**
   * @brief   Timeout period selector (CFG_PERSEL value).
   * @details One of @p EFR32_WDOG_PERSEL_*.
   */
  uint32_t                  persel;
  /**
   * @brief   Warning interrupt selector (CFG_WARNSEL value).
   * @details One of @p EFR32_WDOG_WARNSEL_*. Set to DIS to disable.
   */
  uint32_t                  warnsel;
  /**
   * @brief   Run in EM1 enable (CFG_EM1RUN).
   */
  bool                      em1run;
  /**
   * @brief   Run in EM2 enable (CFG_EM2RUN).
   */
  bool                      em2run;
  /**
   * @brief   Run in EM3 enable (CFG_EM3RUN).
   */
  bool                      em3run;
  /**
   * @brief   Pause in debug enable (CFG_DEBUGRUN).
   */
  bool                      debugrun;
  /**
   * @brief   Lock the WDOG configuration after start.
   * @details If true, the WDOG cannot be stopped or reconfigured until reset.
   */
  bool                      lock;
  /**
   * @brief   Warning interrupt callback, or NULL.
   * @details Called from the WDOG warning IRQ when the counter reaches
   *          the warning threshold. Use this to kick the dog early.
   */
  void                    (*warn_cb)(WDGDriver *wdgp);
} WDGConfig;

/**
 * @brief   Structure representing a WDG driver.
 */
struct WDGDriver {
  /**
   * @brief   Driver state.
   */
  wdgstate_t                state;
  /**
   * @brief   Current configuration data.
   */
  const WDGConfig           *config;
  /* End of the mandatory fields.*/
  /**
   * @brief   Pointer to the WDOG registers block.
   */
  WDOG_TypeDef              *wdg;
  /**
   * @brief   WDOG instance index (0 = WDOG0, 1 = WDOG1).
   */
  uint32_t                  index;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if EFR32_WDG_USE_WDOG0 && !defined(__DOXYGEN__)
extern WDGDriver WDGD1;
#endif
#if EFR32_WDG_USE_WDOG1 && !defined(__DOXYGEN__)
extern WDGDriver WDGD2;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void wdg_lld_init(void);
  void wdg_lld_start(WDGDriver *wdgp);
  void wdg_lld_stop(WDGDriver *wdgp);
  void wdg_lld_reset(WDGDriver *wdgp);
  void wdg_lld_serve_interrupt(WDGDriver *wdgp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_WDG == TRUE */

#endif /* HAL_WDG_LLD_H */

/** @} */
