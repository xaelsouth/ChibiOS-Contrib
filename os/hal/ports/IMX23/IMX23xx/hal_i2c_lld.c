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
 * @file    IMX23/IMX23xx/hal_i2c_lld.c
 * @brief   i.MX23 I2C subsystem low level driver code (I2C0, polled PIO master).
 *
 * @details Reworked from Linux drivers/i2c/busses/i2c-mxs.c (PIO mode path)
 *          and the i.MX23 RM chapter 25. Master transmit/receive are polled
 *          using the decrementing CTRL0 XFER_COUNT field; NAK and timeouts
 *          are reported through i2cp->errors.
 *
 * @addtogroup IMX23
 * @{
 */

#include "hal.h"

#if (HAL_USE_I2C == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

#if (IMX23_I2C_USE_I2C0 == TRUE) && !defined(__DOXYGEN__)
I2CDriver I2CD1;
#endif

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/* Programs TIMING0/1/2 for the requested bus speed, ported from Linux
   mxs_i2c_set_bus_speed() (24MHz bus clock). */
static void i2c_set_timing(imx23_i2c_t *i2c, uint32_t speed) {
  uint32_t clk = IMX23_I2C_CLK_HZ;
  uint32_t divider, low_count, high_count, rcv_count, xmit_count, leadin, bus_free;

  if (speed == 0U || speed > 400000U) {
    speed = 100000U;
  }
  divider = (clk + speed - 1U) / speed;

  if (speed > 100000U) {
    /* fast mode */
    low_count  = (divider * 13U + 9U) / 19U;
    high_count = (divider * 6U + 9U) / 19U;
    leadin     = (600U * (clk / 1000000U) + 999U) / 1000U;
    bus_free   = (1300U * (clk / 1000000U) + 999U) / 1000U;
  }
  else {
    /* standard mode */
    low_count  = (divider * 47U + 43U) / 87U;
    high_count = (divider * 40U + 43U) / 87U;
    leadin     = (4700U * (clk / 1000000U) + 999U) / 1000U;
    bus_free   = (4700U * (clk / 1000000U) + 999U) / 1000U;
  }
  rcv_count  = high_count * 3U / 8U;
  xmit_count = low_count * 3U / 8U;

  if (low_count > 2U) low_count -= 2U;
  if (high_count > 7U) high_count -= 7U;

  i2c->TIMING0 = (high_count << 16) | rcv_count;
  i2c->TIMING1 = (low_count << 16) | xmit_count;
  i2c->TIMING2 = (bus_free << 16) | leadin;
}

/* Waits until CTRL0 RUN clears; returns false on NAK or timeout. */
static bool i2c_wait_end(imx23_i2c_t *i2c, sysinterval_t timeout) {
  systime_t t0 = stGetCounter();

  while ((i2c->CTRL0 & IMX23_I2C_CTRL0_RUN) != 0U) {
    if ((i2c->CTRL1 & IMX23_I2C_CTRL1_NO_SLAVE_ACK_IRQ) != 0U) {
      return false;   /* NAK */
    }
    if ((stGetCounter() - t0) > timeout) {
      return false;   /* timeout */
    }
  }
  return (i2c->STAT & IMX23_I2C_STAT_GOT_A_NAK) == 0U;
}

/* Master write of n bytes (PIO mode). */
static msg_t i2c_write(imx23_i2c_t *i2c, i2caddr_t addr, const uint8_t *txbuf,
                       size_t n, bool stop, sysinterval_t timeout) {
  systime_t t0 = stGetCounter();
  size_t written = 0U;

  i2c->CTRL1 = IMX23_I2C_CTRL1_SLAVE_ADDR_BYTE((addr << 1) | 0U);
  i2c->CTRL0 = IMX23_I2C_CTRL0_PIO_MODE | IMX23_I2C_CTRL0_MASTER_MODE |
               IMX23_I2C_CTRL0_PRE_SEND_START |
               (stop ? IMX23_I2C_CTRL0_POST_SEND_STOP : 0U) |
               (uint32_t)n;

  /* Preload up to 4 bytes, then keep the FIFO fed. */
  while (written < n) {
    uint32_t xc = i2c->CTRL0 & IMX23_I2C_CTRL0_XFER_COUNT_MASK;
    if ((n - written) - (xc > n ? 0U : (n - xc)) < 4U || written == 0U) {
      /* FIFO has room (4 deep): written-but-unsent < 4 */
      if (written - (n - xc) < 4U) {
        i2c->DATA = txbuf[written++];
        continue;
      }
    }
    if ((stGetCounter() - t0) > timeout) {
      return MSG_TIMEOUT;
    }
  }

  i2c->CTRL0 |= IMX23_I2C_CTRL0_RUN;
  if (!i2c_wait_end(i2c, timeout)) {
    return (i2c->CTRL1 & IMX23_I2C_CTRL1_NO_SLAVE_ACK_IRQ) ? MSG_RESET : MSG_TIMEOUT;
  }
  return MSG_OK;
}

/* Master read of n bytes (PIO mode, DIRECTION=1). */
static msg_t i2c_read(imx23_i2c_t *i2c, i2caddr_t addr, uint8_t *rxbuf,
                      size_t n, bool start, bool stop, sysinterval_t timeout) {
  systime_t t0 = stGetCounter();
  size_t got = 0U;

  i2c->CTRL1 = IMX23_I2C_CTRL1_SLAVE_ADDR_BYTE((addr << 1) | 1U);
  i2c->CTRL0 = IMX23_I2C_CTRL0_PIO_MODE | IMX23_I2C_CTRL0_MASTER_MODE |
               IMX23_I2C_CTRL0_DIRECTION | IMX23_I2C_CTRL0_SEND_NAK_ON_LAST |
               (start ? IMX23_I2C_CTRL0_PRE_SEND_START : 0U) |
               (stop ? IMX23_I2C_CTRL0_POST_SEND_STOP : 0U) |
               (uint32_t)n;
  i2c->CTRL0 |= IMX23_I2C_CTRL0_RUN;

  while (got < n) {
    uint32_t xc = i2c->CTRL0 & IMX23_I2C_CTRL0_XFER_COUNT_MASK;
    if (xc < (uint32_t)(n - got)) {
      rxbuf[got++] = (uint8_t)(i2c->DATA & 0xFFU);
      continue;
    }
    if ((i2c->CTRL1 & IMX23_I2C_CTRL1_NO_SLAVE_ACK_IRQ) != 0U) {
      return MSG_RESET;
    }
    if ((stGetCounter() - t0) > timeout) {
      return MSG_TIMEOUT;
    }
  }

  if (!i2c_wait_end(i2c, timeout)) {
    return (i2c->CTRL1 & IMX23_I2C_CTRL1_NO_SLAVE_ACK_IRQ) ? MSG_RESET : MSG_TIMEOUT;
  }
  return MSG_OK;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level I2C driver initialization.
 *
 * @notapi
 */
void i2c_lld_init(void) {

#if (IMX23_I2C_USE_I2C0 == TRUE) && !defined(__DOXYGEN__)
  i2cObjectInit(&I2CD1);
  I2CD1.i2c = IMX23_I2C0;
#endif
}

/**
 * @brief   Configures and activates the I2C peripheral.
 *
 * @notapi
 */
void i2c_lld_start(I2CDriver *i2cp) {
  imx23_i2c_t *i2c = i2cp->i2c;

  /* Reset and ungate. */
  i2c->CTRL0 = IMX23_I2C_CTRL0_SFTRST;
  while ((i2c->CTRL0 & IMX23_I2C_CTRL0_SFTRST) != 0U) {
  }
  i2c->CTRL0 = 0U;

  i2c_set_timing(i2c, i2cp->config->clock);
}

/**
 * @brief   Deactivates the I2C peripheral.
 *
 * @notapi
 */
void i2c_lld_stop(I2CDriver *i2cp) {

  if (i2cp->state == I2C_READY) {
    i2cp->state = I2C_STOP;
    i2cp->i2c->CTRL0 = IMX23_I2C_CTRL0_SFTRST;
  }
}

/**
 * @brief   Master transmit with optional receive (repeated start).
 *
 * @notapi
 */
msg_t i2c_lld_master_transmit_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                      const uint8_t *txbuf, size_t txbytes,
                                      uint8_t *rxbuf, size_t rxbytes,
                                      sysinterval_t timeout) {
  msg_t msg;

  i2cp->errors = I2C_NO_ERROR;

  if (txbytes > 0U) {
    msg = i2c_write(i2cp->i2c, addr, txbuf, txbytes, rxbytes == 0U, timeout);
    if (msg != MSG_OK) {
      i2cp->errors |= I2C_ACK_FAILURE;
      return msg;
    }
  }
  if (rxbytes > 0U) {
    msg = i2c_read(i2cp->i2c, addr, rxbuf, rxbytes, txbytes > 0U, true, timeout);
    if (msg != MSG_OK) {
      i2cp->errors |= (msg == MSG_RESET) ? I2C_ACK_FAILURE : I2C_TIMEOUT;
      return msg;
    }
  }
  return MSG_OK;
}

/**
 * @brief   Master receive.
 *
 * @notapi
 */
msg_t i2c_lld_master_receive_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                     uint8_t *rxbuf, size_t rxbytes,
                                     sysinterval_t timeout) {
  msg_t msg;

  i2cp->errors = I2C_NO_ERROR;
  msg = i2c_read(i2cp->i2c, addr, rxbuf, rxbytes, true, true, timeout);
  if (msg != MSG_OK) {
    i2cp->errors |= (msg == MSG_RESET) ? I2C_ACK_FAILURE : I2C_TIMEOUT;
  }
  return msg;
}

#endif /* HAL_USE_I2C == TRUE */

/** @} */
