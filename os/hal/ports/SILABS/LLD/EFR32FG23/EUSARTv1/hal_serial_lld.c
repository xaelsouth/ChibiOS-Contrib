/*
    ChibiOS - Copyright (C) 2024 Xael South

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
 * @file    EUSARTv1/hal_serial_lld.c
 * @brief   SERIAL Driver subsystem low level driver source.
 *
 * @addtogroup SERIAL
 * @{
 */

#include "hal.h"

#if (HAL_USE_SERIAL == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if (EFR32_SERIAL_USE_USART1 == TRUE) || defined(__DOXYGEN__)
SerialDriver SD1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

static const SerialConfig default_config = {
  .speed = SERIAL_DEFAULT_BITRATE
};

/* USART0 (hardware) -> SD1 (ChibiOS) register/clock mapping. */
#define SERIAL_USART      USART0
#define SERIAL_CLKEN      CMU_CLKEN0_USART0
#define SERIAL_CLK        EFR32_USART1CLK
#define SERIAL_RX_IRQN    EFR32_USART1_RX_NUMBER
#define SERIAL_TX_IRQN    EFR32_USART1_TX_NUMBER
#define SERIAL_RX_PRIO    EFR32_USART1_RX_IRQ_PRIORITY
#define SERIAL_TX_PRIO    EFR32_USART1_TX_IRQ_PRIORITY

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

#if EFR32_SERIAL_USE_RX_DMA
static void serial_lld_dma_rx_pump(SerialDriver *sdp) {
  do {
    while (!iqIsFullI(&sdp->iqueue) &&
           !efr32_stream_rx_empty(&sdp->dma_rx))
      sdIncomingDataI(sdp, efr32_stream_rx_get(&sdp->dma_rx));
    efr32_stream_rx_rearm(&sdp->dma_rx, iqIsEmptyI(&sdp->iqueue));
  } while (!iqIsFullI(&sdp->iqueue) &&
           !efr32_stream_rx_empty(&sdp->dma_rx));
}

static void serial_lld_rx_notify(io_queue_t *qp) {
  SerialDriver *sdp = qGetLink(qp);
  if (sdp->state == SD_READY)
    serial_lld_dma_rx_pump(sdp);
}

static void serial_lld_dma_rx_cb(void *p, uint32_t flags) {
  SerialDriver *sdp = p;
  if ((flags & EFR32_DMA_ISR_ERROR_MASK) != 0U) {
    EFR32_SERIAL_DMA_ERROR_HOOK(sdp);
    return;
  }
  efr32_stream_rx_complete(&sdp->dma_rx);
  serial_lld_dma_rx_pump(sdp);
}
#endif

#if EFR32_SERIAL_USE_TX_DMA
static void serial_lld_dma_tx_start(SerialDriver *sdp) {
  USART_TypeDef *u = sdp->usart;
  size_t count = 0U;

  if (sdp->dma_tx_busy)
    return;
  while (count < sizeof(sdp->dma_tx_buf)) {
    msg_t b = oqGetI(&sdp->oqueue);
    if (b < MSG_OK)
      break;
    sdp->dma_tx_buf[count++] = (uint8_t)b;
  }
  if (count == 0U) {
    chnAddFlagsI(sdp, CHN_OUTPUT_EMPTY);
    u->IEN_SET = USART_IEN_TXC;
    return;
  }
  u->IEN_CLR = USART_IEN_TXC | USART_IEN_TXBL;
  u->IF_CLR = USART_IF_TXC;
  dmaStreamSetRequestSource(sdp->dma_tx,
                            _LDMAXBAR_CH_REQSEL_SOURCESEL_USART0,
                            _LDMAXBAR_CH_REQSEL_SIGSEL_USART0TXBL);
  sdp->dma_tx_busy = true;
  dmaStartMemToPeriph(sdp->dma_tx, sdp->dma_tx_buf, &u->TXDATA, count,
                      EFR32_DMA_CH_CTRL_SIZE_BYTE);
}

static void serial_lld_dma_tx_cb(void *p, uint32_t flags) {
  SerialDriver *sdp = p;
  if ((flags & EFR32_DMA_ISR_ERROR_MASK) != 0U) {
    EFR32_SERIAL_DMA_ERROR_HOOK(sdp);
    return;
  }
  dmaStreamDisable(sdp->dma_tx);
  sdp->dma_tx_busy = false;
  serial_lld_dma_tx_start(sdp);
}
#endif

static void serial_lld_notify(io_queue_t *qp) {
  SerialDriver *sdp = qGetLink(qp);
#if EFR32_SERIAL_USE_TX_DMA
  if (sdp->state == SD_READY)
    serial_lld_dma_tx_start(sdp);
#else
  USART_TypeDef *u = sdp->usart;

  u->IF_CLR = USART_IF_TXC;
  u->IEN_SET = USART_IEN_TXBL | USART_IEN_TXC;
#endif
}

__STATIC_INLINE void serial_lld_set_error(SerialDriver *sdp, uint32_t iflags) {
  if (iflags & USART_IF_RXOF)  chnAddFlagsI(sdp, SD_OVERRUN_ERROR);
  if (iflags & USART_IF_PERR)  chnAddFlagsI(sdp, SD_PARITY_ERROR);
  if (iflags & USART_IF_FERR)  chnAddFlagsI(sdp, SD_FRAMING_ERROR);
}

/*===========================================================================*/
/* Driver interrupt handlers (dispatched from hal_eusart_irq.c).             */
/*===========================================================================*/

void sd_lld_serve_interrupt(SerialDriver *sdp) {
  USART_TypeDef *u = (USART_TypeDef *)sdp->usart;
  uint32_t isr = u->IF;
  bool sent = false;

  /* RX: drain the hardware FIFO in a loop while data is available. */
  if (isr & (USART_IF_RXDATAV | USART_IF_RXFULL | USART_IF_RXOF |
             USART_IF_PERR | USART_IF_FERR)) {
    osalSysLockFromISR();
    if (isr & (USART_IF_RXOF | USART_IF_PERR | USART_IF_FERR))
      serial_lld_set_error(sdp, isr);
#if !EFR32_SERIAL_USE_RX_DMA
    /* Loop while RXDATAV is set — drains up to FIFO depth (8) per IRQ. */
    while ((u->STATUS & USART_STATUS_RXDATAV) != 0U) {
      uint8_t b = (uint8_t)(u->RXDATA & 0xFFU);
      sdIncomingDataI(sdp, b);
    }
#endif
    osalSysUnlockFromISR();
  }

  /* TX: fill the hardware FIFO in a loop while there is room. */
  if ((u->IEN & USART_IEN_TXBL) && (isr & USART_IF_TXBL)) {
    osalSysLockFromISR();
    /* Loop while TXBL (TX buffer has room) and queue is not empty. */
    while ((u->STATUS & USART_STATUS_TXBL) != 0U) {
      msg_t b = oqGetI(&sdp->oqueue);
      if (b < MSG_OK) {
        chnAddFlagsI(sdp, CHN_OUTPUT_EMPTY);
        u->IEN_CLR = USART_IEN_TXBL;
        break;
      }
      u->TXDATA = (uint32_t)b;
      sent = true;
    }
    osalSysUnlockFromISR();
  }

  /* TX complete. */
  if (!sent && (u->IEN & USART_IEN_TXC) && (isr & USART_IF_TXC)) {
    osalSysLockFromISR();
    if (oqIsEmptyI(&sdp->oqueue)) {
      chnAddFlagsI(sdp, CHN_TRANSMISSION_END);
      u->IEN_CLR = USART_IEN_TXC;
    }
    osalSysUnlockFromISR();
    u->IF_CLR = USART_IF_TXC;
  }

  /* Clear handled flags. */
  u->IF_CLR = isr & (USART_IF_RXOF | USART_IF_PERR | USART_IF_FERR);
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void sd_lld_init(void) {
#if EFR32_SERIAL_USE_USART1 == TRUE
  sdObjectInit(&SD1,
#if EFR32_SERIAL_USE_RX_DMA
               serial_lld_rx_notify,
#else
               NULL,
#endif
               serial_lld_notify);
  SD1.usart = (void *)SERIAL_USART;
  SD1.clock = SERIAL_CLK;
#endif
}

msg_t sd_lld_start(SerialDriver *sdp, const SerialConfig *config) {
  USART_TypeDef *u = (USART_TypeDef *)sdp->usart;
  uint64_t div;

  if (config == NULL)
    config = &default_config;

  if ((config->speed == 0U) || (config->irhfcg != 0U) ||
      (config->irlfcfg != 0U) || (config->irctrl != 0U) ||
      ((uint64_t)config->speed * 16U > sdp->clock)) {
    if (sdp->state == SD_READY)
      sd_lld_stop(sdp);
    return HAL_RET_CONFIG_ERROR;
  }
  div = ((uint64_t)sdp->clock * 256U) /
        ((uint64_t)config->speed * 16U) - 256U;
  if (div > _USART_CLKDIV_DIV_MASK) {
    if (sdp->state == SD_READY)
      sd_lld_stop(sdp);
    return HAL_RET_CONFIG_ERROR;
  }

  if (sdp->state == SD_READY)
    sd_lld_stop(sdp);

#if EFR32_SERIAL_USE_TX_DMA
  sdp->dma_tx = dmaStreamAllocI(EFR32_DMA_STREAM_ID_ANY,
                                 EFR32_SERIAL_DMA_IRQ_PRIORITY,
                                 serial_lld_dma_tx_cb, sdp);
  if (sdp->dma_tx == NULL)
    return HAL_RET_NO_RESOURCE;
  sdp->dma_tx_busy = false;
#endif

#if EFR32_SERIAL_USE_RX_DMA
  sdp->dma_rx.stream = dmaStreamAllocI(EFR32_DMA_STREAM_ID_ANY,
                                        EFR32_SERIAL_DMA_IRQ_PRIORITY,
                                        serial_lld_dma_rx_cb, sdp);
  if (sdp->dma_rx.stream == NULL) {
#if EFR32_SERIAL_USE_TX_DMA
    dmaStreamFreeI(sdp->dma_tx);
    sdp->dma_tx = NULL;
#endif
    return HAL_RET_NO_RESOURCE;
  }
  sdp->dma_rx.reg = &u->RXDATA;
  sdp->dma_rx.source = _LDMAXBAR_CH_REQSEL_SOURCESEL_USART0;
  sdp->dma_rx.rd = sdp->dma_rx.wr = 0U;
  sdp->dma_rx.active = false;
#endif

  CMU->CLKEN0_SET = SERIAL_CLKEN;
  u->EN_CLR = USART_EN_EN;
  u->CTRL = 0;
  u->FRAME = USART_FRAME_DATABITS_EIGHT | USART_FRAME_PARITY_EVEN |
             USART_FRAME_STOPBITS_ONE;

  u->EN_SET = USART_EN_EN;
  u->CLKDIV = (uint32_t)div & _USART_CLKDIV_DIV_MASK;

  u->CMD_SET = USART_CMD_CLEARTX | USART_CMD_CLEARRX;
  u->IF_CLR = _USART_IF_MASK;
  u->CMD_SET = USART_CMD_RXEN | USART_CMD_TXEN;

  u->IEN = USART_IEN_RXOF | USART_IEN_PERR | USART_IEN_FERR;
#if EFR32_SERIAL_USE_RX_DMA
  efr32_stream_rx_start(&sdp->dma_rx, true);
#else
  u->IEN_SET = USART_IEN_RXDATAV;
#endif
  if (!oqIsEmptyI(&sdp->oqueue)) {
#if EFR32_SERIAL_USE_TX_DMA
    serial_lld_dma_tx_start(sdp);
#else
    u->IEN_SET = USART_IEN_TXBL | USART_IEN_TXC;
#endif
  }
  nvicEnableVector(SERIAL_RX_IRQN, SERIAL_RX_PRIO);
  nvicEnableVector(SERIAL_TX_IRQN, SERIAL_TX_PRIO);
  return HAL_RET_SUCCESS;
}

void sd_lld_stop(SerialDriver *sdp) {
  USART_TypeDef *u = (USART_TypeDef *)sdp->usart;

  if (sdp->state == SD_READY) {
    nvicDisableVector(SERIAL_RX_IRQN);
    nvicDisableVector(SERIAL_TX_IRQN);
#if EFR32_SERIAL_USE_RX_DMA
    efr32_stream_rx_stop(&sdp->dma_rx);
    dmaStreamFreeI(sdp->dma_rx.stream);
    sdp->dma_rx.stream = NULL;
#endif
#if EFR32_SERIAL_USE_TX_DMA
    dmaStreamDisable(sdp->dma_tx);
    while ((LDMA->CHBUSY & (1U << sdp->dma_tx->selfindex)) != 0U) {}
    dmaStreamFreeI(sdp->dma_tx);
    sdp->dma_tx = NULL;
    sdp->dma_tx_busy = false;
#endif
    u->IEN_CLR = _USART_IEN_MASK;
    u->CMD_SET = USART_CMD_RXDIS | USART_CMD_TXDIS;
    u->EN_CLR = USART_EN_EN;
    CMU->CLKEN0_CLR = SERIAL_CLKEN;
  }
}

#endif /* HAL_USE_SERIAL == TRUE */

/** @} */
