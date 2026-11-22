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
 * @file    devices_lib/display/mxs_pxp.h
 * @brief   MXS-family PXP (i.MX23/i.MX28) (pixel pipeline) mem2mem blit/CSC driver.
 *
 * @details Minimal port of the Linux imx23-pxp.c programming model:
 *          the S0 input plane is converted (optionally CSC for YUV
 *          sources) and written to the RGB output buffer. RGB sources
 *          support crop/flip/rotate at 1:1; YUV sources additionally
 *          support 1/2x..1x scaling. Sizes are rounded to 8x8 blocks.
 *
 * @addtogroup IMX23
 * @{
 */

#ifndef MXS_PXP_H
#define MXS_PXP_H

#include "hal.h"

#include <stdint.h>


#define MXS_PXP_BASE          0x8002A000U
#define MXS_PXP_REG(addr)      (*(volatile uint32_t *)(uintptr_t)(addr))

#define MXS_PXP_CTRL              (MXS_PXP_BASE + 0x000U)
#define MXS_PXP_CTRL_SET          (MXS_PXP_BASE + 0x004U)
#define MXS_PXP_CTRL_CLR          (MXS_PXP_BASE + 0x008U)
#define MXS_PXP_STAT              (MXS_PXP_BASE + 0x010U)
#define MXS_PXP_STAT_CLR          (MXS_PXP_BASE + 0x018U)
#define MXS_PXP_RGBBUF            (MXS_PXP_BASE + 0x020U)
#define MXS_PXP_RGBSIZE           (MXS_PXP_BASE + 0x040U)
#define MXS_PXP_S0BUF             (MXS_PXP_BASE + 0x050U)
#define MXS_PXP_S0UBUF            (MXS_PXP_BASE + 0x060U)
#define MXS_PXP_S0VBUF            (MXS_PXP_BASE + 0x070U)
#define MXS_PXP_S0PARAM           (MXS_PXP_BASE + 0x080U)
#define MXS_PXP_S0BACKGROUND      (MXS_PXP_BASE + 0x090U)
#define MXS_PXP_S0CROP            (MXS_PXP_BASE + 0x0A0U)
#define MXS_PXP_S0SCALE           (MXS_PXP_BASE + 0x0B0U)
#define MXS_PXP_S0OFFSET          (MXS_PXP_BASE + 0x0C0U)
#define MXS_PXP_CSCCOEFF0         (MXS_PXP_BASE + 0x0D0U)
#define MXS_PXP_CSCCOEFF1         (MXS_PXP_BASE + 0x0E0U)
#define MXS_PXP_CSCCOEFF2         (MXS_PXP_BASE + 0x0F0U)
#define MXS_PXP_S0COLORKEYLOW     (MXS_PXP_BASE + 0x180U)
#define MXS_PXP_S0COLORKEYHIGH    (MXS_PXP_BASE + 0x190U)
#define MXS_PXP_VERSION           (MXS_PXP_BASE + 0x1F0U)
#define MXS_PXP_CTRL_SFTRST           (1U << 31)
#define MXS_PXP_CTRL_CLKGATE          (1U << 30)
#define MXS_PXP_CTRL_ALPHA_OUTPUT     (1U << 22)
#define MXS_PXP_CTRL_CROP             (1U << 19)
#define MXS_PXP_CTRL_SCALE            (1U << 18)
#define MXS_PXP_CTRL_S0_FORMAT(v)     (((v) & 0xFU) << 12)
#define MXS_PXP_CTRL_VFLIP            (1U << 11)
#define MXS_PXP_CTRL_HFLIP            (1U << 10)
#define MXS_PXP_CTRL_ROTATE(v)        (((v) & 0x3U) << 8)
#define MXS_PXP_CTRL_OUT_FMT(v)       (((v) & 0xFU) << 4)
#define MXS_PXP_CTRL_IRQ_ENABLE       (1U << 1)
#define MXS_PXP_CTRL_ENABLE           (1U << 0)
#define MXS_PXP_S0_FMT_RGB888         0x1U
#define MXS_PXP_S0_FMT_RGB565         0x4U
#define MXS_PXP_S0_FMT_YUV422         0x8U
#define MXS_PXP_S0_FMT_YUV420         0x9U
#define MXS_PXP_OUT_FMT_ARGB8888      0x0U
#define MXS_PXP_OUT_FMT_RGB888        0x1U
#define MXS_PXP_OUT_FMT_RGB565        0x4U
#define MXS_PXP_STAT_IRQ              (1U << 0)
#define MXS_PXP_STAT_AXI_WRITE_ERROR  (1U << 1)
#define MXS_PXP_STAT_AXI_READ_ERROR   (1U << 2)
#define MXS_PXP_BLOCKS(px)            ((px) >> 3)
#define MXS_PXP_BLOCK_PARAM(x, y, w, h) \
    (((uint32_t)(x) & 0xFFU) << 24 | ((uint32_t)(y) & 0xFFU) << 16 | \
     ((uint32_t)(w) & 0xFFU) << 8  | ((uint32_t)(h) & 0xFFU))
#define MXS_PXP_SCALE_ONE             0x1000U


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   PXP job descriptor.
 */
typedef struct {
  const void *src;        /**< source buffer (S0 plane)                */
  uint32_t    src_w;      /**< source width in pixels                   */
  uint32_t    src_h;      /**< source height in pixels                  */
  uint8_t     src_fmt;    /**< MXS_PXP_S0_FMT_*                       */
  void       *dst;        /**< destination buffer (RGB output)          */
  uint32_t    dst_w;      /**< destination width in pixels              */
  uint32_t    dst_h;      /**< destination height in pixels             */
  uint8_t     dst_fmt;    /**< MXS_PXP_OUT_FMT_*                      */
  uint32_t    crop_x;     /**< crop offset in source, pixels (mult of 8)*/
  uint32_t    crop_y;     /**< crop offset in source, pixels (mult of 8)*/
  uint32_t    crop_w;     /**< crop size in pixels (mult of 8)          */
  uint32_t    crop_h;     /**< crop size in pixels (mult of 8)          */
  bool        hflip;      /**< horizontal flip                          */
  bool        vflip;      /**< vertical flip                            */
  uint8_t     rot90;      /**< rotation in 90-degree steps, 0..3       */
  bool        alpha_out;  /**< output has alpha (ARGB8888)              */
} mxs_pxp_job_t;

/**
 * @brief   Initializes the PXP block (ungate clock, release reset).
 *
 * @notapi
 */
void mxs_pxp_init(void);

/**
 * @brief   Runs one PXP job synchronously (waits for the IRQ).
 *
 * @param[in] job       job descriptor
 * @param[in] timeout   timeout
 * @return              MSG_OK on success, MSG_TIMEOUT/MSG_RESET on error.
 *
 * @notapi
 */
msg_t mxs_pxp_run(const mxs_pxp_job_t *job, sysinterval_t timeout);

/**
 * @brief   Registers a job-done callback (IRQ context, vector 57).
 *
 * @notapi
 */
void mxs_pxp_set_done_callback(void (*cb)(void));

#ifdef __cplusplus
}
#endif

#endif /* MXS_PXP_H */

/** @} */
