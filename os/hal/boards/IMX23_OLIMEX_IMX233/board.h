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

#ifndef _BOARD_H_
#define _BOARD_H_

/*
 * Board identifier.
 */
#define BOARD_IMX23_OLIMEX_IMX233
#define BOARD_NAME              "OLIMEX i.MX233-OLinuXino"

/*
 * SDRAM size: 64MB. This is the single knob that sizes the ChibiOS memory
 * map: the linker script (IMX23.ld) is preprocessed with this board.h, so
 * flash/ram0 regions span the whole chip. Plain literal (no U/L suffix),
 * valid in both C and linker-script context.
 */
#define IMX23_RAM_SIZE          0x4000000

/*
 * CPU clock as left by the bootstream (informational).
 */
#define IMX23_CPU_HZ            454000000

/*
 * The rest of the file is plain C and is hidden when board.h is preprocessed
 * into the linker script (the demo Makefile defines __IMX23_LDSCRIPT__).
 */
#if !defined(__IMX23_LDSCRIPT__)

/* Green LED: SSP1_DETECT pad as GPIO_2_1 (bank 2, pin 1). */
#define BOARD_LED_PORT          GPIOC
#define BOARD_LED_PAD           1U

/* DUART (PL011) console: PWM0/PWM1 pads, function 2 (bank 1, pins 26/27),
   configured in board.c. */
#define BOARD_DUART_RX_BANK     1U
#define BOARD_DUART_RX_PIN      26U
#define BOARD_DUART_TX_BANK     1U
#define BOARD_DUART_TX_PIN      27U
#define BOARD_DUART_FUNC        2U

#endif /* !__IMX23_LDSCRIPT__ */

#endif /* _BOARD_H_ */
