/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

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
 * @file    boards/IMX23_EVK/board.c
 * @brief   NXP i.MX23 EVK board file.
 *
 * @details Pinmux ported from U-Boot board/nxp/mx23evk/spl_boot.c (including
 *          the LCDIF pads for the on-board Seiko 4.3" panel). EMI (DRAM)
 *          pads are configured by the bootstream, not here.
 */

#include "hal.h"

#if !defined(__IMX23_LDSCRIPT__)

typedef struct {
  uint32_t bank;
  uint32_t pin;
  uint32_t func;
  uint32_t drive;
} imx23_pad_t;

#define DRIVE_SSP1      (IMX23_DRIVE_8MA | IMX23_DRIVE_3V3 | IMX23_DRIVE_PULLUP)
#define DRIVE_GPIO      (IMX23_DRIVE_4MA | IMX23_DRIVE_3V3)
#define DRIVE_LCD       (IMX23_DRIVE_4MA | IMX23_DRIVE_3V3)

static const imx23_pad_t board_pads[] = {
  /* DUART console */
  { 1, 26, 2, IMX23_DRIVE_4MA | IMX23_DRIVE_3V3 },   /* PWM0 -> DUART_RX */
  { 1, 27, 2, IMX23_DRIVE_4MA | IMX23_DRIVE_3V3 },   /* PWM1 -> DUART_TX */

  /* User LEDs */
  { 1, 29, 3, DRIVE_GPIO },                          /* PWM3 -> GPIO_1_29 */
  { 1, 30, 3, DRIVE_GPIO },                          /* PWM4 -> GPIO_1_30 */

  /* SD card on SSP1 */
  { 2,  0, 0, DRIVE_SSP1 },                          /* SSP1_CMD   */
  { 2,  2, 0, DRIVE_SSP1 },                          /* SSP1_DATA0 */
  { 2,  3, 0, DRIVE_SSP1 },                          /* SSP1_DATA1 */
  { 2,  4, 0, DRIVE_SSP1 },                          /* SSP1_DATA2 */
  { 2,  5, 0, DRIVE_SSP1 },                          /* SSP1_DATA3 */
  { 2,  6, 0, DRIVE_SSP1 },                          /* SSP1_SCK   */

  /* LCDIF, 24-bit: LCD_D00..D17 on bank1 pins 0..17 func 0,
     LCD_D18..D23 on GPMI_D08..D13 (bank0 pins 8..13) func 1. */
  { 1,  0, 0, DRIVE_LCD }, { 1,  1, 0, DRIVE_LCD },
  { 1,  2, 0, DRIVE_LCD }, { 1,  3, 0, DRIVE_LCD },
  { 1,  4, 0, DRIVE_LCD }, { 1,  5, 0, DRIVE_LCD },
  { 1,  6, 0, DRIVE_LCD }, { 1,  7, 0, DRIVE_LCD },
  { 1,  8, 0, DRIVE_LCD }, { 1,  9, 0, DRIVE_LCD },
  { 1, 10, 0, DRIVE_LCD }, { 1, 11, 0, DRIVE_LCD },
  { 1, 12, 0, DRIVE_LCD }, { 1, 13, 0, DRIVE_LCD },
  { 1, 14, 0, DRIVE_LCD }, { 1, 15, 0, DRIVE_LCD },
  { 1, 16, 0, DRIVE_LCD }, { 1, 17, 0, DRIVE_LCD },
  { 0,  8, 1, DRIVE_LCD }, { 0,  9, 1, DRIVE_LCD },
  { 0, 10, 1, DRIVE_LCD }, { 0, 11, 1, DRIVE_LCD },
  { 0, 12, 1, DRIVE_LCD }, { 0, 13, 1, DRIVE_LCD },
  { 1, 22, 0, DRIVE_LCD },                           /* LCD_DOTCK  */
  { 1, 23, 0, DRIVE_LCD },                           /* LCD_ENABLE */
  { 1, 24, 0, DRIVE_LCD },                           /* LCD_HSYNC  */
  { 1, 25, 0, DRIVE_LCD },                           /* LCD_VSYNC  */
};

/**
 * @brief   Board initialization.
 */
void boardInit(void) {
  unsigned i;

  for (i = 0U; i < sizeof(board_pads) / sizeof(board_pads[0]); i++) {
    imx23_iomux_set_pad(board_pads[i].bank, board_pads[i].pin,
                       board_pads[i].func, board_pads[i].drive);
  }
}

#endif /* !__IMX23_LDSCRIPT__ */
