/*
    ChibiOS - Copyright (C) 2026 Xael South.

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
 * @file    devices_lib/display/mxs_lcdif.h
 * @brief   MXS-family LCDIF (i.MX23/i.MX28) (DOTCLK/RGB mode) framebuffer driver.
 *
 * @details Programs the LCDIF in dotclock master mode with a 24-bit RGB
 *          bus and XRGB8888 (32bpp) framebuffer in SDRAM. No DMA is
 *          needed: the LCDIF fetches the framebuffer itself in master
 *          mode. VSYNC-edge IRQ (vector 46) can be hooked for frame-done
 *          notification.
 *
 * @addtogroup IMX23
 * @{
 */

#ifndef MXS_LCDIF_H
#define MXS_LCDIF_H

#include "hal.h"

#include <stdint.h>


#define MXS_LCDIF_BASE        0x80030000U
#define MXS_LCDIF_REG(addr)    (*(volatile uint32_t *)(uintptr_t)(addr))

typedef struct {
  volatile uint32_t CTRL;         /* 0x00 SET 0x04 CLR 0x08 TOG 0x0C        */
  volatile uint32_t CTRL_SET;
  volatile uint32_t CTRL_CLR;
  volatile uint32_t CTRL_TOG;
  volatile uint32_t CTRL1;        /* 0x10 SET 0x14 CLR 0x18 TOG 0x1C        */
  volatile uint32_t CTRL1_SET;
  volatile uint32_t CTRL1_CLR;
  volatile uint32_t CTRL1_TOG;
  volatile uint32_t TRANSFER_COUNT; /* 0x20: V_COUNT[31:16] H_COUNT[15:0]   */
  volatile uint32_t TRANSFER_COUNT_SET;
  volatile uint32_t TRANSFER_COUNT_CLR;
  volatile uint32_t TRANSFER_COUNT_TOG;
  volatile uint32_t CUR_BUF;      /* 0x30                                   */
  volatile uint32_t CUR_BUF_SET;
  volatile uint32_t CUR_BUF_CLR;
  volatile uint32_t CUR_BUF_TOG;
  volatile uint32_t NEXT_BUF;     /* 0x40                                   */
  volatile uint32_t NEXT_BUF_SET;
  volatile uint32_t NEXT_BUF_CLR;
  volatile uint32_t NEXT_BUF_TOG;
  volatile uint32_t RESERVED0[4]; /* 0x50-0x5C                              */
  volatile uint32_t TIMING;       /* 0x60                                   */
  volatile uint32_t TIMING_SET;
  volatile uint32_t TIMING_CLR;
  volatile uint32_t TIMING_TOG;
  volatile uint32_t VDCTRL0;      /* 0x70                                   */
  volatile uint32_t VDCTRL0_SET;
  volatile uint32_t VDCTRL0_CLR;
  volatile uint32_t VDCTRL0_TOG;
  volatile uint32_t VDCTRL1;      /* 0x80: VSYNC_PERIOD                     */
  volatile uint32_t VDCTRL1_SET;
  volatile uint32_t VDCTRL1_CLR;
  volatile uint32_t VDCTRL1_TOG;
  volatile uint32_t VDCTRL2;      /* 0x90: HSYNC_PULSE_WIDTH[31:24] PERIOD  */
  volatile uint32_t VDCTRL2_SET;
  volatile uint32_t VDCTRL2_CLR;
  volatile uint32_t VDCTRL2_TOG;
  volatile uint32_t VDCTRL3;      /* 0xA0: H_WAIT[27:16] V_WAIT[15:0]       */
  volatile uint32_t VDCTRL3_SET;
  volatile uint32_t VDCTRL3_CLR;
  volatile uint32_t VDCTRL3_TOG;
  volatile uint32_t VDCTRL4;      /* 0xB0: DOTCLK_DLY[31:29] SYNC_ON bit18  */
  volatile uint32_t VDCTRL4_SET;  /*       DOTCLK_H_VALID_DATA_CNT[17:0]    */
  volatile uint32_t VDCTRL4_CLR;
  volatile uint32_t VDCTRL4_TOG;
  volatile uint32_t RESERVED1[0x1B0 / 4 - (0xC0 / 4)]; /* up to DATA  */
  volatile uint32_t DATA;         /* 0x1B0                                  */
  volatile uint32_t DATA_SET;
  volatile uint32_t DATA_CLR;
  volatile uint32_t DATA_TOG;
  volatile uint32_t BM_ERROR_STAT;/* 0x1C0                                  */
  volatile uint32_t BM_ERROR_STAT_SET;
  volatile uint32_t BM_ERROR_STAT_CLR;
  volatile uint32_t BM_ERROR_STAT_TOG;
  volatile uint32_t RESERVED2[4]; /* 0x1D0-0x1DC? STAT at 0x1D0           */
  volatile uint32_t STAT;         /* 0x1D0                                  */
  volatile uint32_t STAT_SET;
  volatile uint32_t STAT_CLR;
  volatile uint32_t STAT_TOG;
  volatile uint32_t VERSION;      /* 0x1E0                                  */
} mxs_lcdif_t;

#define MXS_LCDIF             ((mxs_lcdif_t *)MXS_LCDIF_BASE)

#define MXS_LCDIF_CTRL_SFTRST             (1U << 31)
#define MXS_LCDIF_CTRL_CLKGATE            (1U << 30)
#define MXS_LCDIF_CTRL_DVI_MODE           (1U << 20)
#define MXS_LCDIF_CTRL_BYPASS_COUNT       (1U << 19)
#define MXS_LCDIF_CTRL_VSYNC_MODE         (1U << 18)
#define MXS_LCDIF_CTRL_DOTCLK_MODE        (1U << 17)
#define MXS_LCDIF_CTRL_DATABUS_WIDTH_16   (0U << 10)
#define MXS_LCDIF_CTRL_DATABUS_WIDTH_8    (1U << 10)
#define MXS_LCDIF_CTRL_DATABUS_WIDTH_18   (2U << 10)
#define MXS_LCDIF_CTRL_DATABUS_WIDTH_24   (3U << 10)
#define MXS_LCDIF_CTRL_WORD_LENGTH_16     (0U << 8)
#define MXS_LCDIF_CTRL_WORD_LENGTH_8      (1U << 8)
#define MXS_LCDIF_CTRL_WORD_LENGTH_18     (2U << 8)
#define MXS_LCDIF_CTRL_WORD_LENGTH_24     (3U << 8)
#define MXS_LCDIF_CTRL_LCDIF_MASTER       (1U << 5)
#define MXS_LCDIF_CTRL_DATA_FORMAT_16     (1U << 3)
#define MXS_LCDIF_CTRL_DATA_FORMAT_24     (1U << 1)
#define MXS_LCDIF_CTRL_RUN                (1U << 0)
#define MXS_LCDIF_CTRL1_RECOVER_ON_UNDERFLOW  (1U << 24)
#define MXS_LCDIF_CTRL1_FIFO_CLEAR            (1U << 21)
#define MXS_LCDIF_CTRL1_BYTE_PACKING(v)       (((v) & 0xFU) << 16)
#define MXS_LCDIF_CTRL1_VSYNC_EDGE_IRQ_EN     (1U << 12)
#define MXS_LCDIF_CTRL1_CUR_FRAME_DONE_IRQ_EN (1U << 13)
#define MXS_LCDIF_CTRL1_VSYNC_EDGE_IRQ        (1U << 8)
#define MXS_LCDIF_CTRL1_CUR_FRAME_DONE_IRQ    (1U << 9)
#define MXS_LCDIF_VDCTRL0_ENABLE_PRESENT       (1U << 28)
#define MXS_LCDIF_VDCTRL0_VSYNC_POL           (1U << 27)
#define MXS_LCDIF_VDCTRL0_HSYNC_POL           (1U << 26)
#define MXS_LCDIF_VDCTRL0_DOTCLK_POL          (1U << 25)
#define MXS_LCDIF_VDCTRL0_ENABLE_POL          (1U << 24)
#define MXS_LCDIF_VDCTRL0_VSYNC_PERIOD_UNIT   (1U << 21)
#define MXS_LCDIF_VDCTRL0_VSYNC_PULSE_WIDTH_UNIT (1U << 20)
#define MXS_LCDIF_VDCTRL0_VSYNC_PULSE_WIDTH(v) ((v) & 0x3FFFFU)
#define MXS_LCDIF_VDCTRL2_HSYNC_PULSE_WIDTH(v) (((v) & 0xFFU) << 24)
#define MXS_LCDIF_VDCTRL2_HSYNC_PERIOD(v)      ((v) & 0x3FFFFU)
#define MXS_LCDIF_VDCTRL3_HORIZONTAL_WAIT_CNT(v) (((v) & 0xFFFU) << 16)
#define MXS_LCDIF_VDCTRL3_VERTICAL_WAIT_CNT(v)   ((v) & 0xFFFFU)
#define MXS_LCDIF_VDCTRL4_SYNC_SIGNALS_ON         (1U << 18)
#define MXS_LCDIF_VDCTRL4_DOTCLK_H_VALID_DATA_CNT(v) ((v) & 0x3FFFFU)


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Panel timing descriptor (all values in pixel/line units).
 */
typedef struct {
  uint16_t width;        /**< Active pixels per line.                    */
  uint16_t height;       /**< Active lines per frame.                    */
  uint16_t hsync_len;    /**< HSYNC pulse width.                         */
  uint16_t hfp;          /**< Horizontal front porch.                    */
  uint16_t hbp;          /**< Horizontal back porch.                     */
  uint16_t vsync_len;    /**< VSYNC pulse width.                         */
  uint16_t vfp;          /**< Vertical front porch.                      */
  uint16_t vbp;          /**< Vertical back porch.                       */
  uint32_t pixclk_hz;    /**< Desired pixel clock.                       */
  uint8_t  flags;        /**< MXS_LCDIF_FLAG_* polarity bits.          */
} mxs_lcdif_timing_t;

#define MXS_LCDIF_FLAG_HSYNC_HIGH   0x01U
#define MXS_LCDIF_FLAG_VSYNC_HIGH   0x02U
#define MXS_LCDIF_FLAG_DE_HIGH      0x04U
#define MXS_LCDIF_FLAG_PCLK_RISING  0x08U

/**
 * @brief   Initializes the pixel clock and the LCDIF in dotclock mode.
 *
 * @param[in] t     panel timing
 * @param[in] fb    framebuffer address (XRGB8888, width*height*4 bytes,
 *                  32-byte aligned), must stay valid
 *
 * @notapi
 */
void mxs_lcdif_init(const mxs_lcdif_timing_t *t, void *fb);

/**
 * @brief   Returns the framebuffer address programmed into the LCDIF.
 *
 * @notapi
 */
void *mxs_lcdif_get_fb(void);

/**
 * @brief   Points the LCDIF at a new framebuffer (takes effect next frame).
 *
 * @notapi
 */
void mxs_lcdif_set_fb(void *fb);

/**
 * @brief   Registers a VSYNC-edge callback (called from IRQ context on
 *          vector 46, i.e. start of each frame).
 *
 * @notapi
 */
void mxs_lcdif_set_vsync_callback(void (*cb)(void));

#ifdef __cplusplus
}
#endif

#endif /* MXS_LCDIF_H */

/** @} */
