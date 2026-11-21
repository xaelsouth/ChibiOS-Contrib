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

/* DMA stream assignments. EFR32_DMA_STREAM_ID_ANY = auto-alloc,
   specific 0-7 = pin, -1 = disable DMA for that direction. */
#if !defined(EFR32_I2C0_TX_DMA_STREAM) || defined(__DOXYGEN__)
#define EFR32_I2C0_TX_DMA_STREAM    EFR32_DMA_STREAM_ID_ANY
#endif
#if !defined(EFR32_I2C0_RX_DMA_STREAM) || defined(__DOXYGEN__)
#define EFR32_I2C0_RX_DMA_STREAM    EFR32_DMA_STREAM_ID_ANY
#endif
#if !defined(EFR32_I2C1_TX_DMA_STREAM) || defined(__DOXYGEN__)
#define EFR32_I2C1_TX_DMA_STREAM    EFR32_DMA_STREAM_ID_ANY
#endif
#if !defined(EFR32_I2C1_RX_DMA_STREAM) || defined(__DOXYGEN__)
#define EFR32_I2C1_RX_DMA_STREAM    EFR32_DMA_STREAM_ID_ANY
#endif

#if !defined(EFR32_I2C0_DMA_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_I2C0_DMA_IRQ_PRIORITY 5
#endif
#if !defined(EFR32_I2C1_DMA_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_I2C1_DMA_IRQ_PRIORITY 5
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of I2C slave address.
 */
typedef uint16_t i2caddr_t;

/**
 * @brief   Type of I2C Driver condition flags.
 */
typedef uint32_t i2cflags_t;

/**
 * @brief   Type of I2C driver configuration structure.
 */
typedef struct {
  /**
   * @brief   I2C clock frequency in Hz (SCL).
   */
  uint32_t                  clock_speed;
  /**
   * @brief   Duty cycle, not used on EFR32 (fixed 6:3).
   */
  uint32_t                  duty_cycle;
} I2CConfig;

/**
 * @brief   Type of a structure representing an I2C driver.
 */
typedef struct I2CDriver I2CDriver;

/**
 * @brief   Structure representing an I2C driver.
 */
struct I2CDriver {
  /**
   * @brief   Driver state.
   */
  i2cstate_t                state;
  /**
   * @brief   Current configuration data.
   */
  const I2CConfig           *config;
  /**
   * @brief   Error flags.
   */
  i2cflags_t                errors;
#if (I2C_USE_MUTUAL_EXCLUSION == TRUE) || defined(__DOXYGEN__)
  mutex_t                   mutex;
#endif
  /* End of the mandatory fields.*/
  /**
   * @brief   Pointer to the I2Cx registers block.
   */
  I2C_TypeDef               *i2c;
  /**
   * @brief   I2C index (0 or 1).
   */
  uint32_t                  index;
  /**
   * @brief   Thread waiting for I2C event.
   */
  thread_reference_t        thread;
  /**
   * @brief   TX buffer.
   */
  const uint8_t             *txbuf;
  /**
   * @brief   TX buffer length.
   */
  size_t                    txlen;
  /**
   * @brief   TX index.
   */
  size_t                    txidx;
  /**
   * @brief   RX buffer.
   */
  uint8_t                   *rxbuf;
  /**
   * @brief   RX buffer length.
   */
  size_t                    rxlen;
  /**
   * @brief   RX index.
   */
  size_t                    rxidx;
  /**
   * @brief   Slave address.
   */
  i2caddr_t                 addr;
  /**
   * @brief   DMA TX stream (NULL if not allocated).
   */
  const void               *dma_tx;
  /**
   * @brief   DMA RX stream (NULL if not allocated).
   */
  const void               *dma_rx;
  /**
   * @brief   Non-zero when DMA is handling data phase.
   */
  uint32_t                  dma_active;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

#define i2c_lld_get_errors(i2cp) ((i2cp)->errors)

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
  void i2c_lld_start(I2CDriver *i2cp);
  void i2c_lld_stop(I2CDriver *i2cp);
  msg_t i2c_lld_master_transmit_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                         const uint8_t *txbuf, size_t txbytes,
                                         uint8_t *rxbuf, size_t rxbytes,
                                         systime_t timeout);
  msg_t i2c_lld_master_receive_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                        uint8_t *rxbuf, size_t rxbytes,
                                        systime_t timeout);
  msg_t i2c_lld_master_transmit_dma_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                             const uint8_t *txbuf, size_t txbytes,
                                             uint8_t *rxbuf, size_t rxbytes,
                                             systime_t timeout);
  msg_t i2c_lld_master_receive_dma_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                            uint8_t *rxbuf, size_t rxbytes,
                                            systime_t timeout);
  void i2c_lld_serve_interrupt(I2CDriver *i2cp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_I2C == TRUE */

#endif /* HAL_I2C_LLD_H */

/** @} */
