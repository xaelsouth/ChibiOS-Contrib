/*
    ChibiOS - Copyright (C) 2026 Giovanni Di Sirio.

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
 * @file    IMX23/IMX23xx/imx23_lcdif.c
 * @brief   i.MX23 LCDIF driver (ported from U-Boot mxsfb.c / Linux
 *          drivers/gpu/drm/mxsfb for the i.MX23 register layout).
 *
 * @addtogroup IMX23
 * @{
 */

#include "hal.h"
#include "icoll.h"
#include "imx23_lcdif.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

static void (*vsync_cb)(void);
static void *lcd_fb;

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/*
 * Program the pixel clock (ref_pix) from PLL0 (480MHz) using the fractional
 * divider, ported from U-Boot mxs_set_lcdclk() for i.MX23:
 *
 *   freq = 480MHz * 18 / x / k,  x in 18..35 (FRAC), k = integer DIV.
 */
static void lcdif_set_pixclk(uint32_t freq_hz) {
  volatile uint32_t *pix = (volatile uint32_t *)IMX23_CLKCTRL_PIX;
  volatile uint8_t *frac0 = (volatile uint8_t *)IMX23_CLKCTRL_FRAC0;
  uint32_t freq_khz = freq_hz / 1000U;
  uint32_t fp, tk;
  uint32_t k_best_l = 999, k_best_t = 0;
  uint32_t x_best_l = 0xFFU, x_best_t = 0xFFU;
  uint32_t k_best, x_best;
  int x;

  if (freq_khz == 0U) {
    return;
  }

  /* Run ref_pix from PLL0, not from the XTAL. */
  *(volatile uint32_t *)(IMX23_CLKCTRL_CLKSEQ + 0x08U) =
      IMX23_CLKCTRL_CLKSEQ_BYPASS_PIX;   /* CLR */

  fp = ((IMX23_PLL0_HZ / 1000U) / freq_khz) * 18U;

  for (x = 18; x <= 35; x++) {
    uint32_t k_rest;
    tk = fp / (uint32_t)x;
    if ((tk / 1000U == 0U) || (tk / 1000U > 255U)) {
      continue;
    }
    k_rest = tk % 1000U;
    if (k_rest < (k_best_l % 1000U)) {
      k_best_l = tk;
      x_best_l = (uint32_t)x;
    }
    if (k_rest > (k_best_t % 1000U)) {
      k_best_t = tk;
      x_best_t = (uint32_t)x;
    }
  }

  if (1000U - (k_best_t % 1000U) > (k_best_l % 1000U)) {
    k_best = k_best_l;
    x_best = x_best_l;
  }
  else {
    k_best = k_best_t;
    x_best = x_best_t;
  }
  k_best /= 1000U;
  if (k_best == 0U) {
    k_best = 1U;
  }

  /* FRAC byte (mxs_reg_8: reg[4], _set[4], _clr[4], _tog[4]): gate,
     program x, ungate. */
  frac0[IMX23_CLKCTRL_FRAC0_PIX + 4U] = IMX23_CLKCTRL_FRAC_CLKGATE;  /* SET */
  frac0[IMX23_CLKCTRL_FRAC0_PIX] =
      IMX23_CLKCTRL_FRAC_CLKGATE | (uint8_t)(x_best & IMX23_CLKCTRL_FRAC_MASK);
  frac0[IMX23_CLKCTRL_FRAC0_PIX + 8U] = IMX23_CLKCTRL_FRAC_CLKGATE;  /* CLR */

  /* Integer divider, ungate. */
  *pix = IMX23_CLKCTRL_PIX_CLKGATE;               /* SET gate */
  *pix = (IMX23_CLKCTRL_PIX_DIV_MASK |
          IMX23_CLKCTRL_PIX_CLKGATE);            /* CLR field+gate */
  *(pix + 1U) = (k_best & IMX23_CLKCTRL_PIX_DIV_MASK); /* SET div */

  while ((*pix & IMX23_CLKCTRL_PIX_BUSY) != 0U) {
    /* wait for the divider to settle */
  }
}

/* VSYNC-edge IRQ handler (ICOLL vector 46). */
static bool lcdif_vsync_handler(void) {

  /* Acknowledge the vsync edge. */
  IMX23_LCDIF->CTRL1_CLR = IMX23_LCDIF_CTRL1_VSYNC_EDGE_IRQ;

  if (vsync_cb != NULL) {
    vsync_cb();
  }

  return false;   /* not a wakeup source */
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

void imx23_lcdif_init(const imx23_lcdif_timing_t *t, void *fb) {
  imx23_lcdif_t *lcd = IMX23_LCDIF;
  uint32_t vdctrl0;

  osalDbgCheck(t != NULL);
  osalDbgCheck(fb != NULL);

  lcd_fb = fb;
  vsync_cb = NULL;

  /* Pixel clock. */
  lcdif_set_pixclk(t->pixclk_hz);

  /* Restart the LCDIF block (SFTRST/CLKGATE, then ungate). */
  lcd->CTRL_SET = IMX23_LCDIF_CTRL_SFTRST | IMX23_LCDIF_CTRL_CLKGATE;
  lcd->CTRL_CLR = IMX23_LCDIF_CTRL_SFTRST | IMX23_LCDIF_CTRL_CLKGATE;

  /* XRGB8888 framebuffer on a 24-bit RGB bus, dotclock master mode. */
  lcd->CTRL = IMX23_LCDIF_CTRL_DATABUS_WIDTH_24 |
              IMX23_LCDIF_CTRL_WORD_LENGTH_24 |
              IMX23_LCDIF_CTRL_DOTCLK_MODE |
              IMX23_LCDIF_CTRL_BYPASS_COUNT |
              IMX23_LCDIF_CTRL_LCDIF_MASTER;

  lcd->CTRL1 = IMX23_LCDIF_CTRL1_BYTE_PACKING(0x7) |
               IMX23_LCDIF_CTRL1_RECOVER_ON_UNDERFLOW;

  lcd->TRANSFER_COUNT = ((uint32_t)t->height << 16) | (uint32_t)t->width;

  vdctrl0 = IMX23_LCDIF_VDCTRL0_ENABLE_PRESENT |
            IMX23_LCDIF_VDCTRL0_ENABLE_POL |
            IMX23_LCDIF_VDCTRL0_VSYNC_PERIOD_UNIT |
            IMX23_LCDIF_VDCTRL0_VSYNC_PULSE_WIDTH_UNIT |
            IMX23_LCDIF_VDCTRL0_VSYNC_PULSE_WIDTH(t->vsync_len);
  if (t->flags & IMX23_LCDIF_FLAG_HSYNC_HIGH) {
    vdctrl0 |= IMX23_LCDIF_VDCTRL0_HSYNC_POL;
  }
  if (t->flags & IMX23_LCDIF_FLAG_VSYNC_HIGH) {
    vdctrl0 |= IMX23_LCDIF_VDCTRL0_VSYNC_POL;
  }
  if (t->flags & IMX23_LCDIF_FLAG_PCLK_RISING) {
    vdctrl0 |= IMX23_LCDIF_VDCTRL0_DOTCLK_POL;
  }
  if (t->flags & IMX23_LCDIF_FLAG_DE_HIGH) {
    vdctrl0 |= IMX23_LCDIF_VDCTRL0_ENABLE_POL;
  }
  lcd->VDCTRL0 = vdctrl0;

  lcd->VDCTRL1 = (uint32_t)(t->vbp + t->vfp + t->vsync_len + t->height);
  lcd->VDCTRL2 = IMX23_LCDIF_VDCTRL2_HSYNC_PULSE_WIDTH(t->hsync_len) |
                 IMX23_LCDIF_VDCTRL2_HSYNC_PERIOD(
                     (uint32_t)(t->hbp + t->hfp + t->hsync_len + t->width));
  lcd->VDCTRL3 = IMX23_LCDIF_VDCTRL3_HORIZONTAL_WAIT_CNT(
                     (uint32_t)(t->hbp + t->hsync_len)) |
                 IMX23_LCDIF_VDCTRL3_VERTICAL_WAIT_CNT(
                     (uint32_t)(t->vbp + t->vsync_len));
  lcd->VDCTRL4 = IMX23_LCDIF_VDCTRL4_DOTCLK_H_VALID_DATA_CNT(t->width);

  lcd->CUR_BUF = (uint32_t)(uintptr_t)fb;
  lcd->NEXT_BUF = (uint32_t)(uintptr_t)fb;

  /* Flush FIFO. */
  lcd->CTRL1_SET = IMX23_LCDIF_CTRL1_FIFO_CLEAR;

  /* Sync signals ON. */
  lcd->VDCTRL4_SET = IMX23_LCDIF_VDCTRL4_SYNC_SIGNALS_ON;

  /* FIFO clear release, then RUN. */
  lcd->CTRL1_CLR = IMX23_LCDIF_CTRL1_FIFO_CLEAR;
  lcd->CTRL_SET = IMX23_LCDIF_CTRL_RUN;
}

void *imx23_lcdif_get_fb(void) {

  return lcd_fb;
}

void imx23_lcdif_set_fb(void *fb) {

  lcd_fb = fb;
  IMX23_LCDIF->CUR_BUF = (uint32_t)(uintptr_t)fb;
  IMX23_LCDIF->NEXT_BUF = (uint32_t)(uintptr_t)fb;
}

void imx23_lcdif_set_vsync_callback(void (*cb)(void)) {

  osalSysLock();
  vsync_cb = cb;
  if (cb != NULL) {
    icoll_register_handler(IMX23_IRQ_LCDIF0, lcdif_vsync_handler);
    IMX23_LCDIF->CTRL1_SET = IMX23_LCDIF_CTRL1_VSYNC_EDGE_IRQ_EN;
    icoll_irq_enable(IMX23_IRQ_LCDIF0);
  }
  else {
    IMX23_LCDIF->CTRL1_CLR = IMX23_LCDIF_CTRL1_VSYNC_EDGE_IRQ_EN;
    icoll_irq_disable(IMX23_IRQ_LCDIF0);
  }
  osalSysUnlock();
}

/** @} */
