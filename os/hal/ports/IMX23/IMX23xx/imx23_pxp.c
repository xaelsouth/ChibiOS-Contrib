/*
    ChibiOS - Copyright (C) 2026 Giovanni Di Sirio.

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
 * @file    IMX23/IMX23xx/imx23_pxp.c
 * @brief   i.MX23 PXP mem2mem blit/CSC driver (ported from Linux
 *          drivers/media/platform/nxp/imx23-pxp.c).
 *
 * @addtogroup IMX23
 * @{
 */

#include "hal.h"
#include "icoll.h"
#include "imx23_pxp.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

#define IMX23_REG32(addr)   (*(volatile uint32_t *)(addr))

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

static void (*done_cb)(void);
static semaphore_t done_sem;
static volatile bool done_flag;

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/* IRQ handler for PXP (ICOLL vector 57). */
static bool pxp_irq_handler(void) {
  uint32_t stat = IMX23_REG32(IMX23_PXP_STAT);

  /* Acknowledge all IRQ sources. */
  IMX23_REG32(IMX23_PXP_STAT_CLR) = stat & 0x7U;

  if ((stat & IMX23_PXP_STAT_IRQ) != 0U) {
    done_flag = true;
    osalSysLockFromISR();
    osalThreadResumeI(&done_sem, MSG_OK);
    osalSysUnlockFromISR();
    if (done_cb != NULL) {
      done_cb();
    }
  }

  return false;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

void imx23_pxp_init(void) {

  chSemObjectInit(&done_sem, 0);
  done_cb = NULL;
  done_flag = false;

  /* Ungate clock, release reset. */
  IMX23_REG32(IMX23_PXP_CTRL_CLR) = IMX23_PXP_CTRL_SFTRST |
                                     IMX23_PXP_CTRL_CLKGATE;

  icoll_register_handler(IMX23_IRQ_PXP, pxp_irq_handler);
  icoll_irq_enable(IMX23_IRQ_PXP);
}

msg_t imx23_pxp_run(const imx23_pxp_job_t *job, sysinterval_t timeout) {
  uint32_t ctrl;
  bool yuv = (job->src_fmt == IMX23_PXP_S0_FMT_YUV422) ||
             (job->src_fmt == IMX23_PXP_S0_FMT_YUV420);
  /* Rotation swaps the output dimensions. */
  bool swap = ((job->rot90 & 1U) != 0U);
  uint32_t ow = swap ? job->dst_h : job->dst_w;
  uint32_t oh = swap ? job->dst_w : job->dst_h;
  uint32_t xscale = IMX23_PXP_SCALE_ONE, yscale = IMX23_PXP_SCALE_ONE;
  bool scale = false;
  uint32_t luma;
  msg_t msg;

  osalDbgCheck(job != NULL);

  /* Scaling only applies to YUV sources, 1/2x min (2.12 fixed point). */
  if (yuv && ((ow != job->src_w) || (oh != job->src_h))) {
    scale = true;
    xscale = (job->src_w << 12) / ow;
    yscale = (job->src_h << 12) / oh;
    if (xscale > 0x2000U) xscale = 0x2000U;
    if (yscale > 0x2000U) yscale = 0x2000U;
    if (xscale < 0x1000U) xscale = 0x1000U;   /* min 1/2x downscale */
    if (yscale < 0x1000U) yscale = 0x1000U;
  }

  ctrl = IMX23_PXP_CTRL_IRQ_ENABLE | IMX23_PXP_CTRL_ENABLE |
         IMX23_PXP_CTRL_S0_FORMAT(job->src_fmt) |
         IMX23_PXP_CTRL_OUT_FMT(job->dst_fmt) |
         IMX23_PXP_CTRL_ROTATE(job->rot90);
  if (job->hflip)   ctrl |= IMX23_PXP_CTRL_HFLIP;
  if (job->vflip)   ctrl |= IMX23_PXP_CTRL_VFLIP;
  if (job->alpha_out) ctrl |= IMX23_PXP_CTRL_ALPHA_OUTPUT;
  if (scale || (job->crop_w != 0U)) ctrl |= IMX23_PXP_CTRL_CROP;
  if (scale) ctrl |= IMX23_PXP_CTRL_SCALE;

  osalSysLock();
  done_flag = false;

  /* Make sure the block is neither in reset nor clock gated. */
  IMX23_REG32(IMX23_PXP_CTRL_CLR) = IMX23_PXP_CTRL_SFTRST |
                                     IMX23_PXP_CTRL_CLKGATE;

  IMX23_REG32(IMX23_PXP_S0BUF) = (uint32_t)(uintptr_t)job->src;
  if (yuv) {
    /* BT.601 limited range YUV -> RGB, per the reference manual. */
    luma = job->src_w * job->src_h;
    IMX23_REG32(IMX23_PXP_S0UBUF) = (uint32_t)(uintptr_t)job->src + luma;
    IMX23_REG32(IMX23_PXP_S0VBUF) = (uint32_t)(uintptr_t)job->src + luma +
                                    luma / ((job->src_fmt == IMX23_PXP_S0_FMT_YUV420) ? 4U : 2U);
    /* CSC0(c0, uv_off, y_off), CSC1(c1, c4), CSC2(c2, c3). */
    IMX23_REG32(IMX23_PXP_CSCCOEFF0) = (1U << 31) | (0x12AU << 18) |
                                       (0x180U << 9) | 0x1F0U;
    IMX23_REG32(IMX23_PXP_CSCCOEFF1) = (0x198U << 16) | 0x204U;
    IMX23_REG32(IMX23_PXP_CSCCOEFF2) = (0x730U << 16) | 0x79CU;
  }

  IMX23_REG32(IMX23_PXP_S0PARAM) =
      IMX23_PXP_BLOCK_PARAM(0, 0, IMX23_PXP_BLOCKS(job->src_w),
                            IMX23_PXP_BLOCKS(job->src_h));
  IMX23_REG32(IMX23_PXP_S0BACKGROUND) = 0U;
  IMX23_REG32(IMX23_PXP_S0CROP) =
      IMX23_PXP_BLOCK_PARAM(IMX23_PXP_BLOCKS(job->crop_x),
                            IMX23_PXP_BLOCKS(job->crop_y),
                            IMX23_PXP_BLOCKS(job->crop_w ? job->crop_w : ow),
                            IMX23_PXP_BLOCKS(job->crop_h ? job->crop_h : oh));
  IMX23_REG32(IMX23_PXP_S0SCALE) = (yscale << 16) | xscale;
  IMX23_REG32(IMX23_PXP_S0OFFSET) = 0U;

  /* Color keying disabled (low > high). */
  IMX23_REG32(IMX23_PXP_S0COLORKEYLOW)  = 0xFFFFFFU;
  IMX23_REG32(IMX23_PXP_S0COLORKEYHIGH) = 0U;

  IMX23_REG32(IMX23_PXP_RGBBUF) = (uint32_t)(uintptr_t)job->dst;
  IMX23_REG32(IMX23_PXP_RGBSIZE) = ((uint32_t)(job->alpha_out ? 1U : 0U) << 24) |
                                   (ow << 12) | oh;

  /* GO. */
  IMX23_REG32(IMX23_PXP_CTRL) = ctrl;

  msg = osalThreadSuspendTimeoutS(&done_sem, timeout);
  if (msg != MSG_OK) {
    /* Timed out or reset: stop the engine. */
    IMX23_REG32(IMX23_PXP_CTRL_CLR) = IMX23_PXP_CTRL_ENABLE;
    msg = (msg == MSG_TIMEOUT) ? MSG_TIMEOUT : MSG_RESET;
  }
  osalSysUnlock();

  /* Gate the block clock while idle. */
  IMX23_REG32(IMX23_PXP_CTRL_SET) = IMX23_PXP_CTRL_CLKGATE;

  return msg;
}

void imx23_pxp_set_done_callback(void (*cb)(void)) {

  osalSysLock();
  done_cb = cb;
  osalSysUnlock();
}

/** @} */
