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

#if !defined(EFR32_DMA_REQUIRED)
#define EFR32_DMA_REQUIRED
#endif

static EUSART_TypeDef * const efr32_spi_eusart[3] = { EUSART0, EUSART1, EUSART2 };
static const uint32_t efr32_spi_clken[3] = {
  CMU_CLKEN1_EUSART0, CMU_CLKEN1_EUSART1, CMU_CLKEN1_EUSART2
};
static const uint32_t efr32_spi_clk[3] = {
  EFR32_EUSART1CLK, EFR32_EUSART23CLK, EFR32_EUSART23CLK
};

/* DMA request source (SOURCESEL field) per EUSART index. */
static const uint32_t efr32_spi_dma_src[3] = {
  LDMAXBAR_CH_REQSEL_SOURCESEL_EUSART0 >> 16,
  LDMAXBAR_CH_REQSEL_SOURCESEL_EUSART1 >> 16,
  LDMAXBAR_CH_REQSEL_SOURCESEL_EUSART2 >> 16
};

/* SIGSEL for TX (TXFL=1) and RX (RXFL=0). */
#define EFR32_SPI_DMA_SIG_TX   1U
#define EFR32_SPI_DMA_SIG_RX   0U

/* DMA stream config per instance. */
static const int32_t efr32_spi_tx_dma_stream[3] = {
  EFR32_SPI_EUSART1_TX_DMA_STREAM,
  EFR32_SPI_EUSART2_TX_DMA_STREAM,
  EFR32_SPI_EUSART3_TX_DMA_STREAM
};
static const int32_t efr32_spi_rx_dma_stream[3] = {
  EFR32_SPI_EUSART1_RX_DMA_STREAM,
  EFR32_SPI_EUSART2_RX_DMA_STREAM,
  EFR32_SPI_EUSART3_RX_DMA_STREAM
};
static const uint32_t efr32_spi_dma_prio[3] = {
  EFR32_SPI_EUSART1_DMA_IRQ_PRIORITY,
  EFR32_SPI_EUSART2_DMA_IRQ_PRIORITY,
  EFR32_SPI_EUSART3_DMA_IRQ_PRIORITY
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

/* Sync-mode divider: goes into CFG2.SDIV, spi_clk = clock / (sdiv + 1). */
static uint32_t spi_lld_sdiv(uint32_t clock, uint32_t bitrate) {
  uint32_t sdiv;
  if (bitrate == 0U || bitrate > clock) {
    sdiv = 0U;
  } else {
    sdiv = clock / bitrate - 1U;
  }
  if (sdiv > (_EUSART_CFG2_SDIV_MASK >> _EUSART_CFG2_SDIV_SHIFT)) {
    sdiv = (_EUSART_CFG2_SDIV_MASK >> _EUSART_CFG2_SDIV_SHIFT);
  }
  return sdiv;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void spi_lld_init(void) {
#if EFR32_SPI_USE_EUSART1
  spiObjectInit(&SPID1);
  SPID1.eusart = EUSART0; SPID1.index = 0;
  SPID1.dma_tx = NULL; SPID1.dma_rx = NULL;
#endif
#if EFR32_SPI_USE_EUSART2
  spiObjectInit(&SPID2);
  SPID2.eusart = EUSART1; SPID2.index = 1;
  SPID2.dma_tx = NULL; SPID2.dma_rx = NULL;
#endif
#if EFR32_SPI_USE_EUSART3
  spiObjectInit(&SPID3);
  SPID3.eusart = EUSART2; SPID3.index = 2;
  SPID3.dma_tx = NULL; SPID3.dma_rx = NULL;
#endif
}

void spi_lld_start(SPIDriver *spip) {
  EUSART_TypeDef *u = spip->eusart;
  uint32_t idx = spip->index;
  const SPIConfig *cfg = spip->config;
  uint32_t clkdiv;

  if (spip->state == SPI_STOP) {
    CMU->CLKEN1_SET = efr32_spi_clken[idx];

    /* SPI mode: synchronous master, MSB first. The sync divider lives in
       CFG2.SDIV (spi_clk = eusart_clk / (SDIV+1)), MASTER enables the
       internal clock generator. CFG regs must be written while EN=0. */
    clkdiv = spi_lld_sdiv(efr32_spi_clk[idx], cfg->bitrate);
    u->CFG2 = (u->CFG2 & ~(_EUSART_CFG2_MASK)) |
              _EUSART_CFG2_MASTER_MASTER | EUSART_CFG2_FORCELOAD |
              (clkdiv << _EUSART_CFG2_SDIV_SHIFT);
    u->CFG0 = cfg->cfg0 | EUSART_CFG0_SYNC | EUSART_CFG0_MSBF;
    u->FRAMECFG = cfg->framecfg;

    u->EN_SET = EUSART_EN_EN;

    u->IF_CLR = u->IF;
    u->CMD_SET = EUSART_CMD_RXEN | EUSART_CMD_TXEN | EUSART_CMD_CLEARTX;
    while ((u->SYNCBUSY & _EUSART_SYNCBUSY_MASK) != 0U) {}

    /* Allocate DMA streams if configured. */
#if defined(EFR32_DMA_REQUIRED)
    if (efr32_spi_tx_dma_stream[idx] >= 0 && spip->dma_tx == NULL) {
      spip->dma_tx = dmaStreamAlloc((uint32_t)efr32_spi_tx_dma_stream[idx],
                                     efr32_spi_dma_prio[idx], NULL, NULL);
    }
    if (efr32_spi_rx_dma_stream[idx] >= 0 && spip->dma_rx == NULL) {
      spip->dma_rx = dmaStreamAlloc((uint32_t)efr32_spi_rx_dma_stream[idx],
                                     efr32_spi_dma_prio[idx], NULL, NULL);
    }
#endif
  }
}

void spi_lld_stop(SPIDriver *spip) {
  EUSART_TypeDef *u = spip->eusart;
  uint32_t idx = spip->index;

  if (spip->state == SPI_READY) {
#if defined(EFR32_DMA_REQUIRED)
    if (spip->dma_tx != NULL) {
      dmaStreamFree(spip->dma_tx);
      spip->dma_tx = NULL;
    }
    if (spip->dma_rx != NULL) {
      dmaStreamFree(spip->dma_rx);
      spip->dma_rx = NULL;
    }
#endif
    u->CMD_CLR = EUSART_CMD_RXEN | EUSART_CMD_TXEN | EUSART_CMD_CLEARTX;
    while ((u->SYNCBUSY & _EUSART_SYNCBUSY_MASK) != 0U) {}
    u->IF_CLR = u->IF;
    u->EN_CLR = EUSART_EN_EN;
    while ((u->EN & EUSART_EN_DISABLING) == EUSART_EN_DISABLING) {}
    CMU->CLKEN1_CLR = efr32_spi_clken[idx];
  }
}

void spi_lld_select(SPIDriver *spip) {
#if (SPI_SELECT_MODE == SPI_SELECT_MODE_PAD) || defined(__DOXYGEN__)
  palClearPad(spip->config->ssport, spip->config->sspad);
#elif (SPI_SELECT_MODE == SPI_SELECT_MODE_LINE) || defined(__DOXYGEN__)
  palClearLine(spip->config->ssline);
#elif (SPI_SELECT_MODE == SPI_SELECT_MODE_PORT) || defined(__DOXYGEN__)
  palClearPort(spip->config->ssport, spip->config->ssmask);
#endif
}

void spi_lld_unselect(SPIDriver *spip) {
#if (SPI_SELECT_MODE == SPI_SELECT_MODE_PAD) || defined(__DOXYGEN__)
  palSetPad(spip->config->ssport, spip->config->sspad);
#elif (SPI_SELECT_MODE == SPI_SELECT_MODE_LINE) || defined(__DOXYGEN__)
  palSetLine(spip->config->ssline);
#elif (SPI_SELECT_MODE == SPI_SELECT_MODE_PORT) || defined(__DOXYGEN__)
  palSetPort(spip->config->ssport, spip->config->ssmask);
#endif
  if (spip->config->end_cb != NULL) {
    spip->config->end_cb(spip);
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

#if defined(EFR32_DMA_REQUIRED) || defined(__DOXYGEN__)
/* DMA variants are defined below; forward declarations for dispatch. */
void spi_lld_exchange_dma(SPIDriver *spip, size_t n, const void *txbuf, void *rxbuf);
void spi_lld_send_dma(SPIDriver *spip, size_t n, const void *txbuf);
void spi_lld_receive_dma(SPIDriver *spip, size_t n, void *rxbuf);

/**
 * @brief   True if the DMA path can be used for this transfer.
 */
static bool spi_lld_dma_usable(SPIDriver *spip, size_t n, bool need_tx, bool need_rx) {

  if (n < (size_t)EFR32_SPI_DMA_MIN_TRANSFER) {
    return false;
  }
  if (need_tx && (spip->dma_tx == NULL)) {
    return false;
  }
  if (need_rx && (spip->dma_rx == NULL)) {
    return false;
  }
  return true;
}
#endif /* EFR32_DMA_REQUIRED */

/**
 * @brief   Polled (non-DMA) exchange loop.
 */
static void spi_lld_exchange_polled(SPIDriver *spip, size_t n,
                                    const void *txbuf, void *rxbuf) {
  const uint8_t *tx = txbuf;
  uint8_t *rx = rxbuf;

  while (n-- > 0U) {
    uint16_t frame = tx ? *tx++ : 0xFFU;
    frame = spi_lld_polled_exchange(spip, frame);
    if (rx != NULL) {
      *rx++ = (uint8_t)frame;
    }
  }

  /* Polled transfer is complete: restore READY so that spiSynchronizeS()
     does not block waiting for an interrupt that never comes. */
  spip->state = SPI_READY;
}

void spi_lld_exchange(SPIDriver *spip, size_t n, const void *txbuf, void *rxbuf) {
#if defined(EFR32_DMA_REQUIRED) || defined(__DOXYGEN__)
  if (spi_lld_dma_usable(spip, n, true, false) &&
      (rxbuf == NULL || spip->dma_rx != NULL)) {
    spi_lld_exchange_dma(spip, n, txbuf, rxbuf);
    return;
  }
#endif
  spi_lld_exchange_polled(spip, n, txbuf, rxbuf);
}

void spi_lld_send(SPIDriver *spip, size_t n, const void *txbuf) {
#if defined(EFR32_DMA_REQUIRED) || defined(__DOXYGEN__)
  if (spi_lld_dma_usable(spip, n, true, false)) {
    spi_lld_send_dma(spip, n, txbuf);
    return;
  }
#endif
  spi_lld_exchange_polled(spip, n, txbuf, NULL);
}

void spi_lld_receive(SPIDriver *spip, size_t n, void *rxbuf) {
#if defined(EFR32_DMA_REQUIRED) || defined(__DOXYGEN__)
  if (spi_lld_dma_usable(spip, n, false, true)) {
    spi_lld_receive_dma(spip, n, rxbuf);
    return;
  }
#endif
  spi_lld_exchange_polled(spip, n, NULL, rxbuf);
}

/*===========================================================================*/
/* DMA-based transfer functions.                                             */
/*===========================================================================*/

#if defined(EFR32_DMA_REQUIRED) || defined(__DOXYGEN__)

/**
 * @brief   DMA SPI full-duplex exchange.
 */
void spi_lld_exchange_dma(SPIDriver *spip, size_t n,
                          const void *txbuf, void *rxbuf) {
  EUSART_TypeDef *u = spip->eusart;
  uint32_t idx = spip->index;
  const efr32_dma_stream_t *tx_st = spip->dma_tx;
  const efr32_dma_stream_t *rx_st = spip->dma_rx;

  if (tx_st != NULL && rx_st != NULL && n > 0U && txbuf != NULL) {
    /* Route DMA requests. */
    dmaStreamSetRequestSource(tx_st, efr32_spi_dma_src[idx],
                              EFR32_SPI_DMA_SIG_TX);
    dmaStreamSetRequestSource(rx_st, efr32_spi_dma_src[idx],
                              EFR32_SPI_DMA_SIG_RX);

    /* Start RX first so it's ready to capture incoming data. */
    if (rxbuf != NULL) {
      dmaStartPeriphToMem(rx_st, &u->RXDATA, rxbuf, n,
                          EFR32_DMA_CH_CTRL_SIZE_BYTE);
    }

    /* Start TX. */
    dmaStartMemToPeriph(tx_st, txbuf, &u->TXDATA, n,
                        EFR32_DMA_CH_CTRL_SIZE_BYTE);

    /* If no RX buffer was given, drain the incoming data with a dummy
       buffer in chunks so the RX FIFO cannot overflow for n > 256. */
    if (rxbuf == NULL) {
      static uint8_t dummy_rx[256];
      size_t left = n;
      while (left > 0U) {
        size_t c = (left > sizeof(dummy_rx)) ? sizeof(dummy_rx) : left;
        dmaStartPeriphToMem(rx_st, &u->RXDATA, dummy_rx, c,
                            EFR32_DMA_CH_CTRL_SIZE_BYTE);
        dmaWaitCompletion(rx_st);
        dmaStreamDisable(rx_st);
        left -= c;
      }
    }

    /* Wait for both transfers. */
    dmaWaitCompletion(tx_st);
    dmaStreamDisable(tx_st);
    if (rxbuf != NULL) {
      dmaWaitCompletion(rx_st);
      dmaStreamDisable(rx_st);
    }

    /* Wait for TX complete (all bits shifted out). */
    while ((u->IF & EUSART_IF_TXC) == 0U) {}
    u->IF_CLR = EUSART_IF_TXC;

    spip->state = SPI_READY;
  } else {
    /* Fallback to polling. */
    spi_lld_exchange_polled(spip, n, txbuf, rxbuf);
  }
}

/**
 * @brief   DMA SPI send (TX only).
 */
void spi_lld_send_dma(SPIDriver *spip, size_t n, const void *txbuf) {
  EUSART_TypeDef *u = spip->eusart;
  uint32_t idx = spip->index;
  const efr32_dma_stream_t *tx_st = spip->dma_tx;

  if (tx_st != NULL && n > 0U && txbuf != NULL) {
    dmaStreamSetRequestSource(tx_st, efr32_spi_dma_src[idx],
                              EFR32_SPI_DMA_SIG_TX);
    dmaStartMemToPeriph(tx_st, txbuf, &u->TXDATA, n,
                        EFR32_DMA_CH_CTRL_SIZE_BYTE);
    dmaWaitCompletion(tx_st);
    dmaStreamDisable(tx_st);

    while ((u->IF & EUSART_IF_TXC) == 0U) {}
    u->IF_CLR = EUSART_IF_TXC;

    spip->state = SPI_READY;
  } else {
    spi_lld_exchange_polled(spip, n, txbuf, NULL);
  }
}

/**
 * @brief   DMA SPI receive (RX only).
 */
void spi_lld_receive_dma(SPIDriver *spip, size_t n, void *rxbuf) {
  EUSART_TypeDef *u = spip->eusart;
  uint32_t idx = spip->index;
  const efr32_dma_stream_t *rx_st = spip->dma_rx;
  const efr32_dma_stream_t *tx_st = spip->dma_tx;

  if (rx_st != NULL && n > 0U && rxbuf != NULL) {
    /* If TX DMA is available, use DMA exchange with dummy TX pattern. */
    if (tx_st != NULL) {
      static const uint8_t dummy_tx[256] = { [0 ... 255] = 0xFFU };
      if (n <= sizeof(dummy_tx)) {
        spi_lld_exchange_dma(spip, n, dummy_tx, rxbuf);
        return;
      }
    }

    /* RX DMA only: clock out via polling TX. */
    dmaStreamSetRequestSource(rx_st, efr32_spi_dma_src[idx],
                              EFR32_SPI_DMA_SIG_RX);
    dmaStartPeriphToMem(rx_st, &u->RXDATA, rxbuf, n,
                        EFR32_DMA_CH_CTRL_SIZE_BYTE);

    for (size_t i = 0U; i < n; i++) {
      u->TXDATA = 0xFFU;
      while ((u->IF & EUSART_IF_TXFL) == 0U) {}
    }

    dmaWaitCompletion(rx_st);
    dmaStreamDisable(rx_st);

    while ((u->IF & EUSART_IF_TXC) == 0U) {}
    u->IF_CLR = EUSART_IF_TXC;

    spip->state = SPI_READY;
  } else {
    spi_lld_exchange_polled(spip, n, NULL, rxbuf);
  }
}

#endif /* EFR32_DMA_REQUIRED */

#endif /* HAL_USE_SPI == TRUE */

/** @} */
