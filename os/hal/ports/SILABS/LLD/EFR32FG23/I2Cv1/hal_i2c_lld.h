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
 * @file    I2Cv1/hal_i2c_lld.h
 * @brief   EFR32FG23 I2C subsystem low level driver header.
 *
 * @addtogroup I2C
 * @{
 */

#ifndef HAL_I2C_LLD_H
#define HAL_I2C_LLD_H

#if (HAL_USE_I2C == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

#if !defined(EFR32_I2C_USE_I2C0) || defined(__DOXYGEN__)
#define EFR32_I2C_USE_I2C0      FALSE
#endif
#if !defined(EFR32_I2C_USE_I2C1) || defined(__DOXYGEN__)
#define EFR32_I2C_USE_I2C1      FALSE
#endif

#if !defined(EFR32_I2C0_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_I2C0_IRQ_PRIORITY 4
#endif
#if !defined(EFR32_I2C1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_I2C1_IRQ_PRIORITY 4
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   I2C driver configuration structure.
 */
typedef struct {
  /**
   * @brief   I2C clock frequency in Hz.
   */
  uint32_t                  clock_speed;
  /**
   * @brief   Duty cycle (LOW/HIGH ratio), not used on EFR32 (fixed 6:3).
   */
  uint32_t                  duty_cycle;
} I2CConfig;

/**
 * @brief   I2C driver structure fields.
 */
#define i2c_lld_driver_fields                                               \
  /* Pointer to I2C registers. */                                           \
  I2C_TypeDef               *i2c;                                           \
  /* I2C index (0 or 1). */                                                 \
  uint32_t                  index;                                         \
  /* Thread waiting for I2C event. */                                       \
  thread_t                  *thread;                                      \
  /* Last error code. */                                                    \
  i2cflags_t                errors;                                      \
  /* TX buffer. */                                                          \
  const uint8_t             *txbuf;                                       \
  /* TX buffer length. */                                                   \
  size_t                    txlen;                                        \
  /* TX index. */                                                           \
  size_t                    txidx;                                        \
  /* RX buffer. */                                                          \
  uint8_t                   *rxbuf;                                       \
  /* RX buffer length. */                                                   \
  size_t                    rxlen;                                        \
  /* RX index. */                                                           \
  size_t                    rxidx;                                        \
  /* Slave address. */                                                      \
  i2caddr_t                 addr

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if EFR32_I2C_USE_I2C0 && !defined(__DOXYGEN__)
extern I2CDriver I2CD1;
#endif
#if EFR32_I2C_USE_I2C1 && !defined(__DOXYGEN__)
extern I2CDriver I2CD2;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void i2c_lld_init(void);
  void i2c_lld_start(I2CDriver *i2cp, const I2CConfig *config);
  void i2c_lld_stop(I2CDriver *i2cp);
  msg_t i2c_lld_master_transmit_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                         const uint8_t *txbuf, size_t txbytes,
                                         uint8_t *rxbuf, size_t rxbytes,
                                         systime_t timeout);
  msg_t i2c_lld_master_receive_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                        uint8_t *rxbuf, size_t rxbytes,
                                        systime_t timeout);
  void i2c_lld_serve_interrupt(I2CDriver *i2cp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_I2C == TRUE */

#endif /* HAL_I2C_LLD_H */

/** @} */
