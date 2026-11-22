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
 * @file    WDOGv1/hal_wdg_lld.c
 * @brief   WDG Driver subsystem low level driver source.
 *
 * @addtogroup WDG
 * @{
 */

#include "hal.h"

#if (HAL_USE_WDG == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* WDOG lock/unlock keys. */
#define EFR32_WDOG_LOCKKEY_UNLOCK       0x0000ABE8U
#define EFR32_WDOG_LOCKKEY_LOCK         0x00000000U

/* CMU clock enable bits and clock-control register offsets per instance. */
static const uint32_t efr32_wdog_clken[2] = {
  CMU_CLKEN0_WDOG0, CMU_CLKEN1_WDOG1
};

static WDOG_TypeDef * const efr32_wdog_periph[2] = { WDOG0, WDOG1 };

/* Pointers to CMU WDOGxCLKCTRL registers (not contiguous, index explicitly). */
static volatile uint32_t * const efr32_wdog_clkctrl[2] = {
  &CMU->WDOG0CLKCTRL, &CMU->WDOG1CLKCTRL
};

static const uint32_t efr32_wdog_irqn[2] = {
  EFR32_WDOG1_NUMBER, EFR32_WDOG2_NUMBER
};

static const uint32_t efr32_wdog_irqprio[2] = {
  EFR32_WDG_WDOG0_IRQ_PRIORITY, EFR32_WDG_WDOG1_IRQ_PRIORITY
};

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if EFR32_WDG_USE_WDOG0 || defined(__DOXYGEN__)
WDGDriver WDGD1;
#endif
#if EFR32_WDG_USE_WDOG1 || defined(__DOXYGEN__)
WDGDriver WDGD2;
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static WDGDriver *wdg_lld_get_driver(uint32_t index) {
  switch (index) {
#if EFR32_WDG_USE_WDOG0
  case 0: return &WDGD1;
#endif
#if EFR32_WDG_USE_WDOG1
  case 1: return &WDGD2;
#endif
  default: return NULL;
  }
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if EFR32_WDG_USE_WDOG0 || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_WDOG1_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  wdg_lld_serve_interrupt(&WDGD1);
  OSAL_IRQ_EPILOGUE();
}
#endif

#if EFR32_WDG_USE_WDOG1 || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_WDOG2_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  wdg_lld_serve_interrupt(&WDGD2);
  OSAL_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level WDG driver initialization.
 *
 * @notapi
 */
void wdg_lld_init(void) {
#if EFR32_WDG_USE_WDOG0
  WDGD1.state = WDG_STOP;
  WDGD1.wdg   = WDOG0;
  WDGD1.index = 0;
#endif
#if EFR32_WDG_USE_WDOG1
  WDGD2.state = WDG_STOP;
  WDGD2.wdg   = WDOG1;
  WDGD2.index = 1;
#endif
}

/**
 * @brief   Configures and activates the WDG peripheral.
 *
 * @param[in] wdgp      pointer to the @p WDGDriver object
 *
 * @notapi
 */
void wdg_lld_start(WDGDriver *wdgp) {
  WDOG_TypeDef *wdg = wdgp->wdg;
  uint32_t idx = wdgp->index;
  uint32_t cfg = 0U;

  /* Enable bus clock for this WDOG instance. */
  if (idx == 0U) {
    CMU->CLKEN0_SET = efr32_wdog_clken[idx];
  } else {
    CMU->CLKEN1_SET = efr32_wdog_clken[idx];
  }

  /* Select the WDOG clock source. */
  *efr32_wdog_clkctrl[idx] = wdgp->config->clksel;

  /* Build the CFG register value. */
  cfg |= wdgp->config->persel;
  cfg |= wdgp->config->warnsel;
  cfg |= WDOG_CFG_CLRSRC_SW;                          /* SW clear only */
  if (wdgp->config->em1run)   cfg |= WDOG_CFG_EM1RUN;
  if (wdgp->config->em2run)   cfg |= WDOG_CFG_EM2RUN;
  if (wdgp->config->em3run)   cfg |= WDOG_CFG_EM3RUN;
  if (wdgp->config->debugrun) cfg |= WDOG_CFG_DEBUGRUN;

  /* Unlock, write config, then enable. */
  wdg->LOCK = EFR32_WDOG_LOCKKEY_UNLOCK;
  wdg->CFG  = cfg;

  /* Clear any pending interrupt flags. */
  wdg->IF_CLR = wdg->IF;

  /* Enable warning interrupt if configured. */
  if (wdgp->config->warnsel != EFR32_WDOG_WARNSEL_DIS) {
    wdg->IEN_SET = WDOG_IEN_WARN;
    nvicEnableVector(efr32_wdog_irqn[idx], efr32_wdog_irqprio[idx]);
  }

  /* Enable the watchdog. */
  wdg->EN_SET = WDOG_EN_EN;

  /* Wait for synchronization. */
  while ((wdg->SYNCBUSY & _WDOG_SYNCBUSY_MASK) != 0U) {}

  /* Start the counter by clearing it. */
  wdg->CMD = WDOG_CMD_CLEAR;

  /* Optionally lock the configuration. */
  if (wdgp->config->lock) {
    wdg->LOCK = EFR32_WDOG_LOCKKEY_LOCK;
  }
}

/**
 * @brief   Deactivates the WDG peripheral.
 *
 * @param[in] wdgp      pointer to the @p WDGDriver object
 *
 * @notapi
 */
void wdg_lld_stop(WDGDriver *wdgp) {
  WDOG_TypeDef *wdg = wdgp->wdg;
  uint32_t idx = wdgp->index;

  /* Cannot stop if locked. */
  osalDbgAssert((wdg->STATUS & WDOG_STATUS_LOCK) == 0U,
                "WDOG is locked and cannot be stopped");

  /* Disable interrupts. */
  nvicDisableVector(efr32_wdog_irqn[idx]);
  wdg->IEN_CLR = _WDOG_IEN_MASK;

  /* Disable the watchdog. */
  wdg->EN_CLR = WDOG_EN_EN;
  while ((wdg->EN & WDOG_EN_DISABLING) == WDOG_EN_DISABLING) {}

  /* Disable bus clock. */
  if (idx == 0U) {
    CMU->CLKEN0_CLR = efr32_wdog_clken[idx];
  } else {
    CMU->CLKEN1_CLR = efr32_wdog_clken[idx];
  }
}

/**
 * @brief   Reloads WDG's counter.
 *
 * @param[in] wdgp      pointer to the @p WDGDriver object
 *
 * @notapi
 */
void wdg_lld_reset(WDGDriver *wdgp) {
  /* Clear (kick) the watchdog counter. */
  WDOG_TypeDef *wdg = wdgp->wdg;
  while ((wdg->SYNCBUSY & _WDOG_SYNCBUSY_CMD_MASK) != 0U) {}
  wdg->CMD = WDOG_CMD_CLEAR;
}

/**
 * @brief   WDOG interrupt handler body.
 * @details Handles the warning interrupt: calls the user callback if set.
 *
 * @param[in] wdgp      pointer to the @p WDGDriver object
 *
 * @isr
 */
void wdg_lld_serve_interrupt(WDGDriver *wdgp) {
  WDOG_TypeDef *wdg = wdgp->wdg;
  uint32_t isr = wdg->IF;

  /* Clear all flags. */
  wdg->IF_CLR = isr;

  if ((isr & WDOG_IF_WARN) != 0U) {
    if (wdgp->config != NULL && wdgp->config->warn_cb != NULL) {
      wdgp->config->warn_cb(wdgp);
    }
  }
}

#endif /* HAL_USE_WDG == TRUE */

/** @} */
