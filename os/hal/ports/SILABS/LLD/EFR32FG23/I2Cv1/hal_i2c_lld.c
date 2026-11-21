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

static I2C_TypeDef * const efr32_i2c[2] = { I2C0, I2C1 };
static const uint32_t efr32_i2c_clken[2] = { CMU_CLKEN0_I2C0, CMU_CLKEN0_I2C1 };
static const uint32_t efr32_i2c_irqn[2] = { I2C0_IRQn, I2C1_IRQn };
static const uint32_t efr32_i2c_irqprio[2] = {
  EFR32_I2C0_IRQ_PRIORITY, EFR32_I2C1_IRQ_PRIORITY
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
OSAL_IRQ_HANDLER(I2C0_IRQHandler) {
  OSAL_IRQ_PROLOGUE();
  i2c_lld_serve_interrupt(&I2CD1);
  OSAL_IRQ_EPILOGUE();
}
#endif
#if EFR32_I2C_USE_I2C1 || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(I2C1_IRQHandler) {
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
  osalThreadResumeI(&i2cp->thread, MSG_RESET);
}

static void i2c_lld_wakeup_done(I2CDriver *i2cp) {
  osalThreadResumeI(&i2cp->thread, MSG_OK);
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void i2c_lld_init(void) {
#if EFR32_I2C_USE_I2C0
  i2cObjectInit(&I2CD1);
  I2CD1.i2c = I2C0; I2CD1.index = 0;
#endif
#if EFR32_I2C_USE_I2C1
  i2cObjectInit(&I2CD2);
  I2CD2.i2c = I2C1; I2CD2.index = 1;
#endif
}

void i2c_lld_start(I2CDriver *i2cp, const I2CConfig *config) {
  I2C_TypeDef *i2c = i2cp->i2c;
  uint32_t index = i2cp->index;
  uint32_t clkdiv;

  if (i2cp->state == I2C_STOP) {
    CMU->CLKEN0_SET = efr32_i2c_clken[index];

    /* Calculate clock divider for standard/fast mode.
       SCL = I2CCLK / (clkdiv + 1) with 6:3 duty cycle (fixed).
       clkdiv = I2CCLK / SCL - 1 (approximate). */
    clkdiv = (EFR32_I2CCLK / config->clock_speed) - 1U;
    if (clkdiv > _I2C_CLKDIV_MASK >> _I2C_CLKDIV_SHIFT) {
      clkdiv = _I2C_CLKDIV_MASK >> _I2C_CLKDIV_SHIFT;
    }

    i2c->EN_CLR = I2C_EN_EN;
    while ((i2c->EN & I2C_EN_DISABLING) == I2C_EN_DISABLING) {}

    i2c->CTRL = 0;
    i2c->CLKDIV = clkdiv << _I2C_CLKDIV_SHIFT;
    i2c->IF_CLR = _I2C_IF_MASK;
    i2c->IEN = 0;

    /* Enable I2C module. */
    i2c->EN_SET = I2C_EN_EN;

    nvicEnableVector(efr32_i2c_irqn[index], efr32_i2c_irqprio[index]);
  }
}

void i2c_lld_stop(I2CDriver *i2cp) {
  I2C_TypeDef *i2c = i2cp->i2c;

  if (i2cp->state == I2C_READY) {
    i2c->CMD = I2C_CMD_ABORT;
    i2c->IEN = 0;
    i2c->IF_CLR = _I2C_IF_MASK;
    nvicDisableVector(efr32_i2c_irqn[i2cp->index]);
    i2c->EN_CLR = I2C_EN_EN;
    while ((i2c->EN & I2C_EN_DISABLING) == I2C_EN_DISABLING) {}
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

  /* Enable interrupts for the transfer. */
  i2c->IEN_SET = I2C_IEN_ACK | I2C_IEN_NACK | I2C_IEN_MSTOP |
                  I2C_IEN_ARBLOST | I2C_IEN_RXDATAV | I2C_IEN_TXBL;

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

    /* If we have TX data, send next byte. */
    if (i2cp->txidx < i2cp->txlen) {
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

  /* RX data available. */
  if ((isr & I2C_IF_RXDATAV) != 0U) {
    if (i2cp->rxidx < i2cp->rxlen) {
      i2cp->rxbuf[i2cp->rxidx++] = (uint8_t)i2c->RXDATA;
    }
    if (i2cp->rxidx >= i2cp->rxlen && i2cp->txidx >= i2cp->txlen) {
      i2c->IF_CLR = I2C_IF_RXDATAV;
      i2c_lld_wakeup_done(i2cp);
    }
  }

  /* Master STOP sent. */
  if ((isr & I2C_IF_MSTOP) != 0U) {
    i2c->IF_CLR = I2C_IF_MSTOP;
    i2c_lld_wakeup_done(i2cp);
  }
}

#endif /* HAL_USE_I2C == TRUE */

/** @} */
