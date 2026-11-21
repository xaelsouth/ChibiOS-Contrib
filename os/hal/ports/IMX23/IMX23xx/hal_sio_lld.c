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
 * @file    IMX23/IMX23xx/hal_sio_lld.c
 * @brief   i.MX23 SIO subsystem low level driver code (PL011 DUART + AUARTs).
 *
 * @details SIOD1 = PL011 debug UART (0x80070000, IRQ 0), SIOD2 = AUART0
 *          (0x8006C000, IRQ 24), SIOD3 = AUART1 (0x8006E000, IRQ 59).
 *          AUART register map/bits from Linux drivers/tty/serial/mxs-auart.c.
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
#if (IMX23_SIO_USE_AUART0 == TRUE) && !defined(__DOXYGEN__)
SIODriver SIOD2;
#endif
#if (IMX23_SIO_USE_AUART1 == TRUE) && !defined(__DOXYGEN__)
SIODriver SIOD3;
#endif

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

static inline imx23_pl011_t *pl011(SIODriver *siop) {
  return (imx23_pl011_t *)siop->u;
}
static inline imx23_auart_t *auart(SIODriver *siop) {
  return (imx23_auart_t *)siop->u;
}

/* RX FIFO empty. */
static inline bool uart_rx_empty(SIODriver *siop) {
  if (siop->siotype == IMX23_SIO_DUART)
    return (pl011(siop)->FR & IMX23_PL011_FR_RXFE) != 0U;
  return (auart(siop)->STAT & IMX23_AUART_STAT_RXFE) != 0U;
}

/* TX FIFO full. */
static inline bool uart_tx_full(SIODriver *siop) {
  if (siop->siotype == IMX23_SIO_DUART)
    return (pl011(siop)->FR & IMX23_PL011_FR_TXFF) != 0U;
  return (auart(siop)->STAT & IMX23_AUART_STAT_TXFF) != 0U;
}

/* Transmitter busy (shift register). */
static inline bool uart_busy(SIODriver *siop) {
  if (siop->siotype == IMX23_SIO_DUART)
    return (pl011(siop)->FR & IMX23_PL011_FR_BUSY) != 0U;
  return (auart(siop)->STAT & IMX23_AUART_STAT_BUSY) != 0U;
}

/* Pending error flags (raw). */
static inline uint32_t uart_errors(SIODriver *siop) {
  if (siop->siotype == IMX23_SIO_DUART)
    return pl011(siop)->RIS & IMX23_PL011_RIS_ERRORS;
  return auart(siop)->STAT & IMX23_AUART_STAT_ERRORS;
}

/* Clear error flags. */
static inline void uart_clear_errors(SIODriver *siop, uint32_t e) {
  if (siop->siotype == IMX23_SIO_DUART)
    pl011(siop)->ICR = e;
  else
    auart(siop)->INTR = e;   /* write-1-to-clear */
}

/* Raw event bits: RX / TX / RT. */
static inline void uart_events_raw(SIODriver *siop, uint32_t *rx, uint32_t *tx,
                                   uint32_t *rt, uint32_t *err) {
  if (siop->siotype == IMX23_SIO_DUART) {
    imx23_pl011_t *u = pl011(siop);
    *rx  = u->RIS & IMX23_PL011_RIS_RX;
    *tx  = u->RIS & IMX23_PL011_RIS_TX;
    *rt  = u->RIS & IMX23_PL011_RIS_RT;
    *err = u->RIS & IMX23_PL011_RIS_ERRORS;
  }
  else {
    imx23_auart_t *u = auart(siop);
    *rx  = u->INTR & IMX23_AUART_INTR_RXIS;
    *tx  = u->INTR & IMX23_AUART_INTR_TXIS;
    *rt  = u->INTR & IMX23_AUART_INTR_RTIS;
    *err = u->STAT & IMX23_AUART_STAT_ERRORS;
  }
}

/* Clear event bits. */
static inline void uart_clear_events(SIODriver *siop, uint32_t rx, uint32_t tx,
                                     uint32_t rt, uint32_t err) {
  if (siop->siotype == IMX23_SIO_DUART) {
    pl011(siop)->ICR = rx | tx | rt | err;
  }
  else {
    auart(siop)->INTR = rx | tx | rt;    /* status bits are write-1-to-clear */
    if (err != 0U) {
      auart(siop)->INTR = err;
    }
  }
}

/* Read one byte from the RX FIFO. */
static inline uint32_t uart_read(SIODriver *siop) {
  if (siop->siotype == IMX23_SIO_DUART)
    return pl011(siop)->DR & 0xFFU;
  return auart(siop)->DATA & 0xFFU;
}

/* Write one byte to the TX FIFO. */
static inline void uart_write(SIODriver *siop, uint32_t b) {
  if (siop->siotype == IMX23_SIO_DUART)
    pl011(siop)->DR = b;
  else
    auart(siop)->DATA = b;
}

/* Set the interrupt mask. */
static void uart_set_mask(SIODriver *siop, uint32_t mask) {
  if (siop->siotype == IMX23_SIO_DUART) {
    pl011(siop)->IMSC = mask;
  }
  else {
    /* INTR: enable bits at +4 (SET), keep status bits cleared. */
    imx23_auart_t *u = auart(siop);
    uint32_t en = 0U;
    if (mask & IMX23_PL011_RIS_RX) en |= IMX23_AUART_INTR_RXIEN;
    if (mask & IMX23_PL011_RIS_TX) en |= IMX23_AUART_INTR_TXIEN;
    if (mask & IMX23_PL011_RIS_RT) en |= IMX23_AUART_INTR_RTIEN;
    IMX23_REG((uint32_t)(uintptr_t)&u->INTR + 8U) = 0xFFFFFFFFU;  /* CLR all enables */
    IMX23_REG((uint32_t)(uintptr_t)&u->INTR + 4U) = en;            /* SET selected */
  }
}

static inline unsigned uart_irq(SIODriver *siop) {
  switch (siop->siotype) {
  case IMX23_SIO_AUART0: return IMX23_IRQ_AUART0;
  case IMX23_SIO_AUART1: return IMX23_IRQ_AUART1;
  default:               return IMX23_IRQ_DUART;
  }
}

/*===========================================================================*/
/* Module interrupt handlers.                                                */
/*===========================================================================*/

#if !defined(__DOXYGEN__)
#define SIO_IRQ_HANDLER(name)                                               \
CH_IRQ_HANDLER(name) {                                                      \
  CH_IRQ_PROLOGUE();                                                        \
  sio_lld_serve_interrupt(&SIO_##name);                                     \
  CH_IRQ_EPILOGUE();                                                        \
}
#endif

/* The handlers are instantiated per enabled channel below. */

#if (IMX23_SIO_USE_DUART == TRUE) && !defined(__DOXYGEN__)
CH_IRQ_HANDLER(IMX23_DUART_IRQHandler) {
  CH_IRQ_PROLOGUE();
  sio_lld_serve_interrupt(&SIOD1);
  CH_IRQ_EPILOGUE();
}
#endif
#if (IMX23_SIO_USE_AUART0 == TRUE) && !defined(__DOXYGEN__)
CH_IRQ_HANDLER(IMX23_AUART0_IRQHandler) {
  CH_IRQ_PROLOGUE();
  sio_lld_serve_interrupt(&SIOD2);
  CH_IRQ_EPILOGUE();
}
#endif
#if (IMX23_SIO_USE_AUART1 == TRUE) && !defined(__DOXYGEN__)
CH_IRQ_HANDLER(IMX23_AUART1_IRQHandler) {
  CH_IRQ_PROLOGUE();
  sio_lld_serve_interrupt(&SIOD3);
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
  SIOD1.siotype = IMX23_SIO_DUART;
  SIOD1.u = (volatile void *)IMX23_PL011;
#endif
#if (IMX23_SIO_USE_AUART0 == TRUE) && !defined(__DOXYGEN__)
  sioObjectInit(&SIOD2);
  SIOD2.siotype = IMX23_SIO_AUART0;
  SIOD2.u = (volatile void *)IMX23_AUART0;
#endif
#if (IMX23_SIO_USE_AUART1 == TRUE) && !defined(__DOXYGEN__)
  sioObjectInit(&SIOD3);
  SIOD3.siotype = IMX23_SIO_AUART1;
  SIOD3.u = (volatile void *)IMX23_AUART1;
#endif
}

/**
 * @brief   Configures and activates a SIO driver.
 *
 * @notapi
 */
msg_t sio_lld_start(SIODriver *siop) {
  uint32_t baud = siop->config->baud;

  if (baud == 0U) {
    baud = 115200U;
  }

  /* UART clock: 24MHz XTAL, divider 1 (shared gate with the DUART). */
  IMX23_REG_CLR(IMX23_CLKCTRL_XTAL, IMX23_CLKCTRL_XTAL_UART_CLK_GATE);
  IMX23_REG(IMX23_CLKCTRL_XTAL) &= ~IMX23_CLKCTRL_XTAL_DIV_UART;

  if (siop->siotype == IMX23_SIO_DUART) {
    imx23_pl011_t *u = pl011(siop);
    uint32_t clk = IMX23_DUART_CLK_HZ;
    uint32_t ibrd = clk / (16U * baud);
    uint32_t rem  = clk % (16U * baud);
    uint32_t fbrd = (rem * 4U + (baud / 2U)) / baud;

    u->CR = 0U;
    u->ICR = 0x7FFU;
    u->IBRD  = ibrd;
    u->FBRD  = fbrd;
    u->LCR_H = siop->config->framecfg;
    u->IFLS  = 0x12U;
    u->IMSC  = IMX23_PL011_RIS_RX | IMX23_PL011_RIS_RT;
    u->CR    = IMX23_PL011_CR_UARTEN | IMX23_PL011_CR_RXE | IMX23_PL011_CR_TXE;
  }
  else {
    imx23_auart_t *u = auart(siop);
    uint32_t clk = IMX23_DUART_CLK_HZ;
    uint32_t div = (clk * 32U + baud / 2U) / baud;   /* DIVINT/DIVFRAC form */

    u->CTRL0 = IMX23_AUART_CTRL0_SFTRST;
    while ((u->CTRL0 & IMX23_AUART_CTRL0_SFTRST) != 0U) {
    }
    IMX23_REG((uint32_t)(uintptr_t)&u->CTRL0 + 8U) = IMX23_AUART_CTRL0_CLKGATE;

    u->LINECTRL = siop->config->framecfg |
                  IMX23_AUART_LINECTRL_BAUD_DIVINT(div >> 6) |
                  IMX23_AUART_LINECTRL_BAUD_DIVFRAC(div & 0x3FU);
    u->INTR = 0x7FFU;    /* clear all status bits */
    u->CTRL2 = IMX23_AUART_CTRL2_UARTEN | IMX23_AUART_CTRL2_TXE |
               IMX23_AUART_CTRL2_RXE;
    uart_set_mask(siop, IMX23_PL011_RIS_RX | IMX23_PL011_RIS_RT);
  }

  {
    unsigned irq = uart_irq(siop);
    bool (*h)(void) = (siop->siotype == IMX23_SIO_DUART) ? IMX23_DUART_IRQHandler :
                       (siop->siotype == IMX23_SIO_AUART0) ? IMX23_AUART0_IRQHandler :
                                                              IMX23_AUART1_IRQHandler;
    icoll_register_handler(irq, h);
    icoll_irq_set_priority(irq, 2U);
    icoll_irq_enable(irq);
  }

  return MSG_OK;
}

/**
 * @brief   Deactivates a SIO driver.
 *
 * @notapi
 */
void sio_lld_stop(SIODriver *siop) {

  if (siop->state == SIO_READY) {
    siop->state = SIO_STOP;
    if (siop->siotype == IMX23_SIO_DUART) {
      imx23_pl011_t *u = pl011(siop);
      u->CR = 0U;
      u->IMSC = 0U;
      u->ICR = 0x7FFU;
    }
    else {
      imx23_auart_t *u = auart(siop);
      u->CTRL2 = 0U;
      uart_set_mask(siop, 0U);
      u->INTR = 0x7FFU;
    }
    icoll_irq_disable(uart_irq(siop));
  }
}

/**
 * @brief   Updates the enabled interrupts flags in the hardware.
 *
 * @notapi
 */
void sio_lld_update_enable_flags(SIODriver *siop) {
  uint32_t mask = 0U;

  if ((siop->enabled & SIO_EV_RXNOTEMPY) != 0U) mask |= IMX23_PL011_RIS_RX;
  if ((siop->enabled & SIO_EV_RXIDLE) != 0U)    mask |= IMX23_PL011_RIS_RT;
  if ((siop->enabled & SIO_EV_TXNOTFULL) != 0U) mask |= IMX23_PL011_RIS_TX;
  uart_set_mask(siop, mask);
}

/**
 * @brief   Returns pending errors and clears their flags.
 *
 * @notapi
 */
sioevents_t sio_lld_get_and_clear_errors(SIODriver *siop) {
  uint32_t e = uart_errors(siop);
  sioevents_t events;

  uart_clear_errors(siop, e);

  if (siop->siotype == IMX23_SIO_DUART) {
    events = __sio_reloc_field(e, IMX23_PL011_RIS_PE, 1, SIO_EV_PARITY_ERR_POS) |
             __sio_reloc_field(e, IMX23_PL011_RIS_FE, 1, SIO_EV_FRAMING_ERR_POS) |
             __sio_reloc_field(e, IMX23_PL011_RIS_OE, 1, SIO_EV_OVERRUN_ERR_POS);
  }
  else {
    events = __sio_reloc_field(e, IMX23_AUART_STAT_PERR, 1, SIO_EV_PARITY_ERR_POS) |
             __sio_reloc_field(e, IMX23_AUART_STAT_FERR, 1, SIO_EV_FRAMING_ERR_POS) |
             __sio_reloc_field(e, IMX23_AUART_STAT_OERR, 1, SIO_EV_OVERRUN_ERR_POS);
  }
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
    uint32_t rx = 0, tx = 0, rt = 0, err = 0;
    if ((events & SIO_EV_RXNOTEMPY) != 0U) {
      if (siop->siotype == IMX23_SIO_DUART) rx = IMX23_PL011_RIS_RX; else rx = IMX23_AUART_INTR_RXIS;
    }
    if ((events & SIO_EV_TXNOTFULL) != 0U) {
      if (siop->siotype == IMX23_SIO_DUART) tx = IMX23_PL011_RIS_TX; else tx = IMX23_AUART_INTR_TXIS;
    }
    if ((events & SIO_EV_RXIDLE) != 0U) {
      if (siop->siotype == IMX23_SIO_DUART) rt = IMX23_PL011_RIS_RT; else rt = IMX23_AUART_INTR_RTIS;
    }
    if ((events & SIO_EV_ALL_ERRORS) != 0U) err = uart_errors(siop);
    uart_clear_events(siop, rx, tx, rt, err);
  }
  return events;
}

/**
 * @brief   Returns the events currently pending.
 *
 * @notapi
 */
sioevents_t sio_lld_get_events(SIODriver *siop) {
  uint32_t rx, tx, rt, err;
  sioevents_t events;

  uart_events_raw(siop, &rx, &tx, &rt, &err);

  events = __sio_reloc_field(rx, 1, 0, SIO_EV_RXNOTEMPY_POS) |
           __sio_reloc_field(rt, 1, 0, SIO_EV_RXIDLE_POS) |
           __sio_reloc_field(tx, 1, 0, SIO_EV_TXNOTFULL_POS);

  if (siop->siotype == IMX23_SIO_DUART) {
    events |= __sio_reloc_field(err, IMX23_PL011_RIS_PE, 1, SIO_EV_PARITY_ERR_POS) |
              __sio_reloc_field(err, IMX23_PL011_RIS_FE, 1, SIO_EV_FRAMING_ERR_POS) |
              __sio_reloc_field(err, IMX23_PL011_RIS_OE, 1, SIO_EV_OVERRUN_ERR_POS);
  }
  else {
    events |= __sio_reloc_field(err, IMX23_AUART_STAT_PERR, 1, SIO_EV_PARITY_ERR_POS) |
              __sio_reloc_field(err, IMX23_AUART_STAT_FERR, 1, SIO_EV_FRAMING_ERR_POS) |
              __sio_reloc_field(err, IMX23_AUART_STAT_OERR, 1, SIO_EV_OVERRUN_ERR_POS);
  }

  /* TX done: FIFO empty and shifter not busy. */
  if (!uart_busy(siop) && !uart_tx_full(siop)) {
    if (siop->siotype == IMX23_SIO_DUART) {
      if ((pl011(siop)->FR & IMX23_PL011_FR_TXFE) != 0U) events |= SIO_EV_TXDONE;
    }
    else {
      if ((auart(siop)->STAT & IMX23_AUART_STAT_TXFE) != 0U) events |= SIO_EV_TXDONE;
    }
  }

  return events;
}

/**
 * @brief   Reads from the RX FIFO.
 *
 * @notapi
 */
size_t sio_lld_read(SIODriver *siop, uint8_t *buffer, size_t n) {
  size_t rd = 0U;

  while (rd < n && !uart_rx_empty(siop)) {
    *buffer++ = (uint8_t)uart_read(siop);
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
  size_t wr = 0U;

  while (wr < n && !uart_tx_full(siop)) {
    uart_write(siop, *buffer++);
    wr++;
  }
  return wr;
}

/**
 * @brief   Returns a byte from the RX FIFO.
 *
 * @notapi
 */
msg_t sio_lld_get(SIODriver *siop) {

  if (uart_rx_empty(siop)) {
    return MSG_RESET;
  }
  return (msg_t)uart_read(siop);
}

/**
 * @brief   Pushes a byte into the TX FIFO.
 *
 * @notapi
 */
void sio_lld_put(SIODriver *siop, uint_fast16_t data) {

  while (uart_tx_full(siop)) {
  }
  uart_write(siop, (uint32_t)data);
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
  sioevents_t events;

  osalDbgAssert(siop->state == SIO_READY, "invalid state");

  events = sio_lld_get_events(siop) & siop->enabled;
  if (events != 0U) {
    uint32_t rx = 0, tx = 0, rt = 0;

    if ((events & SIO_EV_ALL_ERRORS) != 0U) {
      uint32_t err = uart_errors(siop);
      uart_clear_errors(siop, err);
      __sio_wakeup_errors(siop);
    }
    else {
      if ((events & SIO_EV_RXNOTEMPY) != 0U) {
        rx = (siop->siotype == IMX23_SIO_DUART) ? IMX23_PL011_RIS_RX : IMX23_AUART_INTR_RXIS;
        __sio_wakeup_rx(siop);
      }
      if ((events & SIO_EV_RXIDLE) != 0U) {
        rt = (siop->siotype == IMX23_SIO_DUART) ? IMX23_PL011_RIS_RT : IMX23_AUART_INTR_RTIS;
        __sio_wakeup_rxidle(siop);
      }
    }
    if ((events & SIO_EV_TXNOTFULL) != 0U) {
      tx = (siop->siotype == IMX23_SIO_DUART) ? IMX23_PL011_RIS_TX : IMX23_AUART_INTR_TXIS;
      __sio_wakeup_tx(siop);
    }
    if ((events & SIO_EV_TXDONE) != 0U) {
      __sio_wakeup_txend(siop);
    }

    uart_clear_events(siop, rx, tx, rt, 0U);
    __sio_callback(siop);
  }
}

/**
 * @brief   RX FIFO empty check.
 *
 * @notapi
 */
bool _sio_lld_is_rx_empty(SIODriver *siop) {

  return uart_rx_empty(siop);
}

/**
 * @brief   RX idle check.
 *
 * @notapi
 */
bool _sio_lld_is_rx_idle(SIODriver *siop) {

  return uart_rx_empty(siop);
}

/**
 * @brief   RX errors pending check.
 *
 * @notapi
 */
bool _sio_lld_has_rx_errors(SIODriver *siop) {

  return uart_errors(siop) != 0U;
}

/**
 * @brief   TX FIFO full check.
 *
 * @notapi
 */
bool _sio_lld_is_tx_full(SIODriver *siop) {

  return uart_tx_full(siop);
}

/**
 * @brief   TX ongoing check.
 *
 * @notapi
 */
bool _sio_lld_is_tx_ongoing(SIODriver *siop) {

  return uart_busy(siop);
}

#endif /* HAL_USE_SIO == TRUE */

/** @} */
