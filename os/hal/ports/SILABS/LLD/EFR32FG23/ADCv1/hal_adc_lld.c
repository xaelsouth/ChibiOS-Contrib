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
 * @file    ADCv1/hal_adc_lld.c
 * @brief   EFR32FG23 IADC subsystem low level driver source.
 *
 * @addtogroup ADC
 * @{
 */

#include "hal.h"

#if (HAL_USE_ADC == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if EFR32_ADC_USE_IADC0 || defined(__DOXYGEN__)
ADCDriver ADCD1;
#endif

/*===========================================================================*/
/* Driver interrupt handler.                                                 */
/*===========================================================================*/

#if EFR32_ADC_USE_IADC0 || defined(__DOXYGEN__)
/**
 * @brief   IADC interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(EFR32_IADC_HANDLER) {
  OSAL_IRQ_PROLOGUE();

  uint32_t isr = IADC0->IF;
  IADC0->IF_CLR = isr;

  if ((isr & IADC_IF_SCANFIFODVL) != 0U) {
    osalSysLockFromISR();
    adc_lld_isr_complete_code(&ADCD1, 0U);
    osalSysUnlockFromISR();
  }
  if ((isr & IADC_IF_SINGLEFIFODVL) != 0U) {
    osalSysLockFromISR();
    adc_lld_isr_complete_code(&ADCD1, 0U);
    osalSysUnlockFromISR();
  }
  if ((isr & IADC_IF_SCANTABLEDONE) != 0U) {
    osalSysLockFromISR();
    adc_lld_isr_complete_code(&ADCD1, 0U);
    osalSysUnlockFromISR();
  }

  OSAL_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void adc_lld_init(void) {
#if EFR32_ADC_USE_IADC0
  adcObjectInit(&ADCD1);
  ADCD1.iadc = IADC0;
  ADCD1.dma = NULL;
  ADCD1.errors = 0U;
#endif
}

msg_t adc_lld_start(ADCDriver *adcp) {
#if EFR32_ADC_USE_IADC0
  if (adcp->state == ADC_STOP) {
    /* Enable bus clock. */
    CMU->CLKEN0_SET = CMU_CLKEN0_IADC0;

    /* Select IADC clock source (EM01GRPACLK). */
    CMU->IADCCLKCTRL = (CMU->IADCCLKCTRL & ~_CMU_IADCCLKCTRL_CLKSEL_MASK)
                       | CMU_IADCCLKCTRL_CLKSEL_EM01GRPACLK;

    /* Enable IADC module. */
    IADC0->EN_SET = IADC_EN_EN;

    /* Configure: normal mode, default oversampling, VBGR reference. */
    IADC0->CFG = IADC_CFG_ADCMODE_NORMAL
               | IADC_CFG_REFSEL_VBGR
               | IADC_CFG_OSRHS_HISPD2;

    /* Clear flags. */
    IADC0->IF_CLR = _IADC_IF_MASK;
    IADC0->IEN = 0U;

    /* Allocate DMA stream for scan FIFO. */
#if defined(EFR32_DMA_REQUIRED)
    if (adcp->dma == NULL) {
      adcp->dma = dmaStreamAlloc(EFR32_ADC_IADC0_DMA_STREAM,
                                 EFR32_ADC_IADC0_DMA_PRIORITY, NULL, NULL);
    }
#endif

    nvicEnableVector(EFR32_IADC_NUMBER, EFR32_ADC_IADC0_IRQ_PRIORITY);
  }
#endif
  return HAL_RET_SUCCESS;
}

void adc_lld_stop(ADCDriver *adcp) {
#if EFR32_ADC_USE_IADC0
  if (adcp->state == ADC_READY) {
    IADC0->CMD = IADC_CMD_SINGLESTOP | IADC_CMD_SCANSTOP;
    IADC0->IEN = 0U;
    IADC0->IF_CLR = _IADC_IF_MASK;
    nvicDisableVector(EFR32_IADC_NUMBER);

    if (adcp->dma != NULL) {
      dmaStreamFree(adcp->dma);
      adcp->dma = NULL;
    }

    IADC0->EN_CLR = IADC_EN_EN;
    CMU->CLKEN0_CLR = CMU_CLKEN0_IADC0;
  }
#endif
}

void adc_lld_start_conversion(ADCDriver *adcp) {
#if EFR32_ADC_USE_IADC0
  const ADCSingleConversionGroup *grpp = &adcp->grp[0];

  /* Configure single conversion input from group. */
  IADC0->SINGLE = ((uint32_t)grpp->portneg << _IADC_SINGLE_PORTNEG_SHIFT)
                | ((uint32_t)grpp->portpos << _IADC_SINGLE_PORTPOS_SHIFT)
                | ((uint32_t)grpp->pin    << _IADC_SINGLE_PIN_SHIFT);

  /* Set reference. */
  IADC0->CFG = (IADC0->CFG & ~_IADC_CFG_REFSEL_MASK)
               | ((uint32_t)grpp->refsel << _IADC_CFG_REFSEL_SHIFT);

  /* Trigger single conversion. */
  IADC0->IF_CLR = IADC_IF_SINGLEFIFODVL;
  IADC0->IEN_SET = IADC_IF_SINGLEFIFODVL;
  IADC0->CMD = IADC_CMD_SINGLESTART;

  /* If DMA is used for scan, start scan conversion. */
  if (adcp->dma != NULL && adcp->grp[0].num_channels > 1U) {
    IADC0->IF_CLR = IADC_IF_SCANFIFODVL | IADC_IF_SCANTABLEDONE;
    IADC0->IEN_SET = IADC_IF_SCANFIFODVL | IADC_IF_SCANTABLEDONE;
    IADC0->CMD = IADC_CMD_SCANSTART;
  }
#else
  (void)adcp;
#endif
}

void adc_lld_stop_conversion(ADCDriver *adcp) {
#if EFR32_ADC_USE_IADC0
  IADC0->CMD = IADC_CMD_SINGLESTOP | IADC_CMD_SCANSTOP;
  IADC0->IEN_CLR = IADC_IF_SINGLEFIFODVL | IADC_IF_SCANFIFODVL | IADC_IF_SCANTABLEDONE;
  IADC0->IF_CLR = IADC_IF_SINGLEFIFODVL | IADC_IF_SCANFIFODVL | IADC_IF_SCANTABLEDONE;
#else
  (void)adcp;
#endif
}

#endif /* HAL_USE_ADC == TRUE */

/** @} */
