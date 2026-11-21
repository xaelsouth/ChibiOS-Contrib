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
 * @file    TIMERv1/hal_gpt_lld.c
 * @brief   EFR32FG23 GPT subsystem low level driver source.
 *
 * @addtogroup GPT
 * @{
 */

#include "hal.h"

#if (HAL_USE_GPT == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

static TIMER_TypeDef * const efr32_gpt_timers[5] = {
  TIMER0, TIMER1, TIMER2, TIMER3, TIMER4
};

static const uint32_t efr32_gpt_clken[5] = {
  CMU_CLKEN0_TIMER0, CMU_CLKEN0_TIMER1, CMU_CLKEN0_TIMER2,
  CMU_CLKEN0_TIMER3, CMU_CLKEN0_TIMER4
};

static const uint32_t efr32_gpt_irqn[5] = {
  TIMER0_IRQn, TIMER1_IRQn, TIMER2_IRQn, TIMER3_IRQn, TIMER4_IRQn
};

static const uint32_t efr32_gpt_irqprio[5] = {
  EFR32_GPT_TIMER0_IRQ_PRIORITY, EFR32_GPT_TIMER1_IRQ_PRIORITY,
  EFR32_GPT_TIMER2_IRQ_PRIORITY, EFR32_GPT_TIMER3_IRQ_PRIORITY,
  EFR32_GPT_TIMER4_IRQ_PRIORITY
};

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if EFR32_GPT_USE_TIMER0 || defined(__DOXYGEN__)
GPTDriver GPTD1;
#endif
#if EFR32_GPT_USE_TIMER1 || defined(__DOXYGEN__)
GPTDriver GPTD2;
#endif
#if EFR32_GPT_USE_TIMER2 || defined(__DOXYGEN__)
GPTDriver GPTD3;
#endif
#if EFR32_GPT_USE_TIMER3 || defined(__DOXYGEN__)
GPTDriver GPTD4;
#endif
#if EFR32_GPT_USE_TIMER4 || defined(__DOXYGEN__)
GPTDriver GPTD5;
#endif

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if EFR32_GPT_USE_TIMER0 || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(TIMER0_IRQHandler) {
  OSAL_IRQ_PROLOGUE();
  gpt_lld_serve_interrupt(&GPTD1);
  OSAL_IRQ_EPILOGUE();
}
#endif
#if EFR32_GPT_USE_TIMER1 || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(TIMER1_IRQHandler) {
  OSAL_IRQ_PROLOGUE();
  gpt_lld_serve_interrupt(&GPTD2);
  OSAL_IRQ_EPILOGUE();
}
#endif
#if EFR32_GPT_USE_TIMER2 || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(TIMER2_IRQHandler) {
  OSAL_IRQ_PROLOGUE();
  gpt_lld_serve_interrupt(&GPTD3);
  OSAL_IRQ_EPILOGUE();
}
#endif
#if EFR32_GPT_USE_TIMER3 || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(TIMER3_IRQHandler) {
  OSAL_IRQ_PROLOGUE();
  gpt_lld_serve_interrupt(&GPTD4);
  OSAL_IRQ_EPILOGUE();
}
#endif
#if EFR32_GPT_USE_TIMER4 || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(TIMER4_IRQHandler) {
  OSAL_IRQ_PROLOGUE();
  gpt_lld_serve_interrupt(&GPTD5);
  OSAL_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void gpt_lld_init(void) {
#if EFR32_GPT_USE_TIMER0
  gptObjectInit(&GPTD1);
  GPTD1.timer = TIMER0; GPTD1.index = 0;
#endif
#if EFR32_GPT_USE_TIMER1
  gptObjectInit(&GPTD2);
  GPTD2.timer = TIMER1; GPTD2.index = 1;
#endif
#if EFR32_GPT_USE_TIMER2
  gptObjectInit(&GPTD3);
  GPTD3.timer = TIMER2; GPTD3.index = 2;
#endif
#if EFR32_GPT_USE_TIMER3
  gptObjectInit(&GPTD4);
  GPTD4.timer = TIMER3; GPTD4.index = 3;
#endif
#if EFR32_GPT_USE_TIMER4
  gptObjectInit(&GPTD5);
  GPTD5.timer = TIMER4; GPTD5.index = 4;
#endif
}

void gpt_lld_start(GPTDriver *gptp, const GPTConfig *config) {
  TIMER_TypeDef *tim = gptp->timer;

  if (gptp->state == GPT_STOP) {
    CMU->CLKEN0_SET = efr32_gpt_clken[gptp->index];
    tim->EN_SET = TIMER_EN_EN;
    /* Prescaler DIV1, clock from EM01GRPACLK, up-count mode. */
    tim->CFG = _TIMER_CFG_CLKSEL_PRESCEM01GRPACLK;
    tim->IF_CLR = _TIMER_IF_MASK;
    tim->IEN = 0;
    nvicEnableVector(efr32_gpt_irqn[gptp->index], efr32_gpt_irqprio[gptp->index]);
  }

  gptp->callback = config->callback;
  gptp->callback_param = config->callback_param;
}

void gpt_lld_stop(GPTDriver *gptp) {
  TIMER_TypeDef *tim = gptp->timer;

  if (gptp->state == GPT_READY) {
    tim->CMD_SET = TIMER_CMD_STOP;
    tim->IEN = 0;
    tim->IF_CLR = _TIMER_IF_MASK;
    nvicDisableVector(efr32_gpt_irqn[gptp->index]);
    tim->EN_CLR = TIMER_EN_EN;
    CMU->CLKEN0_CLR = efr32_gpt_clken[gptp->index];
  }
}

void gpt_lld_start_timer(GPTDriver *gptp, gptcnt_t period) {
  TIMER_TypeDef *tim = gptp->timer;

  tim->TOP = (uint32_t)period - 1U;
  tim->CNT = 0;
  tim->IF_CLR = _TIMER_IF_MASK;
  tim->IEN_SET = TIMER_IEN_OF;
  tim->CMD_SET = TIMER_CMD_START;
}

void gpt_lld_stop_timer(GPTDriver *gptp) {
  TIMER_TypeDef *tim = gptp->timer;

  tim->CMD_SET = TIMER_CMD_STOP;
  tim->IEN_CLR = TIMER_IEN_OF;
  tim->IF_CLR = TIMER_IF_OF;
}

void gpt_lld_polled_delay(GPTDriver *gptp, gptcnt_t interval) {
  TIMER_TypeDef *tim = gptp->timer;

  tim->TOP = (uint32_t)interval - 1U;
  tim->CNT = 0;
  tim->IF_CLR = TIMER_IF_OF;
  tim->CMD_SET = TIMER_CMD_START;
  while ((tim->IF & TIMER_IF_OF) == 0U) {
  }
  tim->CMD_SET = TIMER_CMD_STOP;
  tim->IF_CLR = TIMER_IF_OF;
}

void gpt_lld_serve_interrupt(GPTDriver *gptp) {
  TIMER_TypeDef *tim = gptp->timer;

  if ((tim->IF & TIMER_IF_OF) != 0U) {
    tim->IF_CLR = TIMER_IF_OF;
    if (gptp->callback != NULL) {
      gptp->callback(gptp, gptp->callback_param);
    }
  }
}

#endif /* HAL_USE_GPT == TRUE */

/** @} */
