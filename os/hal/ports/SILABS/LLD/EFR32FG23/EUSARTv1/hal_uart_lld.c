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
 * @file    EUSARTv1/hal_uart_lld.c
 * @brief   EFR32FG23 UART subsystem low level driver source.
 *
 * @addtogroup UART
 * @{
 */

#include "hal.h"

#if (HAL_USE_UART == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* Instance table: index 0-2 = EUSART0-2, 3 = USART0. */
static void * const uart_hw[4]       = { EUSART0, EUSART1, EUSART2, USART0 };
static const bool uart_is_usart[4]    = { false, false, false, true };
static const uint32_t uart_clken[4]   = {
  CMU_CLKEN1_EUSART0, CMU_CLKEN1_EUSART1, CMU_CLKEN1_EUSART2, CMU_CLKEN0_USART0
};
static const uint32_t uart_irqn_rx[4] = {
  EUSART0_RX_IRQn, EUSART1_RX_IRQn, EUSART2_RX_IRQn, USART0_RX_IRQn
};
static const uint32_t uart_irqn_tx[4] = {
  EUSART0_TX_IRQn, EUSART1_TX_IRQn, EUSART2_TX_IRQn, USART0_TX_IRQn
};
static const uint32_t uart_clk[4]     = {
  EFR32_EUSART1CLK, EFR32_EUSART23CLK, EFR32_EUSART23CLK, EFR32_USART1CLK
};
static const uint32_t uart_irqprio[4] = {
  EFR32_UART_EUSART1_IRQ_PRIORITY, EFR32_UART_EUSART2_IRQ_PRIORITY,
  EFR32_UART_EUSART3_IRQ_PRIORITY, EFR32_UART_USART1_IRQ_PRIORITY
};

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if EFR32_UART_USE_EUSART1 || defined(__DOXYGEN__)
UARTDriver UARTD1;
#endif
#if EFR32_UART_USE_EUSART2 || defined(__DOXYGEN__)
UARTDriver UARTD2;
#endif
#if EFR32_UART_USE_EUSART3 || defined(__DOXYGEN__)
UARTDriver UARTD3;
#endif
#if EFR32_UART_USE_USART1 || defined(__DOXYGEN__)
UARTDriver UARTD4;
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static uint32_t uart_get_clkdiv(uint32_t clock, uint32_t ovs, uint32_t baud) {
  uint32_t clkdiv = (32U * clock) / (ovs * baud) - 32U;
  return clkdiv * 8U;
}

static uint32_t uart_get_ovs(uint32_t cfg0, bool is_usart) {
  uint32_t shift = is_usart ? _USART_CTRL_OVS_SHIFT : _EUSART_CFG0_OVS_SHIFT;
  uint32_t mask  = is_usart ? _USART_CTRL_OVS_MASK  : _EUSART_CFG0_OVS_MASK;
  switch ((cfg0 & mask) >> shift) {
    case 0: return 16;
    case 1: return 8;
    case 2: return 6;
    case 3: return 4;
    case 4: return 1;
    default: return 16;
  }
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if EFR32_UART_USE_EUSART1 || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_EUSART1_RX_HANDLER) {
  OSAL_IRQ_PROLOGUE(); uart_lld_serve_interrupt(&UARTD1); OSAL_IRQ_EPILOGUE();
}
OSAL_IRQ_HANDLER(EFR32_EUSART1_TX_HANDLER) {
  OSAL_IRQ_PROLOGUE(); uart_lld_serve_interrupt(&UARTD1); OSAL_IRQ_EPILOGUE();
}
#endif
#if EFR32_UART_USE_EUSART2 || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_EUSART2_RX_HANDLER) {
  OSAL_IRQ_PROLOGUE(); uart_lld_serve_interrupt(&UARTD2); OSAL_IRQ_EPILOGUE();
}
OSAL_IRQ_HANDLER(EFR32_EUSART2_TX_HANDLER) {
  OSAL_IRQ_PROLOGUE(); uart_lld_serve_interrupt(&UARTD2); OSAL_IRQ_EPILOGUE();
}
#endif
#if EFR32_UART_USE_EUSART3 || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_EUSART3_RX_HANDLER) {
  OSAL_IRQ_PROLOGUE(); uart_lld_serve_interrupt(&UARTD3); OSAL_IRQ_EPILOGUE();
}
OSAL_IRQ_HANDLER(EFR32_EUSART3_TX_HANDLER) {
  OSAL_IRQ_PROLOGUE(); uart_lld_serve_interrupt(&UARTD3); OSAL_IRQ_EPILOGUE();
}
#endif
#if EFR32_UART_USE_USART1 || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_USART1_RX_HANDLER) {
  OSAL_IRQ_PROLOGUE(); uart_lld_serve_interrupt(&UARTD4); OSAL_IRQ_EPILOGUE();
}
OSAL_IRQ_HANDLER(EFR32_USART1_TX_HANDLER) {
  OSAL_IRQ_PROLOGUE(); uart_lld_serve_interrupt(&UARTD4); OSAL_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void uart_lld_init(void) {
#if EFR32_UART_USE_EUSART1
  uartObjectInit(&UARTD1);
  UARTD1.usart = EUSART0; UARTD1.is_usart = false; UARTD1.index = 0;
  UARTD1.clock = EFR32_EUSART1CLK;
#endif
#if EFR32_UART_USE_EUSART2
  uartObjectInit(&UARTD2);
  UARTD2.usart = EUSART1; UARTD2.is_usart = false; UARTD2.index = 1;
  UARTD2.clock = EFR32_EUSART23CLK;
#endif
#if EFR32_UART_USE_EUSART3
  uartObjectInit(&UARTD3);
  UARTD3.usart = EUSART2; UARTD3.is_usart = false; UARTD3.index = 2;
  UARTD3.clock = EFR32_EUSART23CLK;
#endif
#if EFR32_UART_USE_USART1
  uartObjectInit(&UARTD4);
  UARTD4.usart = USART0; UARTD4.is_usart = true; UARTD4.index = 3;
  UARTD4.clock = EFR32_USART1CLK;
#endif
}

msg_t uart_lld_start(UARTDriver *uartp, const UARTConfig *config) {
  uint32_t idx = uartp->index;
  uint32_t ovs = uart_get_ovs(config->cfg0, uartp->is_usart);
  uint32_t clkdiv = uart_get_clkdiv(uartp->clock, ovs, config->speed);

  if (uartp->state == UART_STOP) {
    CMU->CLKEN0_SET = (idx == 3U) ? uart_clken[idx] : 0U;
    if (idx != 3U) CMU->CLKEN1_SET = uart_clken[idx];

    if (uartp->is_usart) {
      USART_TypeDef *u = uartp->usart;
      u->CTRL = config->cfg0;
      u->FRAME = config->framecfg;
      u->EN_SET = USART_EN_EN;
      u->CLKDIV = clkdiv;
      u->IF_CLR = u->IF;
      u->CMD_SET = USART_CMD_RXEN | USART_CMD_TXEN | USART_CMD_CLEARTX;
    } else {
      EUSART_TypeDef *u = uartp->usart;
      u->CFG0 = config->cfg0;
      u->FRAMECFG = config->framecfg;
      u->EN_SET = EUSART_EN_EN;
      u->CLKDIV = clkdiv;
      u->IF_CLR = u->IF;
      u->CMD_SET = EUSART_CMD_RXEN | EUSART_CMD_TXEN | EUSART_CMD_CLEARTX;
      while ((u->SYNCBUSY & _EUSART_SYNCBUSY_MASK) != 0U) {}
    }

    nvicEnableVector(uart_irqn_rx[idx], uart_irqprio[idx]);
    nvicEnableVector(uart_irqn_tx[idx], uart_irqprio[idx]);
  }

  return HAL_RET_SUCCESS;
}

void uart_lld_stop(UARTDriver *uartp) {
  uint32_t idx = uartp->index;

  if (uartp->state == UART_READY) {
    if (uartp->is_usart) {
      USART_TypeDef *u = uartp->usart;
      u->CMD_CLR = USART_CMD_RXEN | USART_CMD_TXEN;
      u->IF_CLR = u->IF;
      u->EN_CLR = USART_EN_EN;
    } else {
      EUSART_TypeDef *u = uartp->usart;
      u->CMD_CLR = EUSART_CMD_RXEN | EUSART_CMD_TXEN;
      while ((u->SYNCBUSY & _EUSART_SYNCBUSY_MASK) != 0U) {}
      u->IF_CLR = u->IF;
      u->EN_CLR = EUSART_EN_EN;
    }
    nvicDisableVector(uart_irqn_rx[idx]);
    nvicDisableVector(uart_irqn_tx[idx]);
    if (idx == 3U) CMU->CLKEN0_CLR = uart_clken[idx];
    else CMU->CLKEN1_CLR = uart_clken[idx];
  }
}

void uart_lld_start_send(UARTDriver *uartp, size_t n, const void *txbuf) {
  uartp->txbuf = (const uint8_t *)txbuf;
  uartp->txend = uartp->txbuf + n;

  if (uartp->is_usart) {
    USART_TypeDef *u = uartp->usart;
    u->IEN_SET = USART_IEN_TXBL;
  } else {
    EUSART_TypeDef *u = uartp->usart;
    u->IEN_SET = EUSART_IEN_TXFL;
  }
}

size_t uart_lld_stop_send(UARTDriver *uartp) {
  if (uartp->is_usart) {
    USART_TypeDef *u = uartp->usart;
    u->IEN_CLR = USART_IEN_TXBL;
  } else {
    EUSART_TypeDef *u = uartp->usart;
    u->IEN_CLR = EUSART_IEN_TXFL;
  }
  size_t n = (size_t)(uartp->txend - uartp->txbuf);
  uartp->txbuf = NULL;
  return n;
}

void uart_lld_start_receive(UARTDriver *uartp, size_t n, void *rxbuf) {
  uartp->rxbuf = (uint8_t *)rxbuf;
  uartp->rxend = uartp->rxbuf + n;

  if (uartp->is_usart) {
    USART_TypeDef *u = uartp->usart;
    u->IEN_SET = USART_IEN_RXFULL;
  } else {
    EUSART_TypeDef *u = uartp->usart;
    u->IEN_SET = EUSART_IEN_RXFL;
  }
}

size_t uart_lld_stop_receive(UARTDriver *uartp) {
  if (uartp->is_usart) {
    USART_TypeDef *u = uartp->usart;
    u->IEN_CLR = USART_IEN_RXFULL;
  } else {
    EUSART_TypeDef *u = uartp->usart;
    u->IEN_CLR = EUSART_IEN_RXFL;
  }
  size_t n = (size_t)(uartp->rxend - uartp->rxbuf);
  uartp->rxbuf = NULL;
  return n;
}

void uart_lld_serve_interrupt(UARTDriver *uartp) {
  if (uartp->is_usart) {
    USART_TypeDef *u = uartp->usart;
    uint32_t isr = u->IF;

    if ((isr & USART_IF_RXFULL) != 0U && uartp->rxbuf != NULL) {
      *uartp->rxbuf++ = (uint8_t)u->RXDATA;
      if (uartp->rxbuf >= uartp->rxend) {
        u->IEN_CLR = USART_IEN_RXFULL;
        _uart_wakeup_rx_isr(uartp);
      }
      if (uartp->config->rxchar_cb != NULL) {
        uartp->config->rxchar_cb(uartp, (uint16_t)(*(uartp->rxbuf - 1U)));
      }
    }

    if ((isr & USART_IF_TXBL) != 0U && uartp->txbuf != NULL) {
      u->TXDATA = *uartp->txbuf++;
      if (uartp->txbuf >= uartp->txend) {
        u->IEN_CLR = USART_IEN_TXBL;
        u->IEN_SET = USART_IEN_TXC;
      }
    }

    if ((isr & USART_IF_TXC) != 0U) {
      u->IF_CLR = USART_IF_TXC;
      u->IEN_CLR = USART_IEN_TXC;
      _uart_wakeup_tx2_isr(uartp);
      if (uartp->config->txend2_cb != NULL) {
        uartp->config->txend2_cb(uartp);
      }
    }
  } else {
    EUSART_TypeDef *u = uartp->usart;
    uint32_t isr = u->IF;

    if ((isr & EUSART_IF_RXFL) != 0U && uartp->rxbuf != NULL) {
      *uartp->rxbuf++ = (uint8_t)u->RXDATA;
      if (uartp->rxbuf >= uartp->rxend) {
        u->IEN_CLR = EUSART_IEN_RXFL;
        _uart_wakeup_rx_isr(uartp);
      }
      if (uartp->config->rxchar_cb != NULL) {
        uartp->config->rxchar_cb(uartp, (uint16_t)(*(uartp->rxbuf - 1U)));
      }
    }

    if ((isr & EUSART_IF_TXFL) != 0U && uartp->txbuf != NULL) {
      u->TXDATA = *uartp->txbuf++;
      if (uartp->txbuf >= uartp->txend) {
        u->IEN_CLR = EUSART_IEN_TXFL;
        u->IEN_SET = EUSART_IEN_TXC;
      }
    }

    if ((isr & EUSART_IF_TXC) != 0U) {
      u->IF_CLR = EUSART_IF_TXC;
      u->IEN_CLR = EUSART_IEN_TXC;
      _uart_wakeup_tx2_isr(uartp);
      if (uartp->config->txend2_cb != NULL) {
        uartp->config->txend2_cb(uartp);
      }
    }
  }
}

#endif /* HAL_USE_UART == TRUE */

/** @} */
