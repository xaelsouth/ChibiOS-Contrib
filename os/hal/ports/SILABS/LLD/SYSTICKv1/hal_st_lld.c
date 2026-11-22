/*
    ChibiOS - Copyright (C) 2023 Xael South

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
 * @file    SYSTICKv1/hal_st_lld.c
 * @brief   ARM Cortex-M core SysTick based ST low level driver source.
 *
 * @details Optional driver, selected with EFR32_USE_SYSTICKV1=yes.
 *          See hal_st_lld.h for the mode description.
 *
 * @addtogroup ST
 * @{
 */

#include "hal.h"

#if (OSAL_ST_MODE != OSAL_ST_MODE_NONE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define ST_HANDLER                          SysTick_Handler

/* SysTick is clocked from the core clock (HCLK). */
#define SYSTICK_CK                          EFR32_HCLK

#if (SYSTICK_CK / OSAL_ST_FREQUENCY) - 1 > EFR32_SYSTICK_CK_MAX
#error "the selected ST frequency is not obtainable because of timer counter limits"
#endif

#if SYSTICK_CK % OSAL_ST_FREQUENCY != 0
#error "the selected ST frequency is not obtainable because of integer rounding"
#endif

/* Reload value for one system tick. */
#define SYSTICK_RELOAD                      ((SYSTICK_CK / OSAL_ST_FREQUENCY) - 1U)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if (OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING) || defined(__DOXYGEN__)
volatile uint32_t st_lld_sw_counter;
volatile uint32_t st_lld_sw_alarm;
volatile bool     st_lld_sw_alarm_active;
#endif

/*===========================================================================*/
/* Driver local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   SysTick interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(ST_HANDLER) {

  OSAL_IRQ_PROLOGUE();

#if OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING
  /* One system tick elapsed: advance the software free running counter. */
  st_lld_sw_counter++;

  /* SysTick has no compare register: the alarm is checked here with
     1 tick precision. Signed difference handles the 32 bit wrap. */
  if (st_lld_sw_alarm_active &&
      ((int32_t)(st_lld_sw_counter - st_lld_sw_alarm) >= 0)) {
    st_lld_sw_alarm_active = false;
    st_lld_serve_interrupt();
  }
#else
  /* Periodic mode: every tick is a system tick. */
  st_lld_serve_interrupt();
#endif

  OSAL_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level ST driver initialization.
 *
 * @notapi
 */
void st_lld_init(void) {

#if OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING
  st_lld_sw_counter      = 0U;
  st_lld_sw_alarm        = 0U;
  st_lld_sw_alarm_active = false;
#endif

  /* SysTick fires once per system tick in both modes. */
  SysTick->LOAD = SYSTICK_RELOAD;
  SysTick->VAL  = 0U;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                  SysTick_CTRL_ENABLE_Msk |
                  SysTick_CTRL_TICKINT_Msk;

  /* IRQ enabled.*/
  nvicSetSystemHandlerPriority(HANDLER_SYSTICK, EFR32_ST_IRQ_PRIORITY);
}

/**
 * @brief   IRQ handling code.
 */
void st_lld_serve_interrupt(void) {

  osalSysLockFromISR();
  osalOsTimerHandlerI();
  osalSysUnlockFromISR();
}

#endif /* OSAL_ST_MODE != OSAL_ST_MODE_NONE */

/** @} */
