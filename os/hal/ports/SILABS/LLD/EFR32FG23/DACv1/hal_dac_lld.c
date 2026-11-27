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
 * @file    DACv1/hal_dac_lld.c
 * @brief   EFR32FG23 VDAC subsystem low level driver source.
 *
 * @addtogroup DAC
 * @{
 */

#include "hal.h"

#if (HAL_USE_DAC == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define EFR32_DAC1_CLK_EN_MASK      0x03U
#define EFR32_DAC1_CLK_EN_SHIFT(ch) (1U << (ch))

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if (EFR32_DAC_USE_DAC1_CH1 == TRUE) || defined(__DOXYGEN__)
DACDriver DACD1;
#endif

#if (EFR32_DAC_USE_DAC1_CH2 == TRUE) || defined(__DOXYGEN__)
#if !EFR32_DAC_DUAL_MODE
DACDriver DACD2;
#else
#error "DAC1 CH2 not independently available in dual mode"
#endif
#endif

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

static struct {
  uint32_t          clk_en_mask;
  const efr32_dma_stream_t *dma_ch1;
  const efr32_dma_stream_t *dma_ch2;
} dac;

static const efr32_dma_stream_t *dac_dma_stream(DACDriver *dacp) {
#if EFR32_DAC_USE_DAC1_CH1 == TRUE
  if (dacp == &DACD1) {
    return dac.dma_ch1;
  }
#endif
#if EFR32_DAC_USE_DAC1_CH2 == TRUE
  if (dacp == &DACD2) {
    return dac.dma_ch2;
  }
#endif
  return NULL;
}

/*===========================================================================*/
/* Driver interrupt handler.                                                 */
/*===========================================================================*/

#if (EFR32_DAC_USE_DAC1_CH1 || EFR32_DAC_USE_DAC1_CH2) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_VDAC_HANDLER) {
  OSAL_IRQ_PROLOGUE();

  uint32_t isr = VDAC0->IF;
  VDAC0->IF_CLR = isr;

#if EFR32_DAC_USE_DAC1_CH1 == TRUE
  if ((isr & VDAC_IF_CH0CD) != 0U) {
    _dac_isr_full_code(&DACD1);
  }
#endif
#if EFR32_DAC_USE_DAC1_CH2 == TRUE
  if ((isr & VDAC_IF_CH1CD) != 0U) {
    _dac_isr_full_code(&DACD2);
  }
#endif

  OSAL_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void dac_lld_init(void) {
#if EFR32_DAC_USE_DAC1_CH1 == TRUE
  dacObjectInit(&DACD1);
#endif
#if EFR32_DAC_USE_DAC1_CH2 == TRUE
  dacObjectInit(&DACD2);
#endif
  dac.clk_en_mask = 0U;
  dac.dma_ch1 = NULL;
  dac.dma_ch2 = NULL;
}

msg_t dac_lld_start(DACDriver *dacp) {
  if (dacp->state == DAC_STOP) {
    bool was_on = (dac.clk_en_mask & EFR32_DAC1_CLK_EN_MASK) != 0U;

#if EFR32_DAC_USE_DAC1_CH1 == TRUE
    if (&DACD1 == dacp && dac.dma_ch1 == NULL) {
      dac.dma_ch1 = dmaStreamAllocI(EFR32_DAC_DAC1_CH1_DMA_STREAM,
                                    EFR32_DAC_DAC1_IRQ_PRIORITY, NULL, NULL);
      if (dac.dma_ch1 == NULL) {
        return HAL_RET_NO_RESOURCE;
      }
    }
#endif
#if EFR32_DAC_USE_DAC1_CH2 == TRUE
    if (&DACD2 == dacp && dac.dma_ch2 == NULL) {
      dac.dma_ch2 = dmaStreamAllocI(EFR32_DAC_DAC1_CH2_DMA_STREAM,
                                    EFR32_DAC_DAC1_IRQ_PRIORITY, NULL, NULL);
      if (dac.dma_ch2 == NULL) {
        return HAL_RET_NO_RESOURCE;
      }
    }
#endif

    if (!was_on) {
      CMU->CLKEN1_SET = CMU_CLKEN1_VDAC0;
      /* Functional clock: EM01GRPACLK is always running in EM0/EM1. */
      CMU->VDAC0CLKCTRL = CMU_VDAC0CLKCTRL_CLKSEL_EM01GRPACLK;
    }

    /* On xG23 the config registers (CFG, CHxCFG) must be written while the
       module is disabled (EN=0); OUTCTRL/CMD require EN=1. */
    VDAC0->EN_CLR = VDAC_EN_EN;
    while ((VDAC0->EN & VDAC_EN_DISABLING) != 0U) {}
    while ((VDAC0->STATUS & VDAC_STATUS_SYNCBUSY) != 0U) {}
    if (!was_on) {
      /* Single-ended mode, normal warmup. */
      VDAC0->CFG = VDAC_CFG_DIFF_SINGLEENDED;
    }

#if EFR32_DAC_USE_DAC1_CH1 == TRUE
    if (&DACD1 == dacp) {
      dac.clk_en_mask |= EFR32_DAC1_CLK_EN_SHIFT(0);

      /* Channel 0: continuous, high power, immediate trigger. */
      VDAC0->CH0CFG = VDAC_CH0CFG_CONVMODE_CONTINUOUS
                    | VDAC_CH0CFG_POWERMODE_HIGHPOWER
                    | (0x1U << _VDAC_CH0CFG_TRIGMODE_SHIFT);
    }
#endif

#if EFR32_DAC_USE_DAC1_CH2 == TRUE
    if (&DACD2 == dacp) {
      dac.clk_en_mask |= EFR32_DAC1_CLK_EN_SHIFT(1);

      VDAC0->CH1CFG = VDAC_CH1CFG_CONVMODE_CONTINUOUS
                    | VDAC_CH1CFG_POWERMODE_HIGHPOWER
                    | (0x1U << _VDAC_CH1CFG_TRIGMODE_SHIFT);
    }
#endif

    VDAC0->EN_SET = VDAC_EN_EN;
    nvicEnableVector(EFR32_VDAC_NUMBER, EFR32_DAC_DAC1_IRQ_PRIORITY);

#if EFR32_DAC_USE_DAC1_CH1 == TRUE
    if ((dac.clk_en_mask & EFR32_DAC1_CLK_EN_SHIFT(0)) != 0U) {
      VDAC0->OUTCTRL_SET = VDAC_OUTCTRL_MAINOUTENCH0;
      VDAC0->CMD_SET = VDAC_CMD_CH0EN;
    }
#endif
#if EFR32_DAC_USE_DAC1_CH2 == TRUE
    if ((dac.clk_en_mask & EFR32_DAC1_CLK_EN_SHIFT(1)) != 0U) {
      VDAC0->OUTCTRL_SET = VDAC_OUTCTRL_MAINOUTENCH1;
      VDAC0->CMD_SET = VDAC_CMD_CH1EN;
    }
#endif
  }

  return HAL_RET_SUCCESS;
}

void dac_lld_stop(DACDriver *dacp) {
  if (dacp->state == DAC_READY) {
#if EFR32_DAC_USE_DAC1_CH1 == TRUE
    if (&DACD1 == dacp) {
      VDAC0->CMD_SET = VDAC_CMD_CH0DIS;
      VDAC0->OUTCTRL_CLR = VDAC_OUTCTRL_MAINOUTENCH0;
      dac.clk_en_mask &= ~EFR32_DAC1_CLK_EN_SHIFT(0);
      if (dac.dma_ch1 != NULL) {
        dmaStreamFreeI(dac.dma_ch1);
        dac.dma_ch1 = NULL;
      }
    }
#endif
#if EFR32_DAC_USE_DAC1_CH2 == TRUE
    if (&DACD2 == dacp) {
      VDAC0->CMD_SET = VDAC_CMD_CH1DIS;
      VDAC0->OUTCTRL_CLR = VDAC_OUTCTRL_MAINOUTENCH1;
      dac.clk_en_mask &= ~EFR32_DAC1_CLK_EN_SHIFT(1);
      if (dac.dma_ch2 != NULL) {
        dmaStreamFreeI(dac.dma_ch2);
        dac.dma_ch2 = NULL;
      }
    }
#endif

    /* Disable module if no channels active. */
    if ((dac.clk_en_mask & EFR32_DAC1_CLK_EN_MASK) == 0U) {
      VDAC0->EN_CLR = VDAC_EN_EN;
      while ((VDAC0->EN & VDAC_EN_DISABLING) != 0U) {}
      CMU->CLKEN1_CLR = CMU_CLKEN1_VDAC0;
      nvicDisableVector(EFR32_VDAC_NUMBER);
    }
  }
}

msg_t dac_lld_put_channel(DACDriver *dacp, dacchannel_t channel, dacsample_t sample) {
#if EFR32_DAC_USE_DAC1_CH2 == TRUE && !EFR32_DAC_DUAL_MODE
  /* Independent channel drivers: DACD2 drives the second FIFO. */
  if (dacp == &DACD2) {
    VDAC0->CH1F = (uint32_t)sample;
    return HAL_RET_SUCCESS;
  }
#endif
  (void)channel;
  VDAC0->CH0F = (uint32_t)sample;
  (void)dacp;
  return HAL_RET_SUCCESS;
}

msg_t dac_lld_start_conversion(DACDriver *dacp) {
  /* In continuous mode, writing to CHxF triggers a conversion automatically.
     For DMA-driven output, set up the DMA stream to feed CHxF. */
#if defined(EFR32_DMA_REQUIRED)
  const efr32_dma_stream_t *st = dac_dma_stream(dacp);
  if (st != NULL && dacp->grpp != NULL) {
    dacchannel_t ch = (st == dac.dma_ch1) ? 0U : 1U;
    volatile uint32_t *dst = (ch == 0U) ? &VDAC0->CH0F : &VDAC0->CH1F;

    /* Route VDAC0 DMA request to the stream. */
    dmaStreamSetRequestSource(st, LDMAXBAR_CH_REQSEL_SOURCESEL_VDAC0 >> 16, ch);

    dmaStartMemToPeriph(st, dacp->samples, dst,
                        dacp->depth, EFR32_DMA_CH_CTRL_SIZE_HALFWORD);
    return HAL_RET_SUCCESS;
  }
#else
  (void)dacp;
#endif
  return HAL_RET_SUCCESS;
}

void dac_lld_stop_conversion(DACDriver *dacp) {
#if defined(EFR32_DMA_REQUIRED)
  const efr32_dma_stream_t *st = dac_dma_stream(dacp);
  if (st != NULL) {
    dmaStreamDisable(st);
  }
#endif
  (void)dacp;
}

#endif /* HAL_USE_DAC == TRUE */

/** @} */
