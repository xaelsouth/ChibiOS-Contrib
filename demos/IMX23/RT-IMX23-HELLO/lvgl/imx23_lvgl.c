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
 * @file    demos/IMX23/RT-IMX23-HELLO/lvgl/imx23_lvgl.c
 * @brief   LVGL display port for the i.MX23 LCDIF + PXP.
 *
 * @addtogroup LVGL_IMX23
 * @{
 */

#include "hal.h"

#include <string.h>

#include "mxs_lcdif.h"
#include "mxs_pxp.h"
#include "imx23_lradc.h"
#include "imx23_lvgl.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

#define LVGL_POLL_MS        5U      /* lv_timer_handler() period           */

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/* LVGL tick callback: milliseconds from the system tick. */
static uint32_t lvgl_tick_cb(void) {
  systime_t t = chVTGetSystemTimeX();

  return (uint32_t)(((uint64_t)t * 1000U) / CH_CFG_ST_FREQUENCY);
}

/* Flush one dirty area from the LVGL draw buffer into the LCDIF fb. */
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area,
                          uint8_t *px_map) {
  imx23_lvgl_t *lp = (imx23_lvgl_t *)lv_display_get_user_data(disp);
  int32_t x = area->x1;
  int32_t y = area->y1;
  uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
  uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);

  if (lp->use_pxp) {
    /* Hardware path: PXP mem2mem RGB565 -> XRGB8888 (RGB888) at offset. */
    mxs_pxp_job_t job;

    memset(&job, 0, sizeof(job));
    job.src     = px_map;
    job.src_w   = w;
    job.src_h   = h;
    job.src_fmt = MXS_PXP_S0_FMT_RGB565;
    job.dst     = (uint32_t *)lp->lcd_fb + (uint32_t)y * lp->hor_res +
                  (uint32_t)x;
    job.dst_w   = w;
    job.dst_h   = h;
    job.dst_fmt = MXS_PXP_OUT_FMT_RGB888;
    (void)mxs_pxp_run(&job, TIME_MS2I(500));
  }
  else {
    /* CPU path: RGB565 -> XRGB8888 expansion blit. */
    const uint16_t *src = (const uint16_t *)px_map;
    uint32_t *dst = (uint32_t *)lp->lcd_fb + (uint32_t)y * lp->hor_res +
                    (uint32_t)x;
    uint32_t r, c;

    for (r = 0U; r < h; r++) {
      for (c = 0U; c < w; c++) {
        uint16_t px = src[r * w + c];
        dst[r * lp->hor_res + c] = 0xFF000000U |
          ((uint32_t)((px >> 11) & 0x1FU) << 19) |   /* R5 -> R8 */
          ((uint32_t)((px >> 5) & 0x3FU) << 10) |     /* G6 -> G8 */
          ((uint32_t)(px & 0x1FU) << 3);             /* B5 -> B8 */
      }
    }
  }

  lp->flushes++;
  lv_display_flush_ready(disp);
}

/* Touch (LRADC 4-wire) pointer input read callback, runs on the LVGL
   handler thread. Maps raw 18-bit ADC values to screen coordinates. */
static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
  imx23_lvgl_t *lp = (imx23_lvgl_t *)lv_indev_get_user_data(indev);
  uint16_t xr, yr;
  int32_t v;

  if (imx23_lradc_touch_read(&lp->ts, &xr, &yr) != 0) {
    data->state = LV_INDEV_STATE_RELEASED;
    return;
  }
  data->state = LV_INDEV_STATE_PRESSED;
  v = (int32_t)xr - lp->ts.raw_min;
  data->point.x = (lv_coord_t)((v * (int32_t)(lp->hor_res - 1U)) /
                               (int32_t)(lp->ts.raw_max - lp->ts.raw_min));
  v = (int32_t)yr - lp->ts.raw_min;
  data->point.y = (lv_coord_t)((v * (int32_t)(lp->ver_res - 1U)) /
                               (int32_t)(lp->ts.raw_max - lp->ts.raw_min));
}

/* LVGL handler thread. */
static THD_FUNCTION(imx23_lvgl_thread, arg) {
  imx23_lvgl_t *lp = (imx23_lvgl_t *)arg;

  chRegSetThreadName("lvgl");
  (void)lp;
  while (true) {
    lv_timer_handler();
    chThdSleepMilliseconds(LVGL_POLL_MS);
  }
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

void imx23_lvgl_init(imx23_lvgl_t *lp, const mxs_lcdif_timing_t *timing,
                     void *lcd_fb, void *draw_buf, uint32_t draw_buf_px,
                     bool use_pxp) {
  static bool lv_ready;

  memset(lp, 0, sizeof(*lp));
  lp->lcd_fb  = lcd_fb;
  lp->hor_res = timing->width;
  lp->ver_res = timing->height;
  lp->use_pxp = use_pxp;

  if (!lv_ready) {
    lv_init();
    lv_ready = true;
  }

  lp->disp = lv_display_create((int32_t)lp->hor_res, (int32_t)lp->ver_res);
  lv_display_set_color_format(lp->disp, LV_COLOR_FORMAT_RGB565);
  /* Single partial draw buffer, draw_buf_px pixels of RGB565. */
  lv_display_set_buffers(lp->disp, draw_buf, NULL, draw_buf_px * 2U,
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(lp->disp, lvgl_flush_cb);
  lv_display_set_user_data(lp->disp, lp);

  /* Hardware: PXP for the flush blits, LCDIF in dotclock master mode. */
  if (use_pxp) {
    mxs_pxp_init();
  }
  mxs_lcdif_init(timing, lcd_fb);
}

void imx23_lvgl_start(imx23_lvgl_t *lp) {

  lv_tick_set_cb(lvgl_tick_cb);
  chThdCreateStatic(lp->thd_wa, sizeof(lp->thd_wa), IMX23_LVGL_THREAD_PRIO,
                    imx23_lvgl_thread, lp);
}

void imx23_lvgl_add_touch(imx23_lvgl_t *lp, const imx23_lradc_ts_t *ts) {

  imx23_lradc_init();
  lp->ts = *ts;
  lp->indev = lv_indev_create();
  lv_indev_set_type(lp->indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(lp->indev, touch_read_cb);
  lv_indev_set_user_data(lp->indev, lp);
  lv_indev_set_display(lp->indev, lp->disp);
}

/** @} */
