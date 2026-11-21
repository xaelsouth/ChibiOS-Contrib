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
 * @file    IMX23/IMX23xx/hal_sio_lld.c
 * @brief   i.MX23 SIO subsystem low level driver code (PL011 DUART).
 *
 * @addtogroup IMX23
 * @{
 */

#include "hal.h"
#include "icoll.h"

#if (HAL_USE_SIO == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

#if (IMX23_SIO_USE_DUART == TRUE) && !defined(__DOXYGEN__)
SIODriver SIOD1;
#endif

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module interrupt handlers.                                                */
/*===========================================================================*/

#if (IMX23_SIO_USE_DUART == TRUE) && !defined(__DOXYGEN__)
CH_IRQ_HANDLER(IMX23_DUART_IRQHandler) {

  CH_IRQ_PROLOGUE();

  sio_lld_serve_interrupt(&SIOD1);

  CH_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level SIO driver initialization.
 *
 * @notapi
 */
void sio_lld_init(void) {

#if (IMX23_SIO_USE_DUART == TRUE) && !defined(__DOXYGEN__)
  sioObjectInit(&SIOD1);
  SIOD1.uart = IMX23_PL011;
#endif
}

/**
 * @brief   Configures and activates a SIO driver.
 *
 * @param[in] siop      pointer to a @p SIODriver object
 * @return              The operation status.
 *
 * @notapi
 */
msg_t sio_lld_start(SIODriver *siop) {
  imx23_pl011_t *u = siop->uart;
  uint32_t clk, baud, ibrd, rem, fbrd;

  /* Enable the DUART clock (24MHz XTAL, divider 1). */
  IMX23_REG_CLR(IMX23_CLKCTRL_XTAL, IMX23_CLKCTRL_XTAL_UART_CLK_GATE);
  IMX23_REG(IMX23_CLKCTRL_XTAL) &= ~IMX23_CLKCTRL_XTAL_DIV_UART;

  clk  = IMX23_DUART_CLK_HZ;
  baud = siop->config->baud;
  if (baud == 0U) {
    baud = 115200U;
  }

  /* PL011 baud rate divisors: bauddiv = IBRD + FBRD/64 = clk/(16*baud). */
  ibrd = clk / (16U * baud);
  rem  = clk % (16U * baud);
  fbrd = (rem * 4U + (baud / 2U)) / baud;   /* (rem*64)/(16*baud), rounded */

  /* Disable while reconfiguring. */
  u->CR = 0U;
  u->ICR = 0x7FFU;                          /* clear all interrupt flags */

  u->IBRD   = ibrd;
  u->FBRD   = fbrd;
  u->LCR_H  = siop->config->framecfg;
  u->IFLS   = 0x12U;                        /* RX/TX FIFO trigger 1/8      */
  u->IMSC   = IMX23_PL011_RIS_RX | IMX23_PL011_RIS_RT;
  u->CR     = IMX23_PL011_CR_UARTEN | IMX23_PL011_CR_RXE | IMX23_PL011_CR_TXE;

#if (IMX23_SIO_USE_DUART == TRUE) && !defined(__DOXYGEN__)
  if (siop == &SIOD1) {
    icoll_register_handler(IMX23_IRQ_DUART, IMX23_DUART_IRQHandler);
    icoll_irq_set_priority(IMX23_IRQ_DUART, 2U);
    icoll_irq_enable(IMX23_IRQ_DUART);
  }
#endif

  return MSG_OK;
}

/**
 * @brief   Deactivates a SIO driver.
 *
 * @param[in] siop      pointer to a @p SIODriver object
 *
 * @notapi
 */
void sio_lld_stop(SIODriver *siop) {
  imx23_pl011_t *u = siop->uart;

  if (siop->state == SIO_READY) {
    siop->state = SIO_STOP;
    u->CR   = 0U;
    u->IMSC = 0U;
    u->ICR  = 0x7FFU;
#if (IMX23_SIO_USE_DUART == TRUE) && !defined(__DOXYGEN__)
    if (siop == &SIOD1) {
      icoll_irq_disable(IMX23_IRQ_DUART);
    }
#endif
  }
}

/**
 * @brief   Updates the enabled interrupts flags in the hardware.
 *
 * @notapi
 */
void sio_lld_update_enable_flags(SIODriver *siop) {
  imx23_pl011_t *u = siop->uart;
  uint32_t imsc = 0U;

  if ((siop->enabled & SIO_EV_RXNOTEMPY) != 0U) {
    imsc |= IMX23_PL011_RIS_RX;
  }
  if ((siop->enabled & SIO_EV_RXIDLE) != 0U) {
    imsc |= IMX23_PL011_RIS_RT;
  }
  if ((siop->enabled & SIO_EV_TXNOTFULL) != 0U) {
    imsc |= IMX23_PL011_RIS_TX;
  }
  u->IMSC = imsc;
}

/**
 * @brief   Returns pending errors and clears their flags.
 *
 * @notapi
 */
sioevents_t sio_lld_get_and_clear_errors(SIODriver *siop) {
  imx23_pl011_t *u = siop->uart;
  uint32_t ris = u->RIS & IMX23_PL011_RIS_ERRORS;
  sioevents_t events;

  u->ICR = ris;

  events = __sio_reloc_field(ris, IMX23_PL011_RIS_PE, 1, SIO_EV_PARITY_ERR_POS) |
           __sio_reloc_field(ris, IMX23_PL011_RIS_FE, 1, SIO_EV_FRAMING_ERR_POS) |
           __sio_reloc_field(ris, IMX23_PL011_RIS_OE, 1, SIO_EV_OVERRUN_ERR_POS);

  return events;
}

/**
 * @brief   Returns pending events and clears their flags.
 *
 * @notapi
 */
sioevents_t sio_lld_get_and_clear_events(SIODriver *siop) {
  sioevents_t events = sio_lld_get_events(siop);

  if (events != 0U) {
    imx23_pl011_t *u = siop->uart;
    uint32_t clr = 0U;

    if ((events & SIO_EV_RXNOTEMPY) != 0U)    clr |= IMX23_PL011_RIS_RX;
    if ((events & SIO_EV_RXIDLE) != 0U)       clr |= IMX23_PL011_RIS_RT;
    if ((events & SIO_EV_TXNOTFULL) != 0U)    clr |= IMX23_PL011_RIS_TX;
    if ((events & SIO_EV_ALL_ERRORS) != 0U)   clr |= IMX23_PL011_RIS_ERRORS;
    u->ICR = clr;
  }

  return events;
}

/**
 * @brief   Returns the events currently pending.
 *
 * @notapi
 */
sioevents_t sio_lld_get_events(SIODriver *siop) {
  imx23_pl011_t *u = siop->uart;
  uint32_t ris = u->RIS;
  uint32_t fr  = u->FR;
  sioevents_t events;

  events = __sio_reloc_field(ris, IMX23_PL011_RIS_RX, 1, SIO_EV_RXNOTEMPY_POS) |
           __sio_reloc_field(ris, IMX23_PL011_RIS_RT, 1, SIO_EV_RXIDLE_POS) |
           __sio_reloc_field(ris, IMX23_PL011_RIS_TX, 1, SIO_EV_TXNOTFULL_POS) |
           __sio_reloc_field(ris, IMX23_PL011_RIS_PE, 1, SIO_EV_PARITY_ERR_POS) |
           __sio_reloc_field(ris, IMX23_PL011_RIS_FE, 1, SIO_EV_FRAMING_ERR_POS) |
           __sio_reloc_field(ris, IMX23_PL011_RIS_OE, 1, SIO_EV_OVERRUN_ERR_POS);

  /* PL011 has no TX-complete interrupt: report TXDONE when the TX FIFO is
     empty and the shifter is not busy. */
  if ((fr & (IMX23_PL011_FR_TXFE | IMX23_PL011_FR_BUSY)) == IMX23_PL011_FR_TXFE) {
    events |= SIO_EV_TXDONE;
  }

  return events;
}

/**
 * @brief   Reads from the RX FIFO.
 *
 * @notapi
 */
size_t sio_lld_read(SIODriver *siop, uint8_t *buffer, size_t n) {
  imx23_pl011_t *u = siop->uart;
  size_t rd = 0U;

  while (rd < n) {
    if ((u->FR & IMX23_PL011_FR_RXFE) != 0U) {
      break;
    }
    *buffer++ = (uint8_t)(u->DR & 0xFFU);
    rd++;
  }

  return rd;
}

/**
 * @brief   Writes to the TX FIFO.
 *
 * @notapi
 */
size_t sio_lld_write(SIODriver *siop, const uint8_t *buffer, size_t n) {
  imx23_pl011_t *u = siop->uart;
  size_t wr = 0U;

  while (wr < n) {
    if ((u->FR & IMX23_PL011_FR_TXFF) != 0U) {
      break;
    }
    u->DR = *buffer++;
    wr++;
  }

  return wr;
}

/**
 * @brief   Returns a byte from the RX FIFO, blocking not allowed.
 *
 * @notapi
 */
msg_t sio_lld_get(SIODriver *siop) {
  imx23_pl011_t *u = siop->uart;

  if ((u->FR & IMX23_PL011_FR_RXFE) != 0U) {
    return MSG_RESET;
  }
  return (msg_t)(u->DR & 0xFFU);
}

/**
 * @brief   Pushes a byte into the TX FIFO.
 *
 * @notapi
 */
void sio_lld_put(SIODriver *siop, uint_fast16_t data) {
  imx23_pl011_t *u = siop->uart;

  while ((u->FR & IMX23_PL011_FR_TXFF) != 0U) {
  }
  u->DR = (uint32_t)data;
}

/**
 * @brief   SIO control operation (not implemented).
 *
 * @notapi
 */
msg_t sio_lld_control(SIODriver *siop, unsigned int operation, void *arg) {

  (void)siop;
  (void)operation;
  (void)arg;
  return MSG_OK;
}

/**
 * @brief   Serves a SIO interrupt.
 *
 * @notapi
 */
void sio_lld_serve_interrupt(SIODriver *siop) {
  imx23_pl011_t *u = siop->uart;
  sioevents_t events;

  osalDbgAssert(siop->state == SIO_READY, "invalid state");

  events = sio_lld_get_events(siop) & siop->enabled;
  if (events != 0U) {

    /* Errors handled as a group. */
    if ((events & SIO_EV_ALL_ERRORS) != 0U) {
      u->ICR = IMX23_PL011_RIS_ERRORS;
      __sio_wakeup_errors(siop);
    }
    else {
      if ((events & SIO_EV_RXNOTEMPY) != 0U) {
        u->ICR = IMX23_PL011_RIS_RX;
        __sio_wakeup_rx(siop);
      }
      if ((events & SIO_EV_RXIDLE) != 0U) {
        u->ICR = IMX23_PL011_RIS_RT;
        __sio_wakeup_rxidle(siop);
      }
    }

    if ((events & SIO_EV_TXNOTFULL) != 0U) {
      u->ICR = IMX23_PL011_RIS_TX;
      __sio_wakeup_tx(siop);
    }
    if ((events & SIO_EV_TXDONE) != 0U) {
      __sio_wakeup_txend(siop);
    }

    __sio_callback(siop);
  }
}

/**
 * @brief   RX FIFO empty check.
 *
 * @notapi
 */
bool _sio_lld_is_rx_empty(SIODriver *siop) {

  return (siop->uart->FR & IMX23_PL011_FR_RXFE) != 0U;
}

/**
 * @brief   RX idle check.
 *
 * @notapi
 */
bool _sio_lld_is_rx_idle(SIODriver *siop) {

  return (siop->uart->FR & IMX23_PL011_FR_RXFE) != 0U;
}

/**
 * @brief   RX errors pending check.
 *
 * @notapi
 */
bool _sio_lld_has_rx_errors(SIODriver *siop) {

  return (siop->uart->RIS & IMX23_PL011_RIS_ERRORS) != 0U;
}

/**
 * @brief   TX FIFO full check.
 *
 * @notapi
 */
bool _sio_lld_is_tx_full(SIODriver *siop) {

  return (siop->uart->FR & IMX23_PL011_FR_TXFF) != 0U;
}

/**
 * @brief   TX ongoing check.
 *
 * @notapi
 */
bool _sio_lld_is_tx_ongoing(SIODriver *siop) {

  return (siop->uart->FR & IMX23_PL011_FR_BUSY) != 0U;
}

#endif /* HAL_USE_SIO == TRUE */

/** @} */
