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
  /* Push the shadow into the display pipeline: with UPDATECTRL.AUTOLOAD
     MANUAL the panel only loads SEGD on an explicit LOAD command — without
     this, updates after the first one never reach the glass. */
  LCD->CMD = LCD_CMD_LOAD;
  /* Bounded wait for the load sync to finish. */
  {
    volatile uint32_t sp = 100000UL;
    while (((LCD->STATUS & LCD_STATUS_LOADBUSY) != 0U) && (--sp != 0U)) {}
  }
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

  if (lcdp->state != LCD_READY) {
    CMU->CLKEN1_SET = CMU_CLKEN1_LCD;

    /* LCD clock: prefer LFXO (usually enabled for EM23GRPACLK); fall back to
       LFRCO/ULFRCO only if those are enabled in mcuconf — selecting a clock
       whose oscillator is off leaves the LCD LF domain dead (EN.DISABLING
       never clears) and the display blank. */
#if EFR32_LFXO_ENABLED
    CMU->CLKEN0_SET = CMU_CLKEN0_LFXO;
    CMU->LCDCLKCTRL = CMU_LCDCLKCTRL_CLKSEL_LFXO;
#elif EFR32_LFRCO_ENABLED
    CMU->CLKEN0_SET = CMU_CLKEN0_LFRCO;
    CMU->LCDCLKCTRL = CMU_LCDCLKCTRL_CLKSEL_LFRCO;
#else
    CMU->CLKEN0_SET = CMU_CLKEN0_ULFRCO;
    CMU->LCDCLKCTRL = CMU_LCDCLKCTRL_CLKSEL_ULFRCO;
#endif

    /* Allocate segment/common outputs (fixed pin mapping on xG23). */
    GPIO->LCDSEG = cfg->seg_enable & _GPIO_LCDSEG_MASK;
    GPIO->LCDCOM = cfg->com_enable & _GPIO_LCDCOM_MASK;

    LCD->EN_CLR = LCD_EN_EN;
    /* Wait until the disable has propagated (WSTATIC fields need EN=0).
       Bounded: if the LCD clock is dead this bit never clears, and an
       unbounded spin here would hang the caller forever. */
    {
      volatile uint32_t sp = 4000000UL;
      while (((LCD->EN & LCD_EN_DISABLING) != 0U) && (--sp != 0U)) {}
    }

    /* LCDCLK prescaler (CTRL PRESCALE). */
    LCD->CTRL = (LCD->CTRL & ~_LCD_CTRL_PRESCALE_MASK)
                | ((cfg->presc << _LCD_CTRL_PRESCALE_SHIFT)
                   & _LCD_CTRL_PRESCALE_MASK);

    /* Display control: mux, bias (voltage levels), wave, charge redist. */
    LCD->DISPCTRL = (LCD->DISPCTRL & ~(_LCD_DISPCTRL_MUX_MASK |
                                        _LCD_DISPCTRL_BIAS_MASK |
                                        _LCD_DISPCTRL_WAVE_MASK |
                                        _LCD_DISPCTRL_CHGRDST_MASK))
                    | (cfg->mux & _LCD_DISPCTRL_MUX_MASK)
                    | (cfg->bias & _LCD_DISPCTRL_BIAS_MASK)
                    | (cfg->wave & _LCD_DISPCTRL_WAVE_MASK)
                    | (cfg->charge_redist & _LCD_DISPCTRL_CHGRDST_MASK);

    /* Frame rate. */
    if (cfg->frame_rate != 0U) {
      LCD->FRAMERATE = cfg->frame_rate;
    }

    /* Analog bias: operating mode (charge pump), VLCD contrast level,
       DMA mode for display updates. VLCD=0 would blank the panel, so a
       config contrast of 0 falls back to EFR32_LCD_CONTRAST_DEFAULT. */
    {
      uint32_t contrast = (cfg->contrast != 0U) ? cfg->contrast
                                                : EFR32_LCD_CONTRAST_DEFAULT;
      LCD->BIASCTRL = (LCD->BIASCTRL & ~(_LCD_BIASCTRL_MODE_MASK |
                                         _LCD_BIASCTRL_VLCD_MASK |
                                         _LCD_BIASCTRL_DMAMODE_MASK))
                      | (cfg->mode & _LCD_BIASCTRL_MODE_MASK)
                      | ((contrast << _LCD_BIASCTRL_VLCD_SHIFT)
                         & _LCD_BIASCTRL_VLCD_MASK)
                      | LCD_BIASCTRL_DMAMODE_DMADISPLAY;
    }

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
      lcdp->dma = dmaStreamAlloc(EFR32_LCD_DMA_STREAM,
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
      dmaStreamFree(lcdp->dma);
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
    GPIO->LCDSEG = 0U;
    GPIO->LCDCOM = 0U;
    CMU->CLKEN1_CLR = CMU_CLKEN1_LCD;
    lcdp->state = LCD_STOP;
    lcdp->config = NULL;
  }
}

void lcd_lld_clear(LCDDriver *lcdp) {
  /* Shadow only — per the hal_lcd.h contract lcdClear() does not write HW;
     a direct SEGD write here, immediately followed by the update write,
     leaves the panel blank on FG23. */
  for (int i = 0; i < EFR32_LCD_NUM_SEGD; i++) {
    lcdp->segd_shadow[i] = 0U;
  }
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
