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
 * @file    LCDv1/hal_lcd_lld.c
 * @brief   EFR32FG23 LCD segment controller LLD source.
 *
 * @addtogroup LCD
 * @{
 */

#include "hal.h"

#if (EFR32_LCD_USE_LCD == TRUE) || defined(__DOXYGEN__)

#if !defined(EFR32_DMA_REQUIRED)
#define EFR32_DMA_REQUIRED
#endif

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

LCDDriver LCDD1;

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/* 7-segment font: bit 0=a, 1=b, 2=c, 3=d, 4=e, 5=f, 6=g, 7=DP. */
static const uint8_t lcd_7seg_font[16] = {
  0x3F,  /* 0: a,b,c,d,e,f   */
  0x06,  /* 1: b,c             */
  0x5B,  /* 2: a,b,d,e,g     */
  0x4F,  /* 3: a,b,c,d,g     */
  0x66,  /* 4: b,c,f,g       */
  0x6D,  /* 5: a,c,d,f,g     */
  0x7D,  /* 6: a,c,d,e,f,g   */
  0x07,  /* 7: a,b,c         */
  0x7F,  /* 8: all           */
  0x6F,  /* 9: a,b,c,d,f,g   */
  0x77,  /* A                */
  0x7C,  /* b                */
  0x39,  /* C                */
  0x5E,  /* d                */
  0x79,  /* E                */
  0x71,  /* F                */
};

/*===========================================================================*/
/* IRQ handler.                                                              */
/*===========================================================================*/

OSAL_IRQ_HANDLER(EFR32_LCD_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  lcd_lld_serve_interrupt(&LCDD1);
  OSAL_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static void lcd_lld_write_segd_regs(LCDDriver *lcdp) {
  LCD->SEGD0 = lcdp->segd_shadow[0];
  LCD->SEGD1 = lcdp->segd_shadow[1];
  LCD->SEGD2 = lcdp->segd_shadow[2];
  LCD->SEGD3 = lcdp->segd_shadow[3];
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void lcd_lld_init(void) {
  LCDD1.state = LCD_UNINIT;
  LCDD1.config = NULL;
  LCDD1.dma = NULL;
  for (int i = 0; i < EFR32_LCD_NUM_SEGD; i++) {
    LCDD1.segd_shadow[i] = 0U;
  }
}

void lcd_lld_start(LCDDriver *lcdp, const LCDConfig *cfg) {
  lcdp->config = cfg;

  if (lcdp->state == LCD_STOP) {
    CMU->CLKEN1_SET = CMU_CLKEN1_LCD;

    LCD->EN_CLR = LCD_EN_EN;

    /* Display control: mux mode, wave type. */
    LCD->DISPCTRL = (LCD->DISPCTRL & ~(_LCD_DISPCTRL_MUX_MASK |
                                        _LCD_DISPCTRL_WAVE_MASK))
                    | (cfg->mux & _LCD_DISPCTRL_MUX_MASK)
                    | (cfg->wave & _LCD_DISPCTRL_WAVE_MASK);

    /* Frame rate. */
    if (cfg->frame_rate != 0U) {
      LCD->FRAMERATE = cfg->frame_rate;
    }

    /* Bias control: contrast + DMA mode for display updates. */
    LCD->BIASCTRL = (LCD->BIASCTRL & ~_LCD_BIASCTRL_DMAMODE_MASK)
                    | LCD_BIASCTRL_DMAMODE_DMADISPLAY
                    | (cfg->contrast & ~_LCD_BIASCTRL_DMAMODE_MASK);

    /* Clear segment data. */
    LCD->SEGD0 = 0U; LCD->SEGD1 = 0U; LCD->SEGD2 = 0U; LCD->SEGD3 = 0U;
    for (int i = 0; i < EFR32_LCD_NUM_SEGD; i++) {
      lcdp->segd_shadow[i] = 0U;
    }

    LCD->IF_CLR = _LCD_IF_MASK;
    LCD->IEN = 0U;

    /* Enable LCD. */
    LCD->EN_SET = LCD_EN_EN;

    /* Allocate DMA stream for SEGD updates. */
#if defined(EFR32_DMA_REQUIRED)
    if (EFR32_LCD_DMA_STREAM >= 0 && lcdp->dma == NULL) {
      lcdp->dma = dmaStreamAllocI(EFR32_LCD_DMA_STREAM,
                                  EFR32_LCD_DMA_IRQ_PRIORITY, NULL, NULL);
    }
#endif

    nvicEnableVector(EFR32_LCD_NUMBER, EFR32_LCD_IRQ_PRIORITY);
  }

  lcdp->state = LCD_READY;
}

void lcd_lld_stop(LCDDriver *lcdp) {
  if (lcdp->state == LCD_READY) {
#if defined(EFR32_DMA_REQUIRED)
    if (lcdp->dma != NULL) {
      dmaStreamFreeI(lcdp->dma);
      lcdp->dma = NULL;
    }
#endif
    nvicDisableVector(EFR32_LCD_NUMBER);
    LCD->IEN = 0U;
    LCD->IF_CLR = _LCD_IF_MASK;
    LCD->EN_CLR = LCD_EN_EN;
    /* Wait for the disable to propagate before gating the bus clock —
       touching the peripheral with its clock off bus-faults. */
    {
      volatile uint32_t sp = 1000000UL;
      while (((LCD->EN & LCD_EN_DISABLING) != 0U) && (--sp != 0U)) {}
    }
    CMU->CLKEN1_CLR = CMU_CLKEN1_LCD;
    lcdp->state = LCD_STOP;
    lcdp->config = NULL;
  }
}

void lcd_lld_clear(LCDDriver *lcdp) {
  for (int i = 0; i < EFR32_LCD_NUM_SEGD; i++) {
    lcdp->segd_shadow[i] = 0U;
  }
  lcd_lld_write_segd_regs(lcdp);
}

void lcd_lld_update(LCDDriver *lcdp) {
  lcd_lld_write_segd_regs(lcdp);
}

#if defined(EFR32_DMA_REQUIRED) || defined(__DOXYGEN__)
/**
 * @brief   Update segment data via DMA.
 * @details Streams the 4 SEGD shadow words from memory to the LCD SEGD
 *          registers using the LCD DMA request. Falls back to CPU writes
 *          if no DMA stream is allocated.
 */
void lcd_lld_update_dma(LCDDriver *lcdp) {
  const efr32_dma_stream_t *st = lcdp->dma;
  if (st != NULL) {
    dmaStreamSetRequestSource(st, LDMAXBAR_CH_REQSEL_SOURCESEL_LCD >> 16,
                              LDMAXBAR_CH_REQSEL_SIGSEL_LCD & 0xFU);
    dmaStartMemToPeriph(st, lcdp->segd_shadow, &LCD->SEGD0,
                        EFR32_LCD_NUM_SEGD, EFR32_DMA_CH_CTRL_SIZE_WORD);
    dmaWaitCompletion(st);
    dmaStreamDisable(st);
  } else {
    lcd_lld_update(lcdp);
  }
}
#endif

void lcd_lld_set_segment(LCDDriver *lcdp, uint32_t seg, bool on) {
  if (seg >= EFR32_LCD_MAX_SEGMENTS) return;
  uint32_t reg = seg / EFR32_LCD_SEGD_BITS;
  uint32_t bit = seg % EFR32_LCD_SEGD_BITS;
  if (on) {
    lcdp->segd_shadow[reg] |= (1UL << bit);
  } else {
    lcdp->segd_shadow[reg] &= ~(1UL << bit);
  }
}

void lcd_lld_write_digit(LCDDriver *lcdp, uint32_t pos, uint8_t digit) {
  const lcd_model_t *model = lcdp->config->model;
  if (model == NULL || pos >= model->num_digits || digit > 0x0F) return;

  uint8_t seg_pattern = lcd_7seg_font[digit];
  for (uint32_t s = 0; s < 8; s++) {
    uint8_t seg_idx = model->digit_map[pos][s];
    if (seg_idx != 0xFFU) {
      lcd_lld_set_segment(lcdp, seg_idx, (seg_pattern & (1U << s)) != 0U);
    }
  }
}

void lcd_lld_write_icon(LCDDriver *lcdp, uint32_t icon, bool on) {
  const lcd_model_t *model = lcdp->config->model;
  if (model == NULL || icon >= model->num_icons) return;
  uint8_t seg_idx = model->icon_map[icon];
  if (seg_idx != 0xFFU) {
    lcd_lld_set_segment(lcdp, seg_idx, on);
  }
}

void lcd_lld_serve_interrupt(LCDDriver *lcdp) {
  uint32_t isr = LCD->IF;
  LCD->IF_CLR = isr;
  if (lcdp->config && lcdp->config->callback) {
    lcdp->config->callback(lcdp->config->callback_arg, isr);
  }
}

#endif /* EFR32_LCD_USE_LCD == TRUE */

/** @} */
