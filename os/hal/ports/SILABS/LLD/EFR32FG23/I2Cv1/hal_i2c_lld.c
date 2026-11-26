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
 * @file    I2Cv1/hal_i2c_lld.c
 * @brief   EFR32FG23 I2C subsystem low level driver source.
 *
 * @addtogroup I2C
 * @{
 */

#include "hal.h"

#if (HAL_USE_I2C == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#if !defined(EFR32_DMA_REQUIRED)
#define EFR32_DMA_REQUIRED
#endif

static I2C_TypeDef * const efr32_i2c[2] = { I2C0, I2C1 };
static const uint32_t efr32_i2c_clken[2] = { CMU_CLKEN0_I2C0, CMU_CLKEN0_I2C1 };
static const uint32_t efr32_i2c_irqn[2] = { EFR32_I2C1_NUMBER, EFR32_I2C2_NUMBER };
static const uint32_t efr32_i2c_irqprio[2] = {
  EFR32_I2C0_IRQ_PRIORITY, EFR32_I2C1_IRQ_PRIORITY
};

/* DMA request source (SOURCESEL) per I2C index. */
static const uint32_t efr32_i2c_dma_src[2] = {
  LDMAXBAR_CH_REQSEL_SOURCESEL_I2C0 >> 16,
  LDMAXBAR_CH_REQSEL_SOURCESEL_I2C1 >> 16
};
/* SIGSEL: TXBL=1, RXDATAV=0. */
#define EFR32_I2C_DMA_SIG_TX   1U
#define EFR32_I2C_DMA_SIG_RX   0U

static const int32_t efr32_i2c_tx_dma_stream[2] = {
  EFR32_I2C0_TX_DMA_STREAM, EFR32_I2C1_TX_DMA_STREAM
};
static const int32_t efr32_i2c_rx_dma_stream[2] = {
  EFR32_I2C0_RX_DMA_STREAM, EFR32_I2C1_RX_DMA_STREAM
};
static const uint32_t efr32_i2c_dma_prio[2] = {
  EFR32_I2C0_DMA_IRQ_PRIORITY, EFR32_I2C1_DMA_IRQ_PRIORITY
};

/* I2C clock source is EM01GRPACLK on FG23. */
#define EFR32_I2CCLK    EFR32_EM01GRPACLK

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if EFR32_I2C_USE_I2C0 || defined(__DOXYGEN__)
I2CDriver I2CD1;
#endif
#if EFR32_I2C_USE_I2C1 || defined(__DOXYGEN__)
I2CDriver I2CD2;
#endif

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if EFR32_I2C_USE_I2C0 || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_I2C1_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  i2c_lld_serve_interrupt(&I2CD1);
  OSAL_IRQ_EPILOGUE();
}
#endif
#if EFR32_I2C_USE_I2C1 || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_I2C2_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  i2c_lld_serve_interrupt(&I2CD2);
  OSAL_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static void i2c_lld_wakeup_error(I2CDriver *i2cp, i2cflags_t errors) {
  i2cp->errors = errors;
  osalSysLockFromISR();
  osalThreadResumeI(&i2cp->thread, MSG_RESET);
  osalSysUnlockFromISR();
}

static void i2c_lld_wakeup_done(I2CDriver *i2cp) {
  osalSysLockFromISR();
  osalThreadResumeI(&i2cp->thread, MSG_OK);
  osalSysUnlockFromISR();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void i2c_lld_init(void) {
#if EFR32_I2C_USE_I2C0
  i2cObjectInit(&I2CD1);
  I2CD1.i2c = I2C0; I2CD1.index = 0;
  I2CD1.dma_tx = NULL; I2CD1.dma_rx = NULL; I2CD1.dma_active = 0;
#endif
#if EFR32_I2C_USE_I2C1
  i2cObjectInit(&I2CD2);
  I2CD2.i2c = I2C1; I2CD2.index = 1;
  I2CD2.dma_tx = NULL; I2CD2.dma_rx = NULL; I2CD2.dma_active = 0;
#endif
}

void i2c_lld_start(I2CDriver *i2cp) {
  I2C_TypeDef *i2c = i2cp->i2c;
  uint32_t index = i2cp->index;
  uint32_t clkdiv;

  if (i2cp->state == I2C_STOP) {
    CMU->CLKEN0_SET = efr32_i2c_clken[index];

    /* Calculate clock divider for standard/fast mode.
       SCL = I2CCLK / (clkdiv + 1) with 6:3 duty cycle (fixed).
       clkdiv = I2CCLK / SCL - 1 (approximate). */
    clkdiv = (EFR32_I2CCLK / i2cp->config->clock_speed) - 1U;
    if (clkdiv > _I2C_CLKDIV_DIV_MASK) {
      clkdiv = _I2C_CLKDIV_DIV_MASK;
    }

    i2c->EN_CLR = I2C_EN_EN;

    i2c->CTRL = 0;
    i2c->CLKDIV = clkdiv << _I2C_CLKDIV_DIV_SHIFT;
    i2c->IF_CLR = _I2C_IF_MASK;
    i2c->IEN = 0;

    /* Route SDA/SCL pins (GPIO->I2CxROUTE is owned by this driver). */
    GPIO->I2CROUTE[index].SDAROUTE =
        ((uint32_t)i2cp->config->sda_port << _GPIO_I2C_SDAROUTE_PORT_SHIFT) |
        ((uint32_t)i2cp->config->sda_pin  << _GPIO_I2C_SDAROUTE_PIN_SHIFT);
    GPIO->I2CROUTE[index].SCLROUTE =
        ((uint32_t)i2cp->config->scl_port << _GPIO_I2C_SCLROUTE_PORT_SHIFT) |
        ((uint32_t)i2cp->config->scl_pin  << _GPIO_I2C_SCLROUTE_PIN_SHIFT);
    GPIO->I2CROUTE[index].ROUTEEN =
        GPIO_I2C_ROUTEEN_SDAPEN | GPIO_I2C_ROUTEEN_SCLPEN;

    /* Enable I2C module. */
    i2c->EN_SET = I2C_EN_EN;

    nvicEnableVector(efr32_i2c_irqn[index], efr32_i2c_irqprio[index]);

    /* Allocate DMA streams if configured. */
#if defined(EFR32_DMA_REQUIRED)
    if (efr32_i2c_tx_dma_stream[index] >= 0 && i2cp->dma_tx == NULL) {
      i2cp->dma_tx = dmaStreamAlloc(
          (uint32_t)efr32_i2c_tx_dma_stream[index],
          efr32_i2c_dma_prio[index], NULL, NULL);
    }
    if (efr32_i2c_rx_dma_stream[index] >= 0 && i2cp->dma_rx == NULL) {
      i2cp->dma_rx = dmaStreamAlloc(
          (uint32_t)efr32_i2c_rx_dma_stream[index],
          efr32_i2c_dma_prio[index], NULL, NULL);
    }
#endif
  }
}

void i2c_lld_stop(I2CDriver *i2cp) {
  I2C_TypeDef *i2c = i2cp->i2c;

  if (i2cp->state == I2C_READY) {
#if defined(EFR32_DMA_REQUIRED)
    if (i2cp->dma_tx != NULL) {
      dmaStreamFree(i2cp->dma_tx);
      i2cp->dma_tx = NULL;
    }
    if (i2cp->dma_rx != NULL) {
      dmaStreamFree(i2cp->dma_rx);
      i2cp->dma_rx = NULL;
    }
#endif
    i2c->CMD = I2C_CMD_ABORT;
    i2c->IEN = 0;
    i2c->IF_CLR = _I2C_IF_MASK;
    nvicDisableVector(efr32_i2c_irqn[i2cp->index]);
    GPIO->I2CROUTE_CLR[i2cp->index].ROUTEEN =
        GPIO_I2C_ROUTEEN_SDAPEN | GPIO_I2C_ROUTEEN_SCLPEN;
    i2c->EN_CLR = I2C_EN_EN;
    CMU->CLKEN0_CLR = efr32_i2c_clken[i2cp->index];
  }
}

msg_t i2c_lld_master_transmit_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                       const uint8_t *txbuf, size_t txbytes,
                                       uint8_t *rxbuf, size_t rxbytes,
                                       systime_t timeout) {
  I2C_TypeDef *i2c = i2cp->i2c;
  msg_t msg = MSG_OK;

  i2cp->addr = addr;
  i2cp->txbuf = txbuf;
  i2cp->txlen = txbytes;
  i2cp->txidx = 0;
  i2cp->rxbuf = rxbuf;
  i2cp->rxlen = rxbytes;
  i2cp->rxidx = 0;
  i2cp->errors = 0;

  /* Enable interrupts for the transfer. TXBL is deliberately NOT enabled:
     it is a level flag that stays asserted while TXBUF is empty, so on a
     NACK/error path (thread cannot run to clear IEN) it would re-fire
     forever and starve the kernel (verified on BRD2600A). TX bytes are
     fed on ACK, RX bytes on RXDATAV — TXBL is not needed. */
  i2c->IEN_SET = I2C_IEN_ACK | I2C_IEN_NACK | I2C_IEN_MSTOP |
                  I2C_IEN_ARBLOST | I2C_IEN_RXDATAV;

  /* Send START + address (write). */
  i2c->TXDATA = (addr << 1);
  i2c->CMD = I2C_CMD_START;

  /* Wait for completion. */
  msg = osalThreadSuspendTimeoutS(&i2cp->thread, timeout);

  i2c->IEN = 0;
  i2c->CMD = I2C_CMD_ABORT;

  if (msg == MSG_TIMEOUT) {
    return MSG_TIMEOUT;
  }
  if (i2cp->errors != 0U) {
    return (msg_t)i2cp->errors;
  }
  return MSG_OK;
}

msg_t i2c_lld_master_receive_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                      uint8_t *rxbuf, size_t rxbytes,
                                      systime_t timeout) {
  return i2c_lld_master_transmit_timeout(i2cp, addr, NULL, 0, rxbuf, rxbytes, timeout);
}

void i2c_lld_serve_interrupt(I2CDriver *i2cp) {
  I2C_TypeDef *i2c = i2cp->i2c;
  uint32_t isr = i2c->IF;

  /* TXBL is a level interrupt: while TXBUF is empty it stays asserted and
     re-fires forever, and since the I2C IRQ outranks PendSV/SysTick the
     kernel starves (verified on BRD2600A). This ISR never sends on TXBL
     (TX bytes go out on ACK), so mask it whenever it shows up. */
  if ((isr & I2C_IF_TXBL) != 0U) {
    i2c->IEN_CLR = I2C_IEN_TXBL;
  }

  /* NACK or arbitration lost → error. */
  if ((isr & I2C_IF_NACK) != 0U) {
    i2c->IF_CLR = I2C_IF_NACK;
    i2c->CMD = I2C_CMD_STOP;
    i2c_lld_wakeup_error(i2cp, I2C_ACK_FAILURE);
    return;
  }
  if ((isr & I2C_IF_ARBLOST) != 0U) {
    i2c->IF_CLR = I2C_IF_ARBLOST;
    i2c->CMD = I2C_CMD_ABORT;
    i2c_lld_wakeup_error(i2cp, I2C_ARBITRATION_LOST);
    return;
  }

  /* ACK received. */
  if ((isr & I2C_IF_ACK) != 0U) {
    i2c->IF_CLR = I2C_IF_ACK;

    if (i2cp->dma_active != 0U) {
      /* DMA is handling data bytes. Just ACK and continue. */
      i2c->CMD = I2C_CMD_CONT;
    }
    /* If we have TX data, send next byte. */
    else if (i2cp->txidx < i2cp->txlen) {
      i2c->TXDATA = i2cp->txbuf[i2cp->txidx++];
      i2c->CMD = I2C_CMD_CONT;
    }
    /* If we have RX data to receive, switch to read. */
    else if (i2cp->rxidx < i2cp->rxlen) {
      if (i2cp->rxidx == i2cp->rxlen - 1U) {
        /* Last byte: NACK + STOP. */
        i2c->CMD = I2C_CMD_NACK;
      } else {
        i2c->CMD = I2C_CMD_ACK;
      }
    }
    /* All done. */
    else {
      i2c->CMD = I2C_CMD_STOP;
    }
  }

  /* RX data available (only when not in DMA mode). */
  if ((isr & I2C_IF_RXDATAV) != 0U) {
    if (i2cp->dma_active == 0U) {
      if (i2cp->rxidx < i2cp->rxlen) {
        i2cp->rxbuf[i2cp->rxidx++] = (uint8_t)i2c->RXDATA;
      }
      if (i2cp->rxidx >= i2cp->rxlen && i2cp->txidx >= i2cp->txlen) {
        i2c->IF_CLR = I2C_IF_RXDATAV;
        i2c_lld_wakeup_done(i2cp);
      }
    }
  }

  /* Master STOP sent. */
  if ((isr & I2C_IF_MSTOP) != 0U) {
    i2c->IF_CLR = I2C_IF_MSTOP;
    i2c_lld_wakeup_done(i2cp);
  }
}

/*===========================================================================*/
/* DMA-based transfer functions.                                             */
/*===========================================================================*/

#if defined(EFR32_DMA_REQUIRED) || defined(__DOXYGEN__)

/**
 * @brief   DMA-assisted I2C master transmit/receive.
 * @details Uses DMA for TX (mem→TXDATA on TXBL) and RX (RXDATA→mem on
 *          RXDATAV) to offload the CPU. Address phase and control still
 *          handled via interrupts. Falls back to interrupt-driven if DMA
 *          streams are not allocated.
 */
msg_t i2c_lld_master_transmit_dma_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                           const uint8_t *txbuf, size_t txbytes,
                                           uint8_t *rxbuf, size_t rxbytes,
                                           systime_t timeout) {
  I2C_TypeDef *i2c = i2cp->i2c;
  uint32_t index = i2cp->index;
  msg_t msg = MSG_OK;
  const efr32_dma_stream_t *tx_st = i2cp->dma_tx;
  const efr32_dma_stream_t *rx_st = i2cp->dma_rx;

  if (tx_st == NULL && rx_st == NULL) {
    /* No DMA — use interrupt-driven path. */
    return i2c_lld_master_transmit_timeout(i2cp, addr, txbuf, txbytes,
                                            rxbuf, rxbytes, timeout);
  }

  i2cp->addr = addr;
  i2cp->txbuf = txbuf;
  i2cp->txlen = txbytes;
  i2cp->txidx = 0;
  i2cp->rxbuf = rxbuf;
  i2cp->rxlen = rxbytes;
  i2cp->rxidx = 0;
  i2cp->errors = 0;

  /* Set up TX DMA if available and there is TX data. */
  if (tx_st != NULL && txbytes > 0U) {
    dmaStreamSetRequestSource(tx_st, efr32_i2c_dma_src[index],
                              EFR32_I2C_DMA_SIG_TX);
    dmaStartMemToPeriph(tx_st, txbuf, &i2c->TXDATA, txbytes,
                        EFR32_DMA_CH_CTRL_SIZE_BYTE);
    i2cp->dma_active = 1U;
  }

  /* Set up RX DMA if available and there is RX data. */
  if (rx_st != NULL && rxbytes > 0U) {
    dmaStreamSetRequestSource(rx_st, efr32_i2c_dma_src[index],
                              EFR32_I2C_DMA_SIG_RX);
    dmaStartPeriphToMem(rx_st, &i2c->RXDATA, rxbuf, rxbytes,
                        EFR32_DMA_CH_CTRL_SIZE_BYTE);
    i2cp->dma_active = 1U;
  }

  /* Enable interrupts for ACK/NACK/ARBLOST/MSTOP (DMA handles data). */
  i2c->IEN_SET = I2C_IEN_ACK | I2C_IEN_NACK | I2C_IEN_MSTOP | I2C_IEN_ARBLOST;

  /* Send START + address (write first, then repeated start for read). */
  if (txbytes > 0U) {
    i2c->TXDATA = (addr << 1);  /* Write address. */
  } else {
    i2c->TXDATA = (addr << 1) | 1U;  /* Read address. */
  }
  i2c->CMD = I2C_CMD_START;

  msg = osalThreadSuspendTimeoutS(&i2cp->thread, timeout);

  /* Wait for DMA completion. */
  if (tx_st != NULL && txbytes > 0U) {
    dmaWaitCompletion(tx_st);
    dmaStreamDisable(tx_st);
  }
  if (rx_st != NULL && rxbytes > 0U) {
    dmaWaitCompletion(rx_st);
    dmaStreamDisable(rx_st);
  }
  i2cp->dma_active = 0U;

  i2c->IEN = 0;
  i2c->CMD = I2C_CMD_ABORT;

  if (msg == MSG_TIMEOUT) {
    return MSG_TIMEOUT;
  }
  if (i2cp->errors != 0U) {
    return (msg_t)i2cp->errors;
  }
  return MSG_OK;
}

msg_t i2c_lld_master_receive_dma_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                          uint8_t *rxbuf, size_t rxbytes,
                                          systime_t timeout) {
  return i2c_lld_master_transmit_dma_timeout(i2cp, addr, NULL, 0,
                                             rxbuf, rxbytes, timeout);
}

#endif /* EFR32_DMA_REQUIRED */

#endif /* HAL_USE_I2C == TRUE */

/** @} */
