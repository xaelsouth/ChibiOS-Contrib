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
 * @file    IMX23/IMX23xx/hal_sio_lld.h
 * @brief   i.MX23 SIO subsystem low level driver header (PL011 DUART).
 *
 * @addtogroup IMX23
 * @{
 */

#ifndef HAL_SIO_LLD_H
#define HAL_SIO_LLD_H

#if (HAL_USE_SIO == TRUE) || defined(__DOXYGEN__)

#include "imx23.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/* Frame configurations (LCR_H values, word length is always 8, FIFO on). */
#define IMX23_SIO_LLD_8N1   (IMX23_PL011_LCR_H_WLEN_8 | IMX23_PL011_LCR_H_FEN)
#define IMX23_SIO_LLD_8E1   (IMX23_SIO_LLD_8N1 | IMX23_PL011_LCR_H_PEN | IMX23_PL011_LCR_H_EPS)
#define IMX23_SIO_LLD_8O1   (IMX23_SIO_LLD_8N1 | IMX23_PL011_LCR_H_PEN)
#define IMX23_SIO_LLD_8N2   (IMX23_SIO_LLD_8N1 | IMX23_PL011_LCR_H_STP2)

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

#if !defined(IMX23_SIO_USE_DUART) || defined(__DOXYGEN__)
#define IMX23_SIO_USE_DUART         TRUE
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

/* Additional driver fields: pointer to the PL011 register block. */
#define sio_lld_driver_fields                                               \
  /* Pointer to the PL011 registers.*/                                     \
  imx23_pl011_t             *uart;

/* Additional config fields: bit rate + LCR_H frame configuration. */
#define sio_lld_config_fields                                               \
  /* Bit rate in bit/s. */                                                  \
  uint32_t                  baud;                                          \
  /* LCR_H frame configuration (IMX23_SIO_LLD_8N1 etc.).*/                 \
  uint32_t                  framecfg;

#if (IMX23_SIO_USE_DUART == TRUE) && !defined(__DOXYGEN__)
extern SIODriver SIOD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void sio_lld_init(void);
  msg_t sio_lld_start(SIODriver *siop);
  void sio_lld_stop(SIODriver *siop);
  void sio_lld_update_enable_flags(SIODriver *siop);
  sioevents_t sio_lld_get_and_clear_errors(SIODriver *siop);
  sioevents_t sio_lld_get_and_clear_events(SIODriver *siop);
  sioevents_t sio_lld_get_events(SIODriver *siop);
  size_t sio_lld_read(SIODriver *siop, uint8_t *buffer, size_t n);
  size_t sio_lld_write(SIODriver *siop, const uint8_t *buffer, size_t n);
  msg_t sio_lld_get(SIODriver *siop);
  void sio_lld_put(SIODriver *siop, uint_fast16_t data);
  msg_t sio_lld_control(SIODriver *siop, unsigned int operation, void *arg);
  void sio_lld_serve_interrupt(SIODriver *siop);
  bool _sio_lld_is_rx_empty(SIODriver *siop);
  bool _sio_lld_is_rx_idle(SIODriver *siop);
  bool _sio_lld_has_rx_errors(SIODriver *siop);
  bool _sio_lld_is_tx_full(SIODriver *siop);
  bool _sio_lld_is_tx_ongoing(SIODriver *siop);
#ifdef __cplusplus
}
#endif

#define sio_lld_is_rx_empty(siop)       _sio_lld_is_rx_empty(siop)
#define sio_lld_is_rx_idle(siop)        _sio_lld_is_rx_idle(siop)
#define sio_lld_has_rx_errors(siop)     _sio_lld_has_rx_errors(siop)
#define sio_lld_is_tx_full(siop)        _sio_lld_is_tx_full(siop)
#define sio_lld_is_tx_ongoing(siop)     _sio_lld_is_tx_ongoing(siop)

#endif /* HAL_USE_SIO == TRUE */

#endif /* HAL_SIO_LLD_H */

/** @} */
