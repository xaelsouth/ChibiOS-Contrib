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
 * @file    LCDv1/hal_lcd_lld.h
 * @brief   EFR32FG23 LCD segment controller LLD header.
 *
 * @details The FG23 LCD controller drives up to 80 segments (4 SEGD registers
 *          × 20 bits) with up to 4 COM lines (quadruplex mux). DMA can be
 *          used to stream SEGD data for animation/frame updates.
 *
 *          Multiple screen models are supported via a vtable (lcdd_model_t)
 *          that maps logical elements (digits, icons) to physical segments.
 *
 * @addtogroup LCD
 * @{
 */

#ifndef HAL_LCD_LLD_H
#define HAL_LCD_LLD_H

#if (HAL_USE_LCD == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/** Number of SEGD data registers. */
#define EFR32_LCD_NUM_SEGD          4
/** Bits per SEGD register (20 bits used). */
#define EFR32_LCD_SEGD_BITS         20
/** Total segment data bits = 4 × 20 = 80. */
#define EFR32_LCD_MAX_SEGMENTS      (EFR32_LCD_NUM_SEGD * EFR32_LCD_SEGD_BITS)

/* MUX modes (COM lines). */
#define EFR32_LCD_MUX_STATIC        LCD_DISPCTRL_MUX_STATIC
#define EFR32_LCD_MUX_DUPLEX        LCD_DISPCTRL_MUX_DUPLEX
#define EFR32_LCD_MUX_TRIPLEX       LCD_DISPCTRL_MUX_TRIPLEX
#define EFR32_LCD_MUX_QUADRUPLEX    LCD_DISPCTRL_MUX_QUADRUPLEX

/* Bias settings (DISPCTRL BIAS) — number of voltage levels. */
#define EFR32_LCD_BIAS_STATIC       LCD_DISPCTRL_BIAS_STATIC
#define EFR32_LCD_BIAS_ONEHALF      LCD_DISPCTRL_BIAS_ONEHALF
#define EFR32_LCD_BIAS_ONETHIRD     LCD_DISPCTRL_BIAS_ONETHIRD
#define EFR32_LCD_BIAS_ONEFOURTH    LCD_DISPCTRL_BIAS_ONEFOURTH

/* Wave types (DISPCTRL WAVE). */
#define EFR32_LCD_WAVE_LOPOWER      LCD_DISPCTRL_WAVE_TYPEB
#define EFR32_LCD_WAVE_NORMAL       LCD_DISPCTRL_WAVE_TYPEA

/* Operating mode (BIASCTRL MODE): charge pump needs no external divider. */
#define EFR32_LCD_MODE_STEPDOWN     LCD_BIASCTRL_MODE_STEPDOWN
#define EFR32_LCD_MODE_CHARGEPUMP   LCD_BIASCTRL_MODE_CHARGEPUMP

/* Charge redistribution cycles (DISPCTRL CHGRDST). */
#define EFR32_LCD_CHGRDST_DISABLE   LCD_DISPCTRL_CHGRDST_DISABLE
#define EFR32_LCD_CHGRDST_ONE       LCD_DISPCTRL_CHGRDST_ONE
#define EFR32_LCD_CHGRDST_TWO       LCD_DISPCTRL_CHGRDST_TWO
#define EFR32_LCD_CHGRDST_THREE     LCD_DISPCTRL_CHGRDST_THREE
#define EFR32_LCD_CHGRDST_FOUR      LCD_DISPCTRL_CHGRDST_FOUR

/** Default contrast (BIASCTRL VLCD level, 0..31) when config passes 0. */
#define EFR32_LCD_CONTRAST_DEFAULT  15U

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

#if !defined(EFR32_LCD_USE_LCD) || defined(__DOXYGEN__)
#define EFR32_LCD_USE_LCD           FALSE
#endif

#if !defined(EFR32_LCD_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_LCD_IRQ_PRIORITY      3
#endif

/* DMA stream for SEGD updates. EFR32_DMA_STREAM_ID_ANY = auto-alloc,
   0-7 = pin, -1 = disable DMA (CPU writes SEGD directly). */
#if !defined(EFR32_LCD_DMA_STREAM) || defined(__DOXYGEN__)
#define EFR32_LCD_DMA_STREAM        EFR32_DMA_STREAM_ID_ANY
#endif
#if !defined(EFR32_LCD_DMA_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_LCD_DMA_IRQ_PRIORITY  5
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

typedef enum {
  LCD_UNINIT = 0,
  LCD_STOP = 1,
  LCD_READY = 2
} lcdstate_t;

/**
 * @brief   LCD screen model — maps logical elements to physical segments.
 * @details Different boards (BRD2600A, custom boards) have different
 *          segment mappings. Provide a model via the config.
 */
typedef struct lcd_model {
  uint32_t  num_digits;       /**< Number of 7-segment digits. */
  uint32_t  num_icons;        /**< Number of icon elements. */
  /**
   * @brief   Segment map: for each digit position, which SEGD bit controls
   *          each of the 7 segments (a-g) + DP. Indexed [digit][segment].
   *          segment 0=a, 1=b, ..., 6=g, 7=DP. Value = linear segment index
   *          (0-79), or 0xFF if not present.
   */
  const uint8_t (*digit_map)[8];
  /**
   * @brief   Icon map: icon index → linear segment index.
   */
  const uint8_t *icon_map;
} lcd_model_t;

/**
 * @brief   LCD driver configuration.
 */
typedef struct lcd_config {
  uint32_t          mux;          /**< EFR32_LCD_MUX_*. */
  uint32_t          bias;         /**< EFR32_LCD_BIAS_* (DISPCTRL BIAS). */
  uint32_t          wave;         /**< EFR32_LCD_WAVE_*. */
  uint32_t          mode;         /**< EFR32_LCD_MODE_* (charge pump/step down). */
  uint32_t          charge_redist;/**< EFR32_LCD_CHGRDST_*. */
  uint32_t          presc;        /**< LCD CTRL PRESCALE (LCDCLK divider, 0 = /1). */
  uint32_t          frame_rate;   /**< FRAMERATE value (0 = hardware default). */
  uint32_t          contrast;     /**< BIASCTRL VLCD level 0..31 (0 = EFR32_LCD_CONTRAST_DEFAULT). */
  uint32_t          seg_enable;   /**< GPIO->LCDSEG mask (bit n = segment n output on). */
  uint32_t          com_enable;   /**< GPIO->LCDCOM mask (bit n = COM n output on). */
  const lcd_model_t *model;       /**< Screen model (segment mapping). */
  void            (*callback)(void *arg, uint32_t events);
  void             *callback_arg;
} LCDConfig;   /* struct lcd_config */

/**
 * @brief   LCD driver structure.
 */
typedef struct {
  lcdstate_t              state;
  const LCDConfig        *config;
  /* Shadow segment data (4 × 32-bit, only 20 bits used per reg). */
  uint32_t                segd_shadow[EFR32_LCD_NUM_SEGD];
  /* DMA stream for SEGD updates (NULL if not allocated). */
  const void             *dma;
} LCDDriver;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if EFR32_LCD_USE_LCD && !defined(__DOXYGEN__)
extern LCDDriver LCDD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void lcd_lld_init(void);
  void lcd_lld_start(LCDDriver *lcdp, const LCDConfig *cfg);
  void lcd_lld_stop(LCDDriver *lcdp);
  void lcd_lld_clear(LCDDriver *lcdp);
  void lcd_lld_update(LCDDriver *lcdp);
  void lcd_lld_update_dma(LCDDriver *lcdp);
  void lcd_lld_set_segment(LCDDriver *lcdp, uint32_t seg, bool on);
  void lcd_lld_write_digit(LCDDriver *lcdp, uint32_t pos, uint8_t digit);
  void lcd_lld_write_icon(LCDDriver *lcdp, uint32_t icon, bool on);
  void lcd_lld_serve_interrupt(LCDDriver *lcdp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_LCD == TRUE */

#endif /* HAL_LCD_LLD_H */

/** @} */
