/*
    ChibiOS - Copyright (C) 2026 Xael South

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
 * @file    SPIv1/hal_spi_lld.c
 * @brief   EFR32FG23 SPI subsystem low level driver source (via EUSART).
 *
 * @addtogroup SPI
 * @{
 */

#include "hal.h"

#if (HAL_USE_SPI == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

static EUSART_TypeDef * const efr32_spi_eusart[3] = { EUSART0, EUSART1, EUSART2 };
static const uint32_t efr32_spi_clken[3] = {
  CMU_CLKEN1_EUSART0, CMU_CLKEN1_EUSART1, CMU_CLKEN1_EUSART2
};
static const uint32_t efr32_spi_irqn_rx[3] = {
  EUSART0_RX_IRQn, EUSART1_RX_IRQn, EUSART2_RX_IRQn
};
static const uint32_t efr32_spi_irqn_tx[3] = {
  EUSART0_TX_IRQn, EUSART1_TX_IRQn, EUSART2_TX_IRQn
};
static const uint32_t efr32_spi_irqprio[3] = {
  EFR32_SPI_EUSART1_IRQ_PRIORITY, EFR32_SPI_EUSART2_IRQ_PRIORITY,
  EFR32_SPI_EUSART3_IRQ_PRIORITY
};

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if EFR32_SPI_USE_EUSART1 || defined(__DOXYGEN__)
SPIDriver SPID1;
#endif
#if EFR32_SPI_USE_EUSART2 || defined(__DOXYGEN__)
SPIDriver SPID2;
#endif
#if EFR32_SPI_USE_EUSART3 || defined(__DOXYGEN__)
SPIDriver SPID3;
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static uint32_t spi_lld_clkdiv(uint32_t clock, uint32_t bitrate) {
  uint32_t ovs = 16U; /* Default 16x oversampling for SPI. */
  uint32_t clkdiv = (32U * clock) / (ovs * bitrate) - 32U;
  clkdiv *= 8U;
  return clkdiv;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void spi_lld_init(void) {
#if EFR32_SPI_USE_EUSART1
  spiObjectInit(&SPID1);
  SPID1.eusart = EUSART0; SPID1.index = 0;
#endif
#if EFR32_SPI_USE_EUSART2
  spiObjectInit(&SPID2);
  SPID2.eusart = EUSART1; SPID2.index = 1;
#endif
#if EFR32_SPI_USE_EUSART3
  spiObjectInit(&SPID3);
  SPID3.eusart = EUSART2; SPID3.index = 2;
#endif
}

void spi_lld_start(SPIDriver *spip, const SPIConfig *config) {
  EUSART_TypeDef *u = spip->eusart;
  uint32_t idx = spip->index;
  uint32_t clkdiv;

  if (spip->state == SPI_STOP) {
    CMU->CLKEN1_SET = efr32_spi_clken[idx];

    /* SPI mode: synchronous, MSB first, 8-bit frames. */
    u->CFG0 = config->cfg0 | EUSART_CFG0_SYNC;
    u->FRAMECFG = config->framecfg;

    u->EN_SET = EUSART_EN_EN;

    clkdiv = spi_lld_clkdiv(EFR32_EUSART1CLK, config->bitrate);
    u->CLKDIV = clkdiv;

    u->IF_CLR = u->IF;
    u->CMD_SET = EUSART_CMD_RXEN | EUSART_CMD_TXEN | EUSART_CMD_CLEARTX;
    while ((u->SYNCBUSY & _EUSART_SYNCBUSY_MASK) != 0U) {}

    nvicEnableVector(efr32_spi_irqn_rx[idx], efr32_spi_irqprio[idx]);
    nvicEnableVector(efr32_spi_irqn_tx[idx], efr32_spi_irqprio[idx]);
  }

  spip->config = config;
}

void spi_lld_stop(SPIDriver *spip) {
  EUSART_TypeDef *u = spip->eusart;
  uint32_t idx = spip->index;

  if (spip->state == SPI_READY) {
    u->CMD_CLR = EUSART_CMD_RXEN | EUSART_CMD_TXEN | EUSART_CMD_CLEARTX;
    while ((u->SYNCBUSY & _EUSART_SYNCBUSY_MASK) != 0U) {}
    u->IF_CLR = u->IF;
    u->EN_CLR = EUSART_EN_EN;
    while ((u->EN & EUSART_EN_DISABLING) == EUSART_EN_DISABLING) {}
    nvicDisableVector(efr32_spi_irqn_rx[idx]);
    nvicDisableVector(efr32_spi_irqn_tx[idx]);
    CMU->CLKEN1_CLR = efr32_spi_clken[idx];
  }
}

void spi_lld_select(SPIDriver *spip) {
  palClearPad(spip->cs_port, spip->cs_pad);
}

void spi_lld_unselect(SPIDriver *spip) {
  palSetPad(spip->cs_port, spip->cs_pad);
  if (spip->config->end_cb != NULL) {
    spip->config->end_cb(spip, spip->config->end_cb_param);
  }
}

uint16_t spi_lld_polled_exchange(SPIDriver *spip, uint16_t frame) {
  EUSART_TypeDef *u = spip->eusart;

  u->TXDATA = frame;
  while ((u->IF & EUSART_IF_TXC) == 0U) {}
  u->IF_CLR = EUSART_IF_TXC;

  return (uint16_t)(u->RXDATA & 0xFFFFU);
}

void spi_lld_ignore(SPIDriver *spip, size_t n) {
  while (n-- > 0U) {
    (void)spi_lld_polled_exchange(spip, 0xFFFFU);
  }
}

void spi_lld_exchange(SPIDriver *spip, size_t n, const void *txbuf, void *rxbuf) {
  const uint8_t *tx = txbuf;
  uint8_t *rx = rxbuf;

  while (n-- > 0U) {
    uint16_t frame = tx ? *tx++ : 0xFFU;
    frame = spi_lld_polled_exchange(spip, frame);
    if (rx != NULL) {
      *rx++ = (uint8_t)frame;
    }
  }
}

void spi_lld_send(SPIDriver *spip, size_t n, const void *txbuf) {
  spi_lld_exchange(spip, n, txbuf, NULL);
}

void spi_lld_receive(SPIDriver *spip, size_t n, void *rxbuf) {
  spi_lld_exchange(spip, n, NULL, rxbuf);
}

#endif /* HAL_USE_SPI == TRUE */

/** @} */
