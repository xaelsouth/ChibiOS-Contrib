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
 * @file    IMX23/IMX23xx/hal_spi_lld.c
 * @brief   i.MX23 SPI subsystem low level driver code (SSP, PIO mode).
 *
 * @details Reworked from Linux drivers/spi/spi-mxs.c (IMX23 PIO path) and
 *          U-Boot drivers/spi/mxs_spi.c. 8-bit frames, SPI modes 0..3,
 *          chip select via PAL pad (config->ssport/sspad, active low).
 *
 * @addtogroup IMX23
 * @{
 */

#include "hal.h"

#if (HAL_USE_SPI == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

#define SPI_TIMEOUT             100000U     /* polling loop iterations */

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

#if (IMX23_SPI_USE_SSP0 == TRUE) && !defined(__DOXYGEN__)
SPIDriver SPID1;
#endif
#if (IMX23_SPI_USE_SSP1 == TRUE) && !defined(__DOXYGEN__)
SPIDriver SPID2;
#endif

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

static inline void ssp_wait(volatile uint32_t *reg, uint32_t mask, uint32_t val) {
  uint32_t t = SPI_TIMEOUT;

  while (((*reg) & mask) != val) {
    if (--t == 0U) {
      return;   /* timeout: give up (keeps the driver lock-free) */
    }
  }
}

/* Programs the SSP TIMING register for the requested SCK frequency.
   SCK = ssp_clk / clock_divide / (1 + clock_rate), ported from
   Linux mxs_ssp_set_clk_rate(). */
static void ssp_set_clock(imx23_ssp_t *ssp, uint32_t rate) {
  uint32_t ssp_clk = IMX23_SSP_CLK_HZ;
  uint32_t divide, crate, val;

  if (rate == 0U || rate > ssp_clk / 2U) {
    rate = ssp_clk / 2U;
  }

  for (divide = 2U; divide <= 254U; divide += 2U) {
    crate = (ssp_clk + rate * divide - 1U) / (rate * divide);
    crate = (crate > 0U) ? crate - 1U : 0U;
    if (crate <= 255U) {
      break;
    }
  }
  if (divide > 254U) {
    divide = 254U;
    crate = 255U;
  }

  val = IMX23_REG((uint32_t)(uintptr_t)&ssp->TIMING);
  val &= ~(IMX23_SSP_TIMING_CLOCK_DIVIDE | IMX23_SSP_TIMING_CLOCK_RATE);
  val |= ((divide << 8) & IMX23_SSP_TIMING_CLOCK_DIVIDE) |
         (crate & IMX23_SSP_TIMING_CLOCK_RATE);
  ssp->TIMING = val;
}

/* One byte out, clocked (8-bit frame). */
static void ssp_write_byte(imx23_ssp_t *ssp, uint8_t b) {

  IMX23_REG((uint32_t)(uintptr_t)&ssp->CTRL0 + 8U) = IMX23_SSP_CTRL0_XFER_COUNT_MASK;
  IMX23_REG((uint32_t)(uintptr_t)&ssp->CTRL0 + 4U) = 1U;        /* XFER_COUNT = 1 */
  IMX23_REG((uint32_t)(uintptr_t)&ssp->CTRL0 + 8U) = IMX23_SSP_CTRL0_READ;
  IMX23_REG((uint32_t)(uintptr_t)&ssp->CTRL0 + 4U) = IMX23_SSP_CTRL0_RUN;
  ssp_wait(&ssp->CTRL0, IMX23_SSP_CTRL0_RUN, IMX23_SSP_CTRL0_RUN);
  ssp->DATA = b;
  IMX23_REG((uint32_t)(uintptr_t)&ssp->CTRL0 + 4U) = IMX23_SSP_CTRL0_DATA_XFER;
  ssp_wait(&ssp->CTRL0, IMX23_SSP_CTRL0_RUN, 0U);
}

/* One byte in, clocked (dummy byte out). */
static uint8_t ssp_read_byte(imx23_ssp_t *ssp) {

  IMX23_REG((uint32_t)(uintptr_t)&ssp->CTRL0 + 8U) = IMX23_SSP_CTRL0_XFER_COUNT_MASK;
  IMX23_REG((uint32_t)(uintptr_t)&ssp->CTRL0 + 4U) = 1U;
  IMX23_REG((uint32_t)(uintptr_t)&ssp->CTRL0 + 4U) = IMX23_SSP_CTRL0_READ;
  IMX23_REG((uint32_t)(uintptr_t)&ssp->CTRL0 + 4U) = IMX23_SSP_CTRL0_RUN;
  ssp_wait(&ssp->CTRL0, IMX23_SSP_CTRL0_RUN, IMX23_SSP_CTRL0_RUN);
  IMX23_REG((uint32_t)(uintptr_t)&ssp->CTRL0 + 4U) = IMX23_SSP_CTRL0_DATA_XFER;
  ssp_wait(&ssp->STATUS, IMX23_SSP_STATUS_FIFO_EMPTY, 0U);
  {
    uint8_t b = (uint8_t)(ssp->DATA & 0xFFU);
    ssp_wait(&ssp->CTRL0, IMX23_SSP_CTRL0_RUN, 0U);
    return b;
  }
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level SPI driver initialization.
 *
 * @notapi
 */
void spi_lld_init(void) {

#if (IMX23_SPI_USE_SSP0 == TRUE) && !defined(__DOXYGEN__)
  spiObjectInit(&SPID1);
  SPID1.ssp = IMX23_SSP0;
#endif
#if (IMX23_SPI_USE_SSP1 == TRUE) && !defined(__DOXYGEN__)
  spiObjectInit(&SPID2);
  SPID2.ssp = IMX23_SSP1;
#endif
}

/**
 * @brief   Configures and activates the SPI peripheral.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_start(SPIDriver *spip) {
  imx23_ssp_t *ssp = spip->ssp;
  uint32_t mode = spip->config->mode & 3U;
  uint32_t ctrl1;

  /* SSP clock: 24MHz XTAL / 1, ungated. */
  IMX23_REG(IMX23_CLKCTRL_CLKSEQ) |= IMX23_CLKCTRL_CLKSEQ_SSP_SEL_XTAL;
  IMX23_REG(IMX23_CLKCTRL_CLKSEQ) &= ~IMX23_CLKCTRL_CLKSEQ_BYPASS_SSP0;
  IMX23_REG(IMX23_CLKCTRL_SSP0) = 1U;   /* divider 1 */
  IMX23_REG_CLR(IMX23_CLKCTRL_SSP0, IMX23_CLKCTRL_SSP_CLKGATE);

  /* Soft reset the SSP, then ungate. */
  ssp->CTRL0 = IMX23_SSP_CTRL0_SFTRST;
  while ((ssp->CTRL0 & IMX23_SSP_CTRL0_SFTRST) != 0U) {
  }
  IMX23_REG((uint32_t)(uintptr_t)&ssp->CTRL0 + 8U) = IMX23_SSP_CTRL0_CLKGATE;

  ssp->CMD0  = 0U;
  ssp->CMD1  = 0U;
  ssp->TIMING = IMX23_SSP_TIMING_TIMEOUT_MASK;   /* timeout + clock below */
  ssp_set_clock(ssp, spip->config->clock);

  ctrl1 = IMX23_SSP_CTRL1_WORD_LENGTH_8 | IMX23_SSP_CTRL1_SSP_MODE_SPI;
  if (mode & 1U) ctrl1 |= IMX23_SSP_CTRL1_PHASE;
  if (mode & 2U) ctrl1 |= IMX23_SSP_CTRL1_POLARITY;
  ssp->CTRL1 = ctrl1;

  ssp->CTRL0 = IMX23_SSP_CTRL0_ENABLE | IMX23_SSP_CTRL0_BUS_WIDTH_8BIT |
               IMX23_SSP_CTRL0_IGNORE_CRC;

  /* Chip select idle high. */
  palSetPadMode(spip->config->ssport, spip->config->sspad,
                PAL_MODE_OUTPUT_PUSHPULL);
  palSetPad(spip->config->ssport, spip->config->sspad);
}

/**
 * @brief   Deactivates the SPI peripheral.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_stop(SPIDriver *spip) {
  imx23_ssp_t *ssp = spip->ssp;

  if (spip->state == SPI_READY) {
    spip->state = SPI_STOP;
    ssp->CTRL0 = 0U;
    ssp->CTRL1 = 0U;
    IMX23_REG((uint32_t)(uintptr_t)&ssp->CTRL0 + 4U) = IMX23_SSP_CTRL0_SFTRST;
  }
}

/**
 * @brief   Asserts the chip select (active low).
 *
 * @notapi
 */
void spi_lld_select(SPIDriver *spip) {

  palClearPad(spip->config->ssport, spip->config->sspad);
}

/**
 * @brief   Deasserts the chip select.
 *
 * @notapi
 */
void spi_lld_unselect(SPIDriver *spip) {

  palSetPad(spip->config->ssport, spip->config->sspad);
}

/**
 * @brief   Ignores (clocks out and discards) n frames.
 *
 * @notapi
 */
void spi_lld_ignore(SPIDriver *spip, size_t n) {

  while (n-- > 0U) {
    (void)ssp_read_byte(spip->ssp);
  }
}

/**
 * @brief   Exchanges n frames (full duplex, byte per frame).
 *
 * @notapi
 */
void spi_lld_exchange(SPIDriver *spip, size_t n, const void *txbuf, void *rxbuf) {
  const uint8_t *tx = (const uint8_t *)txbuf;
  uint8_t *rx = (uint8_t *)rxbuf;

  while (n-- > 0U) {
    uint8_t b = (tx != NULL) ? *tx++ : 0xFFU;
    ssp_write_byte(spip->ssp, b);
    if (rx != NULL) {
      *rx++ = ssp_read_byte(spip->ssp);
    }
    else {
      (void)ssp_read_byte(spip->ssp);
    }
  }
}

/**
 * @brief   Sends n frames.
 *
 * @notapi
 */
void spi_lld_send(SPIDriver *spip, size_t n, const void *txbuf) {
  const uint8_t *tx = (const uint8_t *)txbuf;

  while (n-- > 0U) {
    ssp_write_byte(spip->ssp, *tx++);
  }
}

/**
 * @brief   Receives n frames.
 *
 * @notapi
 */
void spi_lld_receive(SPIDriver *spip, size_t n, void *rxbuf) {
  uint8_t *rx = (uint8_t *)rxbuf;

  while (n-- > 0U) {
    *rx++ = ssp_read_byte(spip->ssp);
  }
}

/**
 * @brief   Aborts the ongoing transfer (not supported, no-op).
 *
 * @notapi
 */
void spi_lld_abort(SPIDriver *spip) {

  (void)spip;
}

/**
 * @brief   Exchanges one frame, polled.
 *
 * @notapi
 */
uint16_t spi_lld_polled_exchange(SPIDriver *spip, uint16_t frame) {

  ssp_write_byte(spip->ssp, (uint8_t)(frame & 0xFFU));
  return ssp_read_byte(spip->ssp);
}

#endif /* HAL_USE_SPI == TRUE */

/** @} */
