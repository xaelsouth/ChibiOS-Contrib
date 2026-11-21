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
 * @file    IMX23/IMX23xx/imx23_lcdif.h
 * @brief   i.MX23 LCDIF (DOTCLK/RGB mode) framebuffer driver.
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

#ifndef IMX23_LCDIF_H
#define IMX23_LCDIF_H

#include "imx23.h"

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
  uint8_t  flags;        /**< IMX23_LCDIF_FLAG_* polarity bits.          */
} imx23_lcdif_timing_t;

#define IMX23_LCDIF_FLAG_HSYNC_HIGH   0x01U
#define IMX23_LCDIF_FLAG_VSYNC_HIGH   0x02U
#define IMX23_LCDIF_FLAG_DE_HIGH      0x04U
#define IMX23_LCDIF_FLAG_PCLK_RISING  0x08U

/**
 * @brief   Initializes the pixel clock and the LCDIF in dotclock mode.
 *
 * @param[in] t     panel timing
 * @param[in] fb    framebuffer address (XRGB8888, width*height*4 bytes,
 *                  32-byte aligned), must stay valid
 *
 * @notapi
 */
void imx23_lcdif_init(const imx23_lcdif_timing_t *t, void *fb);

/**
 * @brief   Returns the framebuffer address programmed into the LCDIF.
 *
 * @notapi
 */
void *imx23_lcdif_get_fb(void);

/**
 * @brief   Points the LCDIF at a new framebuffer (takes effect next frame).
 *
 * @notapi
 */
void imx23_lcdif_set_fb(void *fb);

/**
 * @brief   Registers a VSYNC-edge callback (called from IRQ context on
 *          vector 46, i.e. start of each frame).
 *
 * @notapi
 */
void imx23_lcdif_set_vsync_callback(void (*cb)(void));

#ifdef __cplusplus
}
#endif

#endif /* IMX23_LCDIF_H */

/** @} */
