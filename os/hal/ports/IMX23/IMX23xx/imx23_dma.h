/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

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
 * @file    IMX23/IMX23xx/imx23_dma.h
 * @brief   i.MX23 APBH/APBX DMA engine support.
 *
 * @details Reworked from U-Boot drivers/dma/apbh_dma.c and Linux
 *          drivers/dma/mxs-dma.c. Transfers are described by CCW
 *          descriptors (imx23_dma_ccw_t) placed in SDRAM; a channel is
 *          kicked by writing its CCW address to NXTCMDAR and incrementing
 *          SEMA. Completion is detected by SEMA returning to zero (polled)
 *          or via the per-channel completion IRQ (IRQONCMPLT).
 *
 * @addtogroup IMX23
 * @{
 */

#ifndef IMX23_DMA_H
#define IMX23_DMA_H

#include "imx23.h"

#ifdef __cplusplus
extern "C" {
#endif
  void imx23_dma_init(void);
  void imx23_dma_reset(unsigned bus, unsigned ch);
  void imx23_dma_start(unsigned bus, unsigned ch, imx23_dma_ccw_t *ccw);
  void imx23_dma_stop(unsigned bus, unsigned ch);
  bool imx23_dma_done(unsigned bus, unsigned ch);
  void imx23_dma_wait(unsigned bus, unsigned ch);
  void imx23_dma_irq_clear(unsigned bus, unsigned ch);
#ifdef __cplusplus
}
#endif

#endif /* IMX23_DMA_H */

/** @} */
