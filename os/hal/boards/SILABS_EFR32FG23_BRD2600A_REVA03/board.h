/*
    ChibiOS - Copyright (C) 2024 Xael South

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

#ifndef _BOARD_H_
#define _BOARD_H_

/*
 * Board identifier.
 */
#define BOARD_SILABS_EFR32FG23_BRD2600A_REVA03
#define BOARD_NAME "BOARD_SILABS_EFR32FG23_BRD2600A_REVA03"

/*
 * Board frequencies.
 */

#if defined(LFXO_FREQ)
#define EFR32_LFXO_FREQ                 LFXO_FREQ
#endif

#if !defined(EFR32_LFXO_FREQ)
#define EFR32_LFXO_FREQ                 32768UL
#endif

#if defined(HFXO_FREQ)
#define EFR32_HFXO_FREQ                 HFXO_FREQ
#endif

#if !defined(EFR32_HFXO_FREQ)
#define EFR32_HFXO_FREQ                 39000000UL
#endif

#if defined(CLKIN0_FREQ)
#define EFR32_CLKIN0_FREQ               CLKIN0_FREQ
#endif

#if !defined(EFR32_CLKIN0_FREQ)
#define EFR32_CLKIN0_FREQ               0UL
#endif

#if defined(LFRCO_FREQ)
#define EFR32_LFRCO_FREQ                LFRCO_FREQ
#endif

#if !defined(EFR32_LFRCO_FREQ)
#define EFR32_LFRCO_FREQ                32768UL
#endif

#if defined(FSRCO_FREQ)
#define EFR32_FSRCO_FREQ                FSRCO_FREQ
#endif

#if !defined(EFR32_FSRCO_FREQ)
#define EFR32_FSRCO_FREQ                20000000UL
#endif

/*
 * Board calibration values.
 */

/** LFXO default gain value manually estmated by measuring 32.768 kHz on CLKOUTn pin. */
#define LFXO_RTC_GAIN_DEFAULT_VALUE     0x02UL

/** LFXO captune default value manually estmated by measuring 32.768 kHz on CLKOUTn pin. */
#define LFXO_RTC_CAPTUNE_DEFAULT_VALUE  0x1CUL


/*
 * Hatch port and pin to get into debugger on startup.
 */

/** PA5 is BUTTON1 used for debug escape. */
#define ESCAPE_HATCH_PORT               GPIO_PORTA
#define ESCAPE_HATCH_PIN                5

/*
 * MCU type as defined in the Silicon Labs header.
 */
#if !defined(EFR32FG23B010F512IM48)
#define EFR32FG23B010F512IM48           1
#endif

/*
 * On-board segment LCD (CE322-1002 glass, 5 digits x 7 segments + 6 icons,
 * 4 COM lines, 20 segment lines; see Gecko SDK segment_lcd config
 * ce322_1002 and brd2600a/sl_segmentlcd_pin_config.h).
 * Linear segment index = COM * 20 + segment line, where the glass logical
 * segment S00..S09 maps to LCD segment lines 0,1,4,5,6,7,18,19,8,10.
 */
/*
 * NOTE: board.h is included before halconf.h/hal_lcd.h, so the LCD model
 * is declared via struct tags (lcd_model_t = struct lcd_model,
 * LCDConfig = struct lcd_config) to stay independent of include order.
 * The definitions in board.c require HAL_USE_LCD == TRUE.
 */
#if !defined(_FROM_ASM_) || defined(__DOXYGEN__)
struct lcd_model;
struct lcd_config;

/** Icons on the BRD2600A LCD (order matches the glass config). */
typedef enum {
  BOARD_LCD_ICON_P2 = 0,  /**< decimal point after digit 3 */
  BOARD_LCD_ICON_P3,      /**< decimal point after digit 4 */
  BOARD_LCD_ICON_P4,      /**< decimal point after digit 5 */
  BOARD_LCD_ICON_P5,      /**< decimal point / colon top */
  BOARD_LCD_ICON_P6,      /**< decimal point / colon bottom */
  BOARD_LCD_ICON_DEGC,    /**< degree-C symbol */
  BOARD_LCD_NUM_ICONS
} board_lcd_icon_t;

extern const struct lcd_model  board_lcd_model;    /**< CE322-1002 glass map. */
extern const struct lcd_config board_lcd_config;   /**< Recommended LCDConfig. */
#endif

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* _BOARD_H_ */
