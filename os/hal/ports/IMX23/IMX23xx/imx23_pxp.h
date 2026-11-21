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
 * @file    IMX23/IMX23xx/imx23_pxp.h
 * @brief   i.MX23 PXP (pixel pipeline) mem2mem blit/CSC driver.
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

#ifndef IMX23_PXP_H
#define IMX23_PXP_H

#include "imx23.h"

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
  uint8_t     src_fmt;    /**< IMX23_PXP_S0_FMT_*                       */
  void       *dst;        /**< destination buffer (RGB output)          */
  uint32_t    dst_w;      /**< destination width in pixels              */
  uint32_t    dst_h;      /**< destination height in pixels             */
  uint8_t     dst_fmt;    /**< IMX23_PXP_OUT_FMT_*                      */
  uint32_t    crop_x;     /**< crop offset in source, pixels (mult of 8)*/
  uint32_t    crop_y;     /**< crop offset in source, pixels (mult of 8)*/
  uint32_t    crop_w;     /**< crop size in pixels (mult of 8)          */
  uint32_t    crop_h;     /**< crop size in pixels (mult of 8)          */
  bool        hflip;      /**< horizontal flip                          */
  bool        vflip;      /**< vertical flip                            */
  uint8_t     rot90;      /**< rotation in 90-degree steps, 0..3       */
  bool        alpha_out;  /**< output has alpha (ARGB8888)              */
} imx23_pxp_job_t;

/**
 * @brief   Initializes the PXP block (ungate clock, release reset).
 *
 * @notapi
 */
void imx23_pxp_init(void);

/**
 * @brief   Runs one PXP job synchronously (waits for the IRQ).
 *
 * @param[in] job       job descriptor
 * @param[in] timeout   timeout
 * @return              MSG_OK on success, MSG_TIMEOUT/MSG_RESET on error.
 *
 * @notapi
 */
msg_t imx23_pxp_run(const imx23_pxp_job_t *job, sysinterval_t timeout);

/**
 * @brief   Registers a job-done callback (IRQ context, vector 57).
 *
 * @notapi
 */
void imx23_pxp_set_done_callback(void (*cb)(void));

#ifdef __cplusplus
}
#endif

#endif /* IMX23_PXP_H */

/** @} */
