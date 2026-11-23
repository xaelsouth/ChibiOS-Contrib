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
 * @file    hal_lld.c
 * @brief   PLATFORM HAL subsystem low level driver source.
 *
 * @addtogroup HAL
 * @{
 */

#include "hal.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) && !defined(__DOXYGEN__)
/**
 * @brief   Post-reset configuration, must be implemented.
 */
const halclkcfg_t hal_clkcfg_reset = {
  .dummy = 0U
};

/**
 * @brief   Configuration from mcuconf.h, must be implemented.
 */
const halclkcfg_t hal_clkcfg_default = {
  .dummy = 0U
};
#endif

/*===========================================================================*/
/* Clock points table.                                                       */
/*===========================================================================*/

/**
 * @brief   Run-time frequencies of the clock points, in Hz.
 */
static halfreq_t efr32_clk_point_freq[EFR32_CLK_ARRAY_SIZE];

/**
 * @brief   Initializes the clock points table from the compile-time
 *          clock configuration.
 */
static void efr32_clock_points_init(void) {

  efr32_clk_point_freq[EFR32_CLK_HCLK]      = EFR32_HCLK;
  efr32_clk_point_freq[EFR32_CLK_RTCCCLK]   = EFR32_RTCCCLK;
  efr32_clk_point_freq[EFR32_CLK_LETIM1CLK] = EFR32_LETIM1CLK;
  efr32_clk_point_freq[EFR32_CLK_LFXO]      = EFR32_LFXOCLK;
  efr32_clk_point_freq[EFR32_CLK_LFRCO]     = EFR32_LFRCOCLK;
  efr32_clk_point_freq[EFR32_CLK_ULFRCO]    = EFR32_ULFRCOCLK;
}

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   EFR32FG14P chip and errata initialization.
 * @note    All the involved constants come from the file @p board.h.
 * @note    This function should be invoked just after the system reset.
 *
 * @special
 */

void efr32_chip_init(void) {

  /* CHIP_Init() is not provided by the current Gecko SDK (Series 1 chip
     revision fixups are no longer needed); nothing to do here. */
}

/**
 * @brief   EFR32FG14P clocks initialization.
 * @note    All the involved constants come from the file @p board.h.
 * @note    This function should be invoked just after the system reset.
 *
 * @special
 */
void efr32_clock_init(void) {
  SystemCoreClock = SystemCoreClockGet();
    
#if EFR32_LFXO_ENABLED
  CMU->OSCENCMD = CMU_OSCENCMD_LFXOEN;
  while (!(CMU->STATUS & CMU_STATUS_LFXORDY)) {}
#else
  CMU->OSCENCMD = CMU_OSCENCMD_LFXODIS;
  while (CMU->STATUS & CMU_STATUS_LFXORDY) {}
#endif

#if EFR32_HFXO_ENABLED
  CMU->OSCENCMD = CMU_OSCENCMD_HFXOEN;
  while (!(CMU->STATUS & CMU_STATUS_HFXORDY)) {}
#else
  CMU->OSCENCMD = CMU_OSCENCMD_HFXODIS;
  while (CMU->STATUS & CMU_STATUS_HFXORDY) {}
#endif

#if EFR32_LFRCO_ENABLED
  CMU->OSCENCMD = CMU_OSCENCMD_LFRCOEN;
  while (!(CMU->STATUS & CMU_STATUS_LFRCORDY)) {}
#else
  CMU->OSCENCMD = CMU_OSCENCMD_LFRCODIS;
  while (CMU->STATUS & CMU_STATUS_LFRCORDY) {}
#endif

#if EFR32_HFRCO_ENABLED
  CMU->OSCENCMD = CMU_OSCENCMD_HFRCOEN;
  while (!(CMU->STATUS & CMU_STATUS_HFRCORDY)) {}
#else
  CMU->OSCENCMD = CMU_OSCENCMD_HFRCODIS;
  while (CMU->STATUS & CMU_STATUS_HFRCORDY) {}
#endif

#if defined(EFR32_RTCCSEL)
#if EFR32_RTCCSEL == EFR32_RTCCSEL_NOCLOCK
  CMU->LFECLKSEL = CMU_LFECLKSEL_LFE_DISABLED;
  
#elif EFR32_RTCCSEL == EFR32_RTCCSEL_LFXO
  CMU->LFECLKSEL = CMU_LFECLKSEL_LFE_LFXO;

#elif EFR32_RTCCSEL == EFR32_RTCCSEL_LFRCO
  CMU->LFECLKSEL = CMU_LFECLKSEL_LFE_LFRCO;

#elif EFR32_RTCCSEL == EFR32_RTCCSEL_ULFRCO
  CMU->LFECLKSEL = CMU_LFECLKSEL_LFE_ULFRCO;

#else
#error "EFR32_RTCCSEL not specified"
#endif
#endif

#if defined(EFR32_LETIM1SEL)
#if EFR32_LETIM1SEL == EFR32_LETIM1SEL_NOCLOCK
  CMU->LFACLKSEL = CMU_LFACLKSEL_LFA_DISABLED;
  
#elif EFR32_LETIM1SEL == EFR32_LETIM1SEL_LFXO
  CMU->LFACLKSEL = CMU_LFACLKSEL_LFA_LFXO;

#elif EFR32_LETIM1SEL == EFR32_LETIM1SEL_LFRCO
  CMU->LFACLKSEL = CMU_LFACLKSEL_LFA_LFRCO;

#elif EFR32_LETIM1SEL == EFR32_LETIM1SEL_ULFRCO
  CMU->LFACLKSEL = CMU_LFACLKSEL_LFA_ULFRCO;

#else
#error "EFR32_LETIM1SEL not specified"
#endif
#endif
}


/**
 * @brief   Low level HAL driver initialization.
 *
 * @notapi
 */
void hal_lld_init(void) {

  /* Clock points table initialization from the compile-time config. */
  efr32_clock_points_init();

  /* NVIC initialization.*/
  nvicInit();
}

/**
 * @brief   Returns the frequency of a clock point in Hz.
 *
 * @param[in] clkpt     clock point to be returned
 * @return              The clock point frequency in Hz or zero if the
 *                      frequency is unknown.
 *
 * @notapi
 */
halfreq_t hal_lld_get_clock_point(halclkpt_t clkpt) {

  if (clkpt >= (halclkpt_t)EFR32_CLK_ARRAY_SIZE) {
    return 0U;
  }

  return efr32_clk_point_freq[clkpt];
}

#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) || defined(__DOXYGEN__)
/**
 * @brief   Switches to a different clock configuration
 * @note    Not implemented on EFR32FG14P: the clock configuration is fixed
 *          at compile time via mcuconf.h (emlib CMU calls in
 *          efr32_clock_init()). Always fails.
 *
 * @param[in] ccp       pointer to clock a @p halclkcfg_t structure
 * @return              The clock switch result.
 * @retval false        if the clock switch succeeded
 * @retval true         if the clock switch failed
 *
 * @notapi
 */
bool hal_lld_clock_switch_mode(const halclkcfg_t *ccp) {

  (void)ccp;

  return true;
}
#endif /* defined(HAL_LLD_USE_CLOCK_MANAGEMENT) */

/**
 * @brief   efr32_escape_hatch()
 * @details When developing or debugging code that enters EM2 or lower, it's
 *          a good idea to have an "escape hatch" mechanism, e.g. a way to
 *          pause the device so that a debugger can connect in order to
 *          erase flash, among other things. If the ESCAPE_HATCH_PIN pin is
 *          low, execute the breakpoint instruction to stop the processor.
 */
void efr32_escape_hatch(void) {

#if defined(ESCAPE_HATCH_ENABLE) && (ESCAPE_HATCH_ENABLE == TRUE)
  const uint32_t port = ESCAPE_HATCH_PORT;
  const uint32_t pin  = ESCAPE_HATCH_PIN;
  const bool gpio_clk_was_enabled =
      (CMU->HFBUSCLKEN0 & CMU_HFBUSCLKEN0_GPIO) != 0U;
  unsigned int pin_state;

  CMU->HFBUSCLKEN0 |= CMU_HFBUSCLKEN0_GPIO;

  GPIO->P[port].DOUT = 1UL << pin;
  if (pin < 8) {
    GPIO->P[port].MODEL = (GPIO->P[port].MODEL & ~(0xFu << (pin * 4))) |
                          (GPIO_P_MODEL_MODE0_INPUTPULLFILTER << (pin * 4));
  }
  else {
    GPIO->P[port].MODEH = (GPIO->P[port].MODEH & ~(0xFu << ((pin - 8) * 4))) |
                          (GPIO_P_MODEH_MODE8_INPUTPULLFILTER << ((pin - 8) * 4));
  }

  pin_state = (GPIO->P[port].DIN >> pin) & 1;

  if (pin_state == 0) { /* Pin asserted, so break into debugger. */
    __BKPT(0);
  }
  else { /* Pin not asserted, so disable input. */
    if (pin < 8) {
      GPIO->P[port].MODEL = (GPIO->P[port].MODEL & ~(0xFu << (pin * 4))) |
                            (GPIO_P_MODEL_MODE0_DISABLED << (pin * 4));
    }
    else {
      GPIO->P[port].MODEH = (GPIO->P[port].MODEH & ~(0xFu << ((pin - 8) * 4))) |
                            (GPIO_P_MODEH_MODE8_DISABLED << ((pin - 8) * 4));
    }

    if (!gpio_clk_was_enabled) {
      CMU->HFBUSCLKEN0 &= ~CMU_HFBUSCLKEN0_GPIO;
    }
  }
#endif
}

/** @} */
