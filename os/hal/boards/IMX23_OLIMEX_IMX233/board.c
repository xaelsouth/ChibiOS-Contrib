/*
    ChibiOS - Copyright (C) 2006-2026 Xael South.

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
 * @file    boards/IMX23_OLIMEX_IMX233/board.c
 * @brief   OLIMEX i.MX233-OLinuXino board file.
 *
 * @details Pinmux ported from U-Boot board/olimex/mx23_olinuxino/spl_boot.c.
 *          EMI (DRAM) pads are configured by the bootstream, not here.
 */

#include "hal.h"

#if !defined(__IMX23_LDSCRIPT__)

/* Pad table: {bank, pin, func, drive}. */
typedef struct {
  uint32_t bank;
  uint32_t pin;
  uint32_t func;
  uint32_t drive;
} imx23_pad_t;

#define DRIVE_SSP1      (IMX23_DRIVE_8MA | IMX23_DRIVE_3V3 | IMX23_DRIVE_PULLUP)
#define DRIVE_GPIO      (IMX23_DRIVE_4MA | IMX23_DRIVE_3V3)

static const imx23_pad_t board_pads[] = {
  /* DUART console */
  { 1, 26, 2, IMX23_DRIVE_4MA | IMX23_DRIVE_3V3 },   /* PWM0  -> DUART_RX */
  { 1, 27, 2, IMX23_DRIVE_4MA | IMX23_DRIVE_3V3 },   /* PWM1  -> DUART_TX */

  /* Green LED */
  { 2,  1, 3, DRIVE_GPIO },                          /* SSP1_DETECT -> GPIO_2_1 */

  /* SD card on SSP1 */
  { 2,  0, 0, DRIVE_SSP1 },                          /* SSP1_CMD  */
  { 2,  2, 0, DRIVE_SSP1 },                          /* SSP1_DATA0 */
  { 2,  3, 0, DRIVE_SSP1 },                          /* SSP1_DATA1 */
  { 2,  4, 0, DRIVE_SSP1 },                          /* SSP1_DATA2 */
  { 2,  5, 0, DRIVE_SSP1 },                          /* SSP1_DATA3 */
  { 2,  6, 0, DRIVE_SSP1 },                          /* SSP1_SCK  */
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
