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
#define BOARD_IMX23_EVK
#define BOARD_NAME              "NXP i.MX23 EVK (128MB SDRAM, 4.3in 800x480 LCD)"

/*
 * SDRAM size: 128MB. Single knob sizing the ChibiOS memory map; the linker
 * script (IMX23.ld) is preprocessed with this board.h. Plain literal (no
 * U/L suffix), valid in both C and linker-script context.
 */
#define IMX23_RAM_SIZE          0x8000000

/*
 * CPU clock as left by the bootstream (informational).
 */
#define IMX23_CPU_HZ            454000000

/*
 * LCD panel on the EVK: Seiko 4.3" WVGA, 800x480, 24-bit RGB, timings from
 * U-Boot drivers/video/mxsfb.c. Framebuffer is RGB565 by default.
 */
#define IMX23_LCD_WIDTH         800
#define IMX23_LCD_HEIGHT        480
#define IMX23_LCD_HSYNC         1
#define IMX23_LCD_HBP           46
#define IMX23_LCD_HFP           210
#define IMX23_LCD_VSYNC         1
#define IMX23_LCD_VBP           23
#define IMX23_LCD_VFP           22
/* Dot clock: 30MHz typ for 800x480@60 (pixel clock ~30MHz). */
#define IMX23_LCD_PIXCLK_HZ     30000000
/* Seiko 4.3in panel on the EVK: DE high active, HSYNC/VSYNC low, DOTCLK
   samples on the falling edge. */
#define IMX23_LCD_FLAGS         IMX23_LCDIF_FLAG_DE_HIGH

/* ENC28J60 SPI ethernet on SSP0 (shares pads with SSP1, bank2 pins 0,2-6).
   CS is a GPIO (adjust to your wiring), IRQ = GPIO bank2 pin3 (ETHNINT). */
#define BOARD_ENC28J60              1
#define BOARD_ENC28J60_CS_BANK      1U
#define BOARD_ENC28J60_CS_PIN       20U
#define BOARD_ENC28J60_IRQ_BANK     2U
#define BOARD_ENC28J60_IRQ_PIN      3U
/* RESET line (adjust to your wiring; 0 = use SPI soft-reset only). */
#define BOARD_ENC28J60_RESET_BANK   1U
#define BOARD_ENC28J60_RESET_PIN    21U
#define BOARD_ENC28J60_SPI_CLK_HZ   6000000U

/*
 * The rest of the file is plain C and is hidden when board.h is preprocessed
 * into the linker script (the demo Makefile defines __IMX23_LDSCRIPT__).
 */
#if !defined(__IMX23_LDSCRIPT__)

/* User LEDs: PWM3/PWM4 pads as GPIO_1_29 / GPIO_1_30 (bank 1, pins 29/30). */
#define BOARD_LED_PORT          GPIOB
#define BOARD_LED_PAD           30U
#define BOARD_LED2_PAD          29U

/* DUART (PL011) console: PWM0/PWM1 pads, function 2 (bank 1, pins 26/27). */
#define BOARD_DUART_RX_BANK     1U
#define BOARD_DUART_RX_PIN      26U
#define BOARD_DUART_TX_BANK     1U
#define BOARD_DUART_TX_PIN      27U
#define BOARD_DUART_FUNC        2U

#endif /* !__IMX23_LDSCRIPT__ */

#endif /* _BOARD_H_ */
