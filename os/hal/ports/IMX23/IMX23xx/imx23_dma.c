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

/**
 * @file    IMX23/IMX23xx/imx23_dma.c
 * @brief   i.MX23 APBH/APBX DMA engine support code.
 *
 * @addtogroup IMX23
 * @{
 */

#include "hal.h"
#include "imx23_dma.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

static const uint32_t dma_ctrl0[2] = { IMX23_APBH_CTRL0, IMX23_APBX_CTRL0 };
static const uint32_t dma_ctrl1[2] = { IMX23_APBH_CTRL1, IMX23_APBX_CTRL1 };
static const uint32_t dma_ctrl2[2] = { IMX23_APBH_CTRL2, IMX23_APBX_CTRL2 };

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

static inline unsigned dma_channels(unsigned bus) {

  return (bus == IMX23_DMA_APBH) ? IMX23_DMA_APBH_CHANNELS
                                 : IMX23_DMA_APBX_CHANNELS;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   DMA engines initialization.
 * @note    Called from hal_lld_init().
 *
 * @notapi
 */
void imx23_dma_init(void) {
  unsigned bus, ch;

  for (bus = 0U; bus < 2U; bus++) {
    /* Soft reset, then ungate. */
    IMX23_REG(dma_ctrl0[bus]) = IMX23_DMA_CTRL0_SFTRST;
    while ((IMX23_REG(dma_ctrl0[bus]) & IMX23_DMA_CTRL0_SFTRST) != 0U) {
    }
    IMX23_REG_CLR(dma_ctrl0[bus], IMX23_DMA_CTRL0_CLKGATE);

    for (ch = 0U; ch < dma_channels(bus); ch++) {
      IMX23_REG(IMX23_DMA_CH_NXTCMDAR(bus, ch)) = 0U;
      IMX23_REG(IMX23_DMA_CH_CMD(bus, ch)) = 0U;
      IMX23_REG(IMX23_DMA_CH_SEMA(bus, ch)) = 0U;
      /* Reset the channel. */
      IMX23_REG_SET(dma_ctrl0[bus], IMX23_DMA_CTRL0_RESET_CHANNEL(ch));
      IMX23_REG_CLR(dma_ctrl0[bus], IMX23_DMA_CTRL0_RESET_CHANNEL(ch));
    }
    /* Clear any pending completion/error IRQs. */
    IMX23_REG(dma_ctrl1[bus]) = 0xFFFFFFFFU;
    IMX23_REG(dma_ctrl2[bus]) = 0xFFFFFFFFU;
  }
}

/**
 * @brief   Resets a DMA channel.
 *
 * @notapi
 */
void imx23_dma_reset(unsigned bus, unsigned ch) {

  IMX23_REG(IMX23_DMA_CH_NXTCMDAR(bus, ch)) = 0U;
  IMX23_REG(IMX23_DMA_CH_CMD(bus, ch)) = 0U;
  IMX23_REG(IMX23_DMA_CH_SEMA(bus, ch)) = 0U;
  IMX23_REG_SET(dma_ctrl0[bus], IMX23_DMA_CTRL0_RESET_CHANNEL(ch));
  IMX23_REG_CLR(dma_ctrl0[bus], IMX23_DMA_CTRL0_RESET_CHANNEL(ch));
}

/**
 * @brief   Starts a transfer on a channel.
 *
 * @param[in] bus       IMX23_DMA_APBH or IMX23_DMA_APBX
 * @param[in] ch        channel number
 * @param[in] ccw       first CCW descriptor (in SDRAM)
 *
 * @notapi
 */
void imx23_dma_start(unsigned bus, unsigned ch, imx23_dma_ccw_t *ccw) {

  IMX23_REG(IMX23_DMA_CH_NXTCMDAR(bus, ch)) = (uint32_t)(uintptr_t)ccw;
  /* Kick the channel: one semaphore increment per pending command. */
  IMX23_REG(IMX23_DMA_CH_SEMA(bus, ch) + 4U) = 1U;   /* SEMA_SET */
}

/**
 * @brief   Stops a channel (aborts pending commands).
 *
 * @notapi
 */
void imx23_dma_stop(unsigned bus, unsigned ch) {

  IMX23_REG(IMX23_DMA_CH_NXTCMDAR(bus, ch)) = 0U;
  IMX23_REG(IMX23_DMA_CH_CMD(bus, ch)) = 0U;
  IMX23_REG(IMX23_DMA_CH_SEMA(bus, ch)) = 0U;
}

/**
 * @brief   Returns true when the channel has no pending commands.
 *
 * @notapi
 */
bool imx23_dma_done(unsigned bus, unsigned ch) {

  return (IMX23_REG(IMX23_DMA_CH_SEMA(bus, ch)) & 0xFFU) == 0U;
}

/**
 * @brief   Busy-waits for channel completion.
 *
 * @notapi
 */
void imx23_dma_wait(unsigned bus, unsigned ch) {

  while (!imx23_dma_done(bus, ch)) {
  }
}

/**
 * @brief   Clears the completion/error IRQ flags of a channel.
 *
 * @notapi
 */
void imx23_dma_irq_clear(unsigned bus, unsigned ch) {

  IMX23_REG(dma_ctrl1[bus] + 4U) = 1U << ch;   /* CTRL1_SET? no: write raw */
  IMX23_REG(dma_ctrl1[bus]) = 1U << ch;
  IMX23_REG(dma_ctrl2[bus]) = 1U << ch;
}

/** @} */
