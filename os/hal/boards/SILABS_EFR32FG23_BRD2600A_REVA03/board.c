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

#include "hal.h"

/**
 * @brief   PAL setup — the board's static default GPIO configuration.
 * @details halInit() calls palInit(&pal_default_config) (hal.c), and the PAL
 *          LLD applies this table once in _pal_lld_init(): each PAData..PDData
 *          entry programs the corresponding GPIO port registers
 *          (CTRL/MODEL/MODEH/DOUT), i.e. the reset-time mode and output level
 *          of every pin of that port.  Without it the pins would simply keep
 *          their hardware reset state.
 *          The values come from the PAL_EFR32_PORTx_{CTRL,MODEL,MODEH,DOUT}_DEFAULT
 *          macros in hal_pal_lld.h, which default every pin to
 *          PAL_EFR32_MODEL_DEFAULT (input, output driver disabled).  To give
 *          a board pin a different power-on state (e.g. LED pin as push-pull
 *          output, button pin with pull-up), redefine the matching macro in
 *          board.h — this table picks it up automatically.
 *          Runtime changes afterwards are done per pin with palSetPadMode() /
 *          palSetPad() / palClearPad(), not through this const table.
 */
#if HAL_USE_PAL || defined(__DOXYGEN__)
const PALConfig pal_default_config =
{
  .PAData = {
    .ctrlr =  PAL_EFR32_PORTA_CTRL_DEFAULT,
    .modelr = PAL_EFR32_PORTA_MODEL_DEFAULT,
    .modehr = PAL_EFR32_PORTA_MODEH_DEFAULT,
    .doutr =  PAL_EFR32_PORTA_DOUT_DEFAULT
  },

  .PBData = {
    .ctrlr =  PAL_EFR32_PORTB_CTRL_DEFAULT,
    .modelr = PAL_EFR32_PORTB_MODEL_DEFAULT,
    .modehr = PAL_EFR32_PORTB_MODEH_DEFAULT,
    .doutr =  PAL_EFR32_PORTB_DOUT_DEFAULT
  },

  .PCData = {
    .ctrlr =  PAL_EFR32_PORTC_CTRL_DEFAULT,
    .modelr = PAL_EFR32_PORTC_MODEL_DEFAULT,
    .modehr = PAL_EFR32_PORTC_MODEH_DEFAULT,
    .doutr =  PAL_EFR32_PORTC_DOUT_DEFAULT
  },

  .PDData = {
    .ctrlr =  PAL_EFR32_PORTD_CTRL_DEFAULT,
    .modelr = PAL_EFR32_PORTD_MODEL_DEFAULT,
    .modehr = PAL_EFR32_PORTD_MODEH_DEFAULT,
    .doutr =  PAL_EFR32_PORTD_DOUT_DEFAULT
  },
};
#endif

/*
 * Early initialization code.
 * This initialization must be performed just after stack setup and before
 * any other initialization.
 */
void __early_init(void) {

  efr32_escape_hatch();
  efr32_chip_init();
  efr32_clock_init();
}

/*
 * Board-specific initialization code.
 */
void boardInit(void) {

}

#if (HAL_USE_LCD == TRUE) || defined(__DOXYGEN__)

/*
 * CE322-1002 glass on BRD2600A. Logical segment S00..S09 -> LCD segment
 * line: { 0, 1, 4, 5, 6, 7, 18, 19, 8, 10 } (sl_segmentlcd_pin_config.h).
 * Linear segment index = COM * 20 + segment line.
 *
 * Digit map from SL_SEGMENT_LCD_EFM_DISPLAY_DEF (ce322_1002 config),
 * segment order a..g; digit pos 0 = rightmost (#1) .. pos 4 = leftmost (#5).
 */
static const uint8_t board_lcd_digit_map[5][8] = {
  /* #1: com={3,2,0,0,1,2,1} bit={S00,S01,S01,S00,S00,S00,S01} */
  { 60, 41,  1,  0, 20, 40, 21, 0xFF },
  /* #2: com={3,3,1,0,1,2,2} bit={S02,S03,S03,S02,S02,S02,S03} */
  { 64, 65, 25,  4, 24, 44, 45, 0xFF },
  /* #3: com={3,3,1,0,1,2,2} bit={S04,S05,S05,S04,S04,S04,S05} */
  { 66, 67, 27,  6, 26, 46, 47, 0xFF },
  /* #4: com={3,2,0,0,1,2,1} bit={S06,S07,S07,S06,S06,S06,S07} */
  { 78, 59, 19, 18, 38, 58, 39, 0xFF },
  /* #5: com={3,2,1,0,1,3,2} bit={S09,S09,S09,S09,S08,S08,S08} */
  { 70, 50, 30, 10, 28, 68, 48, 0xFF },
};

/* Icons: P2=C0/S03, P3=C0/S05, P4=C0/S08, P5=C3/S07, P6=C3/S01, DEGC=C3/S07. */
static const uint8_t board_lcd_icon_map[BOARD_LCD_NUM_ICONS] = {
  5, 7, 8, 79, 61, 79
};

const lcd_model_t board_lcd_model = {
  .num_digits = 5,
  .num_icons  = BOARD_LCD_NUM_ICONS,
  .digit_map  = board_lcd_digit_map,
  .icon_map   = board_lcd_icon_map,
};

/* Recommended config for the on-board CE322-1002 glass (Gecko SDK defaults:
   quadruplex mux, 1/3 bias, low-power wave, charge pump, contrast 15). */
const LCDConfig board_lcd_config = {
  .mux          = EFR32_LCD_MUX_QUADRUPLEX,
  .bias         = EFR32_LCD_BIAS_ONETHIRD,
  .wave         = EFR32_LCD_WAVE_LOPOWER,
  .mode         = EFR32_LCD_MODE_CHARGEPUMP,
  .charge_redist= EFR32_LCD_CHGRDST_DISABLE,
  .presc        = 63U,              /* LCDCLK = LFRCO/64 = 512 Hz */
  .frame_rate   = 1U,               /* 512/2/8 -> 32 Hz frame rate */
  .contrast     = EFR32_LCD_CONTRAST_DEFAULT,
  .seg_enable   = 0x000FFFFFU,      /* all 20 segment outputs */
  .com_enable   = 0x0000000FU,      /* all 4 COM outputs */
  .model        = &board_lcd_model,
  .callback     = NULL,
  .callback_arg = NULL,
};

#endif /* HAL_USE_LCD */
