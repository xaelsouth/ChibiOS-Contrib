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
 * @file    hal_lld.h
 * @brief   PLATFORM HAL subsystem low level driver header.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef HAL_LLD_H
#define HAL_LLD_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

#include "efr32_registry.h"

/**
 * @name    Platform identification macros
 * @{
 */
#define PLATFORM_NAME           "EFR32FG14P"
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*
 * Configuration-related checks.
 */
#if defined(EFR32FG14P23X) && !defined(EFR32FG14P23X_MCUCONF)
#error "Using a wrong mcuconf.h file, EFR32FG14P23X_MCUCONF not defined"
#endif

#define EFR32_HCLK                      EFR32_HFRCO_STARTUP_FREQ
#define EFR32_LFXOCLK                   EFR32_LFXO_FREQ
#define EFR32_LFRCOCLK                  EFR32_LFRCO_FREQ
#define EFR32_ULFRCOCLK                 EFR32_ULFRCO_FREQ


#define EFR32_RTCCSEL_NOCLOCK           (0u << 0)
#define EFR32_RTCCSEL_LFXO              (1u << 0)
#define EFR32_RTCCSEL_LFRCO             (1u << 1)
#define EFR32_RTCCSEL_ULFRCO            (1u << 2)

#define EFR32_RTCC_IRQ_HANDLER          VectorBC
#define EFR32_RTCC_IRQ_NUMBER           RTCC_IRQn
#define EFR32_RTCCCLK                   EFR32_LFXOCLK


#define EFR32_LETIM1SEL_NOCLOCK         (0u << 0)
#define EFR32_LETIM1SEL_LFXO            (1u << 0)
#define EFR32_LETIM1SEL_LFRCO           (1u << 1)
#define EFR32_LETIM1SEL_ULFRCO          (1u << 2)

#define EFR32_LETIM1_HANDLER            VectorAC
#define EFR32_LETIM1_NUMBER             LETIMER0_IRQn
#define EFR32_LETIM1CLK                 EFR32_LFXOCLK

/*===========================================================================*/
/* Clock management.                                                         */
/*===========================================================================*/

/**
 * @name    Clock points identifiers for halClockGetPointX()
 * @{
 */
#define EFR32_CLK_HCLK            0U    /**< HCLK (HFRCO startup frequency).   */
#define EFR32_CLK_RTCCCLK         1U    /**< RTCC clock.                       */
#define EFR32_CLK_LETIM1CLK       2U    /**< LETIMER0 clock.                   */
#define EFR32_CLK_LFXO            3U    /**< 32.768 kHz crystal oscillator.    */
#define EFR32_CLK_LFRCO           4U    /**< 32.768 kHz RC oscillator.         */
#define EFR32_CLK_ULFRCO          5U    /**< 1 kHz ultra-low-power RC osc.     */
#define EFR32_CLK_ARRAY_SIZE      6U    /**< Number of clock points.           */
/** @} */

/**
 * @brief   Names of the clock points, for shell commands.
 */
#define EFR32_CLK_POINT_NAMES                                                 \
  {                                                                           \
    "HCLK", "RTCCCLK", "LETIM1CLK", "LFXO", "LFRCO", "ULFRCO"                 \
  }

/* Generic aliases, same names as used by the STM32 ports. */
#define CLK_ARRAY_SIZE            EFR32_CLK_ARRAY_SIZE
#define CLK_POINT_NAMES           EFR32_CLK_POINT_NAMES

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) || defined(__DOXYGEN__)
/**
 * @brief   Type of a clock configuration structure.
 */
typedef struct {
  uint32_t          dummy;
} halclkcfg_t;
#endif /* defined(HAL_LLD_USE_CLOCK_MANAGEMENT) */

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) && !defined(__DOXYGEN__)
extern const halclkcfg_t hal_clkcfg_reset;
extern const halclkcfg_t hal_clkcfg_default;
#endif

/* Various helpers.*/
#include "nvic.h"

#ifdef __cplusplus
extern "C" {
#endif
  void efr32_chip_init(void);
  void efr32_clock_init(void);
  void hal_lld_init(void);
  /* Always available: hal.h calls it unconditionally from
     halClockGetPointX(). */
  halfreq_t hal_lld_get_clock_point(halclkpt_t clkpt);
#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) || defined(__DOXYGEN__)
  bool hal_lld_clock_switch_mode(const halclkcfg_t *ccp);
#endif /* defined(HAL_LLD_USE_CLOCK_MANAGEMENT) */
#ifdef __cplusplus
}
#endif

#endif /* HAL_LLD_H */

/** @} */
