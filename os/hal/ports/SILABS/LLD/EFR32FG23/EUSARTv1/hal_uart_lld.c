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

#include <string.h>
#include "hal.h"
#include "efr32_irda.h"

#if HAL_USE_UART == TRUE

static const uint32_t uart_clken[4] = {
  CMU_CLKEN1_EUSART0, CMU_CLKEN1_EUSART1, CMU_CLKEN1_EUSART2, CMU_CLKEN0_USART0
};
static const uint32_t uart_irqn_rx[4] = {
  EFR32_EUSART1_RX_NUMBER, EFR32_EUSART2_RX_NUMBER,
  EFR32_EUSART3_RX_NUMBER, EFR32_USART1_RX_NUMBER
};
static const uint32_t uart_irqn_tx[4] = {
  EFR32_EUSART1_TX_NUMBER, EFR32_EUSART2_TX_NUMBER,
  EFR32_EUSART3_TX_NUMBER, EFR32_USART1_TX_NUMBER
};
static const uint32_t uart_irqprio[4] = {
  EFR32_UART_EUSART1_IRQ_PRIORITY, EFR32_UART_EUSART2_IRQ_PRIORITY,
  EFR32_UART_EUSART3_IRQ_PRIORITY, EFR32_UART_USART1_IRQ_PRIORITY
};

#if EFR32_UART_USE_EUSART1
UARTDriver UARTD1;
#endif
#if EFR32_UART_USE_EUSART2
UARTDriver UARTD2;
#endif
#if EFR32_UART_USE_EUSART3
UARTDriver UARTD3;
#endif
#if EFR32_UART_USE_USART1
UARTDriver UARTD4;
#endif

static void uart_enter_rx_idle_loop(UARTDriver *uartp) {
  (void)uartp;
}

static uint32_t uart_get_ovs(uint32_t cfg0, bool is_usart) {
  static const uint8_t values[] = {16U, 8U, 6U, 4U, 1U};
  uint32_t value = is_usart ?
    ((cfg0 & _USART_CTRL_OVS_MASK) >> _USART_CTRL_OVS_SHIFT) :
    ((cfg0 & _EUSART_CFG0_OVS_MASK) >> _EUSART_CFG0_OVS_SHIFT);

  if (value >= (is_usart ? 4U : 5U))
    return 0U;
  return values[value];
}

static uartflags_t uart_errors(uint32_t flags, bool is_usart) {
  uartflags_t errors = 0U;

  if ((flags & (is_usart ? USART_IF_RXOF : EUSART_IF_RXOF)) != 0U)
    errors |= UART_OVERRUN_ERROR;
  if ((flags & (is_usart ? USART_IF_PERR : EUSART_IF_PERR)) != 0U)
    errors |= UART_PARITY_ERROR;
  if ((flags & (is_usart ? USART_IF_FERR : EUSART_IF_FERR)) != 0U)
    errors |= UART_FRAMING_ERROR;
  return errors;
}

#if EFR32_UART_USE_DMA
static const uint32_t uart_dma_source[4] = {
  _LDMAXBAR_CH_REQSEL_SOURCESEL_EUSART0,
  _LDMAXBAR_CH_REQSEL_SOURCESEL_EUSART1,
  _LDMAXBAR_CH_REQSEL_SOURCESEL_EUSART2,
  _LDMAXBAR_CH_REQSEL_SOURCESEL_USART0
};

static void uart_dma_tx_cb(void *p, uint32_t flags) {
  UARTDriver *uartp = p;

  if ((flags & EFR32_DMA_ISR_ERROR_MASK) != 0U) {
    EFR32_UART_DMA_ERROR_HOOK(uartp);
    return;
  }
  dmaStreamDisable(uartp->dma_tx);
  uartp->dma_tx_done = true;
  nvicSetPending(uart_irqn_tx[uartp->index]);
}

static void uart_dma_rx_cb(void *p, uint32_t flags) {
  UARTDriver *uartp = p;

  if ((flags & EFR32_DMA_ISR_ERROR_MASK) != 0U) {
    EFR32_UART_DMA_ERROR_HOOK(uartp);
    return;
  }
  dmaStreamDisable(uartp->dma_rx);
  uartp->dma_rx_done = true;
  nvicSetPending(uart_irqn_rx[uartp->index]);
}

static void uart_dma_start_tx(UARTDriver *uartp) {
  volatile uint32_t *reg;
  uint32_t size = uartp->wide ? EFR32_DMA_CH_CTRL_SIZE_HALFWORD :
                                EFR32_DMA_CH_CTRL_SIZE_BYTE;

  uartp->dma_tx_count = uartp->txremaining > 2048U ? 2048U :
                                                        uartp->txremaining;
  if (uartp->is_usart) {
    USART_TypeDef *u = uartp->usart;
    reg = uartp->wide ? &u->TXDATAX : &u->TXDATA;
  } else {
    EUSART_TypeDef *u = uartp->usart;
    reg = &u->TXDATA;
  }
  uartp->dma_tx_active = true;
  uartp->dma_tx_done = false;
  dmaStreamSetRequestSource(uartp->dma_tx, uart_dma_source[uartp->index],
                            uartp->is_usart ? 2U : 1U);
  dmaStartMemToPeriph(uartp->dma_tx, uartp->txbuf, reg,
                      uartp->dma_tx_count, size);
}

static void uart_dma_start_rx(UARTDriver *uartp) {
  const volatile uint32_t *reg;
  uint32_t size = uartp->wide ? EFR32_DMA_CH_CTRL_SIZE_HALFWORD :
                                EFR32_DMA_CH_CTRL_SIZE_BYTE;

  uartp->dma_rx_count = uartp->rxremaining > 2048U ? 2048U :
                                                        uartp->rxremaining;
  if (uartp->is_usart) {
    USART_TypeDef *u = uartp->usart;
    reg = uartp->wide ? &u->RXDATAX : &u->RXDATA;
  } else {
    EUSART_TypeDef *u = uartp->usart;
    reg = &u->RXDATA;
  }
  uartp->dma_rx_active = true;
  uartp->dma_rx_done = false;
  dmaStreamSetRequestSource(uartp->dma_rx, uart_dma_source[uartp->index], 0U);
  dmaStartPeriphToMem(uartp->dma_rx, reg, uartp->rxbuf,
                      uartp->dma_rx_count, size);
}

static void uart_dma_account_tx(UARTDriver *uartp, size_t count) {
  uartp->txbuf += count * (uartp->wide ? 2U : 1U);
  uartp->txremaining -= count;
  uartp->dma_tx_active = false;
  uartp->dma_tx_done = false;
}

static void uart_dma_account_rx(UARTDriver *uartp, size_t count) {
  if (uartp->is_usart && uartp->wide) {
    for (size_t i = 0U; i < count; i++) {
      uint16_t data;
      memcpy(&data, uartp->rxbuf + i * 2U, sizeof(data));
      data &= _USART_RXDATAX_RXDATA_MASK;
      memcpy(uartp->rxbuf + i * 2U, &data, sizeof(data));
    }
  }
  uartp->rxbuf += count * (uartp->wide ? 2U : 1U);
  uartp->rxremaining -= count;
  uartp->dma_rx_active = false;
  uartp->dma_rx_done = false;
}

static size_t uart_dma_stop_count(const efr32_dma_stream_t *stream,
                                  const void *buf, size_t width,
                                  size_t count, bool done, bool receive) {
  dmaStreamDisable(stream);
  while ((LDMA->CHBUSY & (1U << stream->selfindex)) != 0U) {}
  if (done || ((LDMA->CHDONE & (1U << stream->selfindex)) != 0U))
    return count;
  uintptr_t next = receive ? stream->stream->DST : stream->stream->SRC;
  return (next - (uintptr_t)buf) / width;
}
#endif

void uart_lld_init(void) {
#if EFR32_UART_USE_EUSART1
  uartObjectInit(&UARTD1);
  UARTD1.usart = EUSART0; UARTD1.is_usart = false; UARTD1.index = 0U;
  UARTD1.clock = EFR32_EUSART1CLK;
#endif
#if EFR32_UART_USE_EUSART2
  uartObjectInit(&UARTD2);
  UARTD2.usart = EUSART1; UARTD2.is_usart = false; UARTD2.index = 1U;
  UARTD2.clock = EFR32_EUSART23CLK;
#endif
#if EFR32_UART_USE_EUSART3
  uartObjectInit(&UARTD3);
  UARTD3.usart = EUSART2; UARTD3.is_usart = false; UARTD3.index = 2U;
  UARTD3.clock = EFR32_EUSART23CLK;
#endif
#if EFR32_UART_USE_USART1
  uartObjectInit(&UARTD4);
  UARTD4.usart = USART0; UARTD4.is_usart = true; UARTD4.index = 3U;
  UARTD4.clock = EFR32_USART1CLK;
#endif
}

msg_t uart_lld_start(UARTDriver *uartp) {
  const UARTConfig *config = uartp->config;
  uint32_t ovs = uart_get_ovs(config->cfg0, uartp->is_usart);
  uint32_t bits = config->framecfg & (uartp->is_usart ?
                  _USART_FRAME_DATABITS_MASK : _EUSART_FRAMECFG_DATABITS_MASK);
  uint64_t div;

  if (uartp->state == UART_READY)
    uart_lld_stop(uartp);

  if ((config->speed == 0U) || (ovs == 0U) ||
      ((uint64_t)config->speed * ovs > uartp->clock) ||
      (config->irctrl > EFR32_IRDA_AUTO) ||
      (uartp->is_usart && ((bits > USART_FRAME_DATABITS_NINE) ||
                           (config->irctrl != EFR32_IRDA_NONE) ||
                           (config->irhfcg != 0U) || (config->irlfcfg != 0U))))
    return HAL_RET_CONFIG_ERROR;

  div = ((uint64_t)uartp->clock * 256U) /
        ((uint64_t)ovs * config->speed) - 256U;
  if (div > (uartp->is_usart ? _USART_CLKDIV_DIV_MASK :
                               _EUSART_CLKDIV_MASK))
    return HAL_RET_CONFIG_ERROR;

#if EFR32_UART_USE_DMA
  uartp->dma_tx = dmaStreamAllocI(EFR32_DMA_STREAM_ID_ANY,
                                  EFR32_UART_DMA_IRQ_PRIORITY,
                                  uart_dma_tx_cb, uartp);
  if (uartp->dma_tx == NULL)
    return HAL_RET_NO_RESOURCE;
  uartp->dma_rx = dmaStreamAllocI(EFR32_DMA_STREAM_ID_ANY,
                                  EFR32_UART_DMA_IRQ_PRIORITY,
                                  uart_dma_rx_cb, uartp);
  if (uartp->dma_rx == NULL) {
    dmaStreamFreeI(uartp->dma_tx);
    uartp->dma_tx = NULL;
    return HAL_RET_NO_RESOURCE;
  }
  uartp->dma_tx_active = uartp->dma_rx_active = false;
  uartp->dma_tx_done = uartp->dma_rx_done = false;
#endif

  uartp->wide = bits > (uartp->is_usart ? USART_FRAME_DATABITS_EIGHT :
                                          EUSART_FRAMECFG_DATABITS_EIGHT);
  uartp->txstate = UART_TX_IDLE;
  uartp->rxstate = UART_RX_IDLE;
  uartp->txremaining = 0U;
  uartp->rxremaining = 0U;

  if (uartp->is_usart) {
    USART_TypeDef *u = uartp->usart;
    CMU->CLKEN0_SET = uart_clken[uartp->index];
    u->EN_CLR = USART_EN_EN;
    u->CTRL = config->cfg0;
    u->FRAME = config->framecfg;
    u->EN_SET = USART_EN_EN;
    u->CLKDIV = (uint32_t)div & _USART_CLKDIV_DIV_MASK;
    u->CMD_SET = USART_CMD_CLEARRX | USART_CMD_CLEARTX;
    u->IF_CLR = _USART_IF_MASK;
    u->CMD_SET = USART_CMD_RXEN | USART_CMD_TXEN;
    u->IEN = USART_IEN_RXDATAV | USART_IEN_RXOF | USART_IEN_PERR | USART_IEN_FERR;
  } else {
    EUSART_TypeDef *u = uartp->usart;
    CMU->CLKEN1_SET = uart_clken[uartp->index];
    u->EN_CLR = EUSART_EN_EN;
    while ((u->EN & EUSART_EN_DISABLING) != 0U) {}
    u->CFG0 = config->cfg0;
    u->FRAMECFG = config->framecfg;
    efr32_irda_apply(u, config->irctrl, config->irhfcg, config->irlfcfg,
                     uartp->clock);
    u->EN_SET = EUSART_EN_EN;
    u->CLKDIV = (uint32_t)div & _EUSART_CLKDIV_MASK;
    u->CMD_SET = EUSART_CMD_CLEARTX;
    while ((u->SYNCBUSY & _EUSART_SYNCBUSY_MASK) != 0U) {}
    for (unsigned i = 0U; i < 16U &&
         (u->STATUS & EUSART_STATUS_RXFL) != 0U; i++)
      (void)u->RXDATA;
    u->IF_CLR = _EUSART_IF_MASK;
    u->CMD_SET = EUSART_CMD_RXEN | EUSART_CMD_TXEN;
    while ((u->SYNCBUSY & _EUSART_SYNCBUSY_MASK) != 0U) {}
    u->IEN = EUSART_IEN_RXFL | EUSART_IEN_RXOF | EUSART_IEN_PERR | EUSART_IEN_FERR;
  }
  nvicEnableVector(uart_irqn_rx[uartp->index], uart_irqprio[uartp->index]);
  nvicEnableVector(uart_irqn_tx[uartp->index], uart_irqprio[uartp->index]);
  return HAL_RET_SUCCESS;
}

void uart_lld_stop(UARTDriver *uartp) {
  if (uartp->state != UART_READY)
    return;

  nvicDisableVector(uart_irqn_rx[uartp->index]);
  nvicDisableVector(uart_irqn_tx[uartp->index]);
#if EFR32_UART_USE_DMA
  if (uartp->dma_tx_active)
    (void)uart_lld_stop_send(uartp);
  if (uartp->dma_rx_active)
    (void)uart_lld_stop_receive(uartp);
  dmaStreamFreeI(uartp->dma_tx);
  dmaStreamFreeI(uartp->dma_rx);
  uartp->dma_tx = uartp->dma_rx = NULL;
#endif
  if (uartp->is_usart) {
    USART_TypeDef *u = uartp->usart;
    u->IEN = 0U;
    u->CMD_SET = USART_CMD_RXDIS | USART_CMD_TXDIS;
    u->IF_CLR = _USART_IF_MASK;
    u->EN_CLR = USART_EN_EN;
    CMU->CLKEN0_CLR = uart_clken[uartp->index];
  } else {
    EUSART_TypeDef *u = uartp->usart;
    u->IEN = 0U;
    u->CMD_SET = EUSART_CMD_RXDIS | EUSART_CMD_TXDIS;
    while ((u->SYNCBUSY & _EUSART_SYNCBUSY_MASK) != 0U) {}
    u->IF_CLR = _EUSART_IF_MASK;
    u->EN_CLR = EUSART_EN_EN;
    while ((u->EN & EUSART_EN_DISABLING) != 0U) {}
    CMU->CLKEN1_CLR = uart_clken[uartp->index];
  }
}

void uart_lld_start_send(UARTDriver *uartp, size_t n, const void *txbuf) {
  uartp->txbuf = txbuf;
  uartp->txremaining = n;
  if (uartp->is_usart) {
    USART_TypeDef *u = uartp->usart;
    u->IEN_CLR = USART_IEN_TXC;
    u->IF_CLR = USART_IF_TXC;
#if !EFR32_UART_USE_DMA
    u->IEN_SET = USART_IEN_TXBL;
#endif
  } else {
    EUSART_TypeDef *u = uartp->usart;
    u->IEN_CLR = EUSART_IEN_TXC;
    u->IF_CLR = EUSART_IF_TXC;
#if !EFR32_UART_USE_DMA
    u->IEN_SET = EUSART_IEN_TXFL;
#endif
  }
#if EFR32_UART_USE_DMA
  uart_dma_start_tx(uartp);
#endif
}

size_t uart_lld_stop_send(UARTDriver *uartp) {
#if EFR32_UART_USE_DMA
  if (uartp->dma_tx_active) {
    size_t count = uart_dma_stop_count(uartp->dma_tx, uartp->txbuf,
                                       uartp->wide ? 2U : 1U,
                                       uartp->dma_tx_count,
                                       uartp->dma_tx_done, false);
    uart_dma_account_tx(uartp, count);
  }
#endif
  if (uartp->is_usart) {
    USART_TypeDef *u = uartp->usart;
    u->IEN_CLR = USART_IEN_TXBL | USART_IEN_TXC;
  } else {
    EUSART_TypeDef *u = uartp->usart;
    u->IEN_CLR = EUSART_IEN_TXFL | EUSART_IEN_TXC;
  }
  return uartp->txremaining;
}

void uart_lld_start_receive(UARTDriver *uartp, size_t n, void *rxbuf) {
  uartp->rxbuf = rxbuf;
  uartp->rxremaining = n;
#if EFR32_UART_USE_DMA
  if (uartp->is_usart) {
    USART_TypeDef *u = uartp->usart;
    u->IEN_CLR = USART_IEN_RXDATAV;
  } else {
    EUSART_TypeDef *u = uartp->usart;
    u->IEN_CLR = EUSART_IEN_RXFL;
  }
  uart_dma_start_rx(uartp);
#endif
}

size_t uart_lld_stop_receive(UARTDriver *uartp) {
#if EFR32_UART_USE_DMA
  if (uartp->dma_rx_active) {
    size_t count = uart_dma_stop_count(uartp->dma_rx, uartp->rxbuf,
                                       uartp->wide ? 2U : 1U,
                                       uartp->dma_rx_count,
                                       uartp->dma_rx_done, true);
    uart_dma_account_rx(uartp, count);
  }
  if (uartp->is_usart) {
    USART_TypeDef *u = uartp->usart;
    u->IEN_SET = USART_IEN_RXDATAV;
  } else {
    EUSART_TypeDef *u = uartp->usart;
    u->IEN_SET = EUSART_IEN_RXFL;
  }
#endif
  return uartp->rxremaining;
}

void uart_lld_serve_interrupt(UARTDriver *uartp) {
  uint32_t flags, enabled, errors, rxflag, txflag, txcflag;

  if (uartp->is_usart) {
    USART_TypeDef *u = uartp->usart;
    flags = u->IF; enabled = u->IEN;
    rxflag = USART_IF_RXDATAV; txflag = USART_IF_TXBL; txcflag = USART_IF_TXC;
    errors = flags & enabled & (USART_IF_RXOF | USART_IF_PERR | USART_IF_FERR);
    u->IF_CLR = errors;
  } else {
    EUSART_TypeDef *u = uartp->usart;
    flags = u->IF; enabled = u->IEN;
    rxflag = EUSART_IF_RXFL; txflag = EUSART_IF_TXFL; txcflag = EUSART_IF_TXC;
    errors = flags & enabled & (EUSART_IF_RXOF | EUSART_IF_PERR | EUSART_IF_FERR);
    u->IF_CLR = errors;
  }
  if (errors != 0U)
    _uart_rx_error_isr_code(uartp, uart_errors(errors, uartp->is_usart));

#if EFR32_UART_USE_DMA
  bool rx_complete = false;
  bool tx_complete = false;
  osalSysLockFromISR();
  if (uartp->dma_rx_done) {
    uart_dma_account_rx(uartp, uartp->dma_rx_count);
    if (uartp->rxremaining != 0U) {
      uart_dma_start_rx(uartp);
    } else {
      if (uartp->is_usart)
        ((USART_TypeDef *)uartp->usart)->IEN_SET = USART_IEN_RXDATAV;
      else
        ((EUSART_TypeDef *)uartp->usart)->IEN_SET = EUSART_IEN_RXFL;
      rx_complete = true;
    }
  }
  if (uartp->dma_tx_done) {
    uart_dma_account_tx(uartp, uartp->dma_tx_count);
    if (uartp->txremaining != 0U) {
      uart_dma_start_tx(uartp);
    } else {
      if (uartp->is_usart)
        ((USART_TypeDef *)uartp->usart)->IEN_SET = USART_IEN_TXC;
      else
        ((EUSART_TypeDef *)uartp->usart)->IEN_SET = EUSART_IEN_TXC;
      tx_complete = true;
    }
  }
  osalSysUnlockFromISR();
  if (rx_complete)
    _uart_rx_complete_isr_code(uartp);
  if (tx_complete)
    _uart_tx1_isr_code(uartp);
#endif

  if ((flags & enabled & rxflag) != 0U) {
    while (true) {
      uint16_t data;
      if (uartp->is_usart) {
        USART_TypeDef *u = uartp->usart;
        if ((u->STATUS & USART_STATUS_RXDATAV) == 0U)
          break;
        data = (uint16_t)(uartp->wide ?
                          (u->RXDATAX & _USART_RXDATAX_RXDATA_MASK) :
                          u->RXDATA);
      } else {
        EUSART_TypeDef *u = uartp->usart;
        if ((u->STATUS & EUSART_STATUS_RXFL) == 0U)
          break;
        data = (uint16_t)u->RXDATA;
      }
      if (uartp->rxstate == UART_RX_ACTIVE && uartp->rxremaining > 0U) {
        size_t width = uartp->wide ? 2U : 1U;
        memcpy(uartp->rxbuf, &data, width);
        uartp->rxbuf += width;
        if (--uartp->rxremaining == 0U) {
          _uart_rx_complete_isr_code(uartp);
          break;
        }
      } else if (uartp->config->rxchar_cb != NULL) {
        uartp->config->rxchar_cb(uartp, data);
      }
    }
    if (!uartp->is_usart) {
      EUSART_TypeDef *u = uartp->usart;
      if ((u->STATUS & EUSART_STATUS_RXFL) == 0U)
        u->IF_CLR = EUSART_IF_RXFL;
    }
  }

  if ((flags & enabled & txflag) != 0U && uartp->txremaining > 0U) {
    while (uartp->txremaining > 0U) {
      uint16_t data = 0U;
      size_t width = uartp->wide ? 2U : 1U;
      if (uartp->is_usart) {
        USART_TypeDef *u = uartp->usart;
        if ((u->STATUS & USART_STATUS_TXBL) == 0U)
          break;
        memcpy(&data, uartp->txbuf, width);
        u->IF_CLR = USART_IF_TXC;
        if (uartp->wide) u->TXDATAX = data;
        else u->TXDATA = data;
      } else {
        EUSART_TypeDef *u = uartp->usart;
        if ((u->STATUS & EUSART_STATUS_TXFL) == 0U)
          break;
        memcpy(&data, uartp->txbuf, width);
        u->IF_CLR = EUSART_IF_TXC;
        u->TXDATA = data;
      }
      uartp->txbuf += width;
      uartp->txremaining--;
    }
    if (uartp->txremaining == 0U) {
      if (uartp->is_usart) {
        USART_TypeDef *u = uartp->usart;
        u->IEN_CLR = USART_IEN_TXBL;
        u->IEN_SET = USART_IEN_TXC;
      } else {
        EUSART_TypeDef *u = uartp->usart;
        u->IEN_CLR = EUSART_IEN_TXFL;
        u->IEN_SET = EUSART_IEN_TXC;
      }
      _uart_tx1_isr_code(uartp);
    }
  }

  if ((flags & enabled & txcflag) != 0U) {
    if (uartp->is_usart) {
      USART_TypeDef *u = uartp->usart;
      u->IEN_CLR = USART_IEN_TXC;
      u->IF_CLR = USART_IF_TXC;
    } else {
      EUSART_TypeDef *u = uartp->usart;
      u->IEN_CLR = EUSART_IEN_TXC;
      u->IF_CLR = EUSART_IF_TXC;
    }
    _uart_tx2_isr_code(uartp);
  }
}

#endif /* HAL_USE_UART */
