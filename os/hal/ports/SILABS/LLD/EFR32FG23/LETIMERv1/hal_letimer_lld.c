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
 * @file    LETIMERv1/hal_letimer_lld.c
 * @brief   EFR32FG23 LETIMER low-level driver (GPT-compatible).
 *
 * @details Implements the ChibiOS GPT driver interface on top of LETIMER0.
 *          LETIMER0 is a 24-bit down-counting timer. COMP0 is used as the
 *          period register (underflow event), COMP1 as a secondary alarm.
 *
 *          Clock: EM23GRPACLK (LFXO/LFRCO/ULFRCO). The prescaler is set
 *          from the GPTConfig.frequency field.
 *
 *          Conflict guard: if SYSTICKv3 is also using LETIMER0
 *          (EFR32_ST_USE_TIMER==1 with OSAL_ST_MODE==FREERUNNING),
 *          enabling both will cause a compile-time error.
 *
 * @addtogroup GPT
 * @{
 */

#include "hal.h"
#include "hal_letimer_lld.h"

#if (HAL_USE_GPT == TRUE) && (EFR32_LETIMER_GPT_USE_LETIMER0 == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

GPTDriver GPTD_LETIMER0;

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/* LETIMER0 clock in Hz (EM23GRPACLK = LFXO = 32768 Hz typically). */
static uint32_t letimer_clock_hz;
static gptcnt_t letimer_period;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/* Compute CNTPRESC value from desired frequency. */
static uint32_t letimer_presc_for_freq(uint32_t clk, uint32_t freq) {
  uint32_t div = clk / freq;
  if (div == 0U) div = 1U;
  /* Round to nearest power of 2, max DIV32768 (but FG23 LETIMER max is DIV32768? 
     Actually FG23 LETIMER CNTPRESC goes up to DIV32768 = 0xF). */
  uint32_t presc = 0U;
  uint32_t d = 1U;
  for (uint32_t i = 0U; i <= 15U; i++) {
    if (d >= div) break;
    d <<= 1U;
    presc++;
  }
  return presc;  /* 0=DIV1, 1=DIV2, ..., 15=DIV32768 */
}

/*===========================================================================*/
/* IRQ handler.                                                              */
/*===========================================================================*/

OSAL_IRQ_HANDLER(EFR32_LETIM1_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  gpt_lld_serve_interrupt(&GPTD_LETIMER0);  /* dispatches back to letimer_gpt_lld_serve_interrupt */
  OSAL_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void letimer_gpt_lld_init(void) {
  GPTD_LETIMER0.state = GPT_STOP;
  GPTD_LETIMER0.config = NULL;
  GPTD_LETIMER0.letimer = LETIMER0;
  GPTD_LETIMER0.timer = NULL;
  GPTD_LETIMER0.index = 0;

  /* Read the actual EM23GRPACLK frequency. */
  letimer_clock_hz = EFR32_EM23GRPACLK;
}

void letimer_gpt_lld_start(GPTDriver *gptp) {
  const GPTConfig *config = gptp->config;
  if (gptp->state == GPT_STOP) {
    gptp->letimer = LETIMER0;

    /* Enable clock. */
    CMU->CLKEN0_SET = CMU_CLKEN0_LETIMER0;

    LETIMER_TypeDef *lt = gptp->letimer;

    /* Enable clock and module first (LF-domain sync only runs while EN=1). */
    CMU->CLKEN0_SET = CMU_CLKEN0_LETIMER0;
    lt->EN_SET = LETIMER_EN_EN;
    while (lt->SYNCBUSY != 0U) {}

    lt->IF_CLR = _LETIMER_IF_MASK;
    lt->IEN = 0U;

    /* Set prescaler from frequency. */
    uint32_t presc = letimer_presc_for_freq(letimer_clock_hz, config->frequency);
    lt->CTRL = LETIMER_CTRL_REPMODE_FREE | (presc << _LETIMER_CTRL_CNTPRESC_SHIFT);
    lt->COMP0 = 0U;
    lt->TOP = 0xFFFFU;
    while (lt->SYNCBUSY != 0U) {}

    /* NOTE: CNT is never written here — on xG23 writing CNT while the
       module runs in the LF domain bus-faults; free-running mode does not
       need it. */
    lt->CMD = LETIMER_CMD_START;
    while (lt->SYNCBUSY & (LETIMER_SYNCBUSY_START | LETIMER_SYNCBUSY_STOP)) {}

    nvicEnableVector(EFR32_LETIM1_NUMBER, EFR32_LETIMER_IRQ_PRIORITY);
  }
  gptp->state = GPT_READY;
}

void letimer_gpt_lld_stop(GPTDriver *gptp) {
  if (gptp->state == GPT_READY || gptp->state == GPT_CONTINUOUS ||
      gptp->state == GPT_ONESHOT) {
    LETIMER_TypeDef *lt = gptp->letimer;
    nvicDisableVector(EFR32_LETIM1_NUMBER);
    lt->IEN = 0U;
    lt->IF_CLR = _LETIMER_IF_MASK;
    lt->CMD = LETIMER_CMD_STOP;
    lt->EN_CLR = LETIMER_EN_EN;
    CMU->CLKEN0_CLR = CMU_CLKEN0_LETIMER0;
    gptp->state = GPT_STOP;
    gptp->config = NULL;
  }
}

void letimer_gpt_lld_start_timer(GPTDriver *gptp, gptcnt_t period) {
  LETIMER_TypeDef *lt = gptp->letimer;

  /* Continuous mode via COMP0 compare: LETIMER counts down free-running,
     COMP0 fires when CNT reaches COMP0. Arm COMP0 = CNT - period, the ISR
     re-arms COMP0 -= period. CNT is never written (bus-faults on xG23
     while the module runs). */
  lt->IF_CLR = _LETIMER_IF_MASK;
  while (lt->SYNCBUSY != 0U) {}
  letimer_period = period;
  lt->COMP0 = lt->CNT - (uint32_t)period;
  while (lt->SYNCBUSY != 0U) {}
  lt->IEN_SET = LETIMER_IEN_COMP0;

  gptp->state = GPT_CONTINUOUS;
}

void letimer_gpt_lld_stop_timer(GPTDriver *gptp) {
  LETIMER_TypeDef *lt = gptp->letimer;
  lt->IEN_CLR = LETIMER_IEN_COMP0;
  gptp->state = GPT_READY;
}

void letimer_gpt_lld_polled_delay(GPTDriver *gptp, gptcnt_t interval) {
  LETIMER_TypeDef *lt = gptp->letimer;

  /* Polled delay: arm COMP0 = CNT - interval, wait for the COMP0 flag. */
  lt->IF_CLR = LETIMER_IF_COMP0;
  while (lt->SYNCBUSY != 0U) {}
  lt->COMP0 = lt->CNT - (uint32_t)interval;
  while (lt->SYNCBUSY != 0U) {}
  while ((lt->IF & LETIMER_IF_COMP0) == 0U) {
    /* busy wait */
  }
  lt->IF_CLR = LETIMER_IF_COMP0;
}

void letimer_gpt_lld_serve_interrupt(GPTDriver *gptp) {
  LETIMER_TypeDef *lt = gptp->letimer;
  uint32_t isr = lt->IF;

  if ((isr & LETIMER_IF_COMP0) != 0U) {
    lt->IF_CLR = LETIMER_IF_COMP0;
    /* Re-arm one period ahead (LF-domain writes take a couple of ticks). */
    if (gptp->state == GPT_CONTINUOUS) {
      lt->COMP0 = lt->CNT - (uint32_t)letimer_period;
    }
    _gpt_isr_invoke_cb(gptp);
  }
  /* COMP1 = secondary alarm (unused in basic GPT mode). */
  if ((isr & LETIMER_IF_COMP1) != 0U) {
    lt->IF_CLR = LETIMER_IF_COMP1;
  }
}

#endif /* HAL_USE_GPT && EFR32_LETIMER_GPT_USE_LETIMER0 */

/** @} */
