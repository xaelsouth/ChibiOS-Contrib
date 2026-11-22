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
 * @file    demos/IMX23/RT-IMX23-HELLO/lvgl/imx23_lvgl.h
 * @brief   LVGL display port for the i.MX23 LCDIF + PXP.
 *
 * @details LVGL renders RGB565 into a small partial draw buffer; the flush
 *          callback blits each dirty area into the LCDIF's XRGB8888
 *          framebuffer, either through the PXP pixel pipeline
 *          (RGB565 -> RGB888 mem2mem, hardware) or with a CPU loop.
 *          A dedicated thread runs lv_timer_handler(); the LVGL tick is
 *          driven by lv_tick_set_cb() from the system tick.
 *
 *          This is why the shared lv_conf.h (os/various/lvgl_bindings)
 *          defaults LV_COLOR_FORMAT_DEFAULT to LV_COLOR_FORMAT_RGB565:
 *          the PXP here converts RGB565 to the LCDIF's XRGB8888 framebuffer.
 *
 * @addtogroup LVGL_IMX23
 * @{
 */

#ifndef IMX23_LVGL_H
#define IMX23_LVGL_H

#include "hal.h"
#include "mxs_lcdif.h"
#include "imx23_lradc.h"
#include "lvgl/lvgl.h"

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

#if !defined(IMX23_LVGL_THREAD_STACK) || defined(__DOXYGEN__)
#define IMX23_LVGL_THREAD_STACK     2048U
#endif

#if !defined(IMX23_LVGL_THREAD_PRIO) || defined(__DOXYGEN__)
#define IMX23_LVGL_THREAD_PRIO      (NORMALPRIO + 2U)
#endif

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   LVGL-on-i.MX23 port object (one per display).
 */
typedef struct {
  lv_display_t *disp;                 /* LVGL display object             */
  void         *lcd_fb;               /* XRGB8888 LCDIF framebuffer      */
  uint32_t      hor_res;              /* active width in pixels          */
  uint32_t      ver_res;              /* active height in pixels         */
  bool          use_pxp;              /* flush via PXP, else CPU blit    */
  volatile uint32_t flushes;          /* completed flush callbacks       */
  uint8_t       thd_wa[IMX23_LVGL_THREAD_STACK];  /* handler thread stack */
  /* Optional touch (LRADC 4-wire) pointer input. */
  lv_indev_t   *indev;                /* NULL until imx23_lvgl_add_touch */
  imx23_lradc_ts_t ts;                /* touch wiring + calibration      */
} imx23_lvgl_t;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void imx23_lvgl_init(imx23_lvgl_t *lp, const mxs_lcdif_timing_t *timing,
                       void *lcd_fb, void *draw_buf, uint32_t draw_buf_px,
                       bool use_pxp);
  void imx23_lvgl_start(imx23_lvgl_t *lp);
  void imx23_lvgl_add_touch(imx23_lvgl_t *lp, const imx23_lradc_ts_t *ts);
#ifdef __cplusplus
}
#endif

#endif /* IMX23_LVGL_H */

/** @} */
