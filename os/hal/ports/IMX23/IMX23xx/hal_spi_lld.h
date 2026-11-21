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
 * @file    IMX23/IMX23xx/hal_spi_lld.h
 * @brief   i.MX23 SPI subsystem low level driver header (SSP, PIO mode).
 *
 * @addtogroup IMX23
 * @{
 */

#ifndef HAL_SPI_LLD_H
#define HAL_SPI_LLD_H

#if (HAL_USE_SPI == TRUE) || defined(__DOXYGEN__)

#include "imx23.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

#if !defined(IMX23_SPI_USE_SSP0) || defined(__DOXYGEN__)
#define IMX23_SPI_USE_SSP0      TRUE
#endif
#if !defined(IMX23_SPI_USE_SSP1) || defined(__DOXYGEN__)
#define IMX23_SPI_USE_SSP1      TRUE
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

#define spi_lld_driver_fields                                               \
  /* Pointer to the SSP registers.*/                                       \
  imx23_ssp_t               *ssp;

/* Additional config fields: SCK frequency in Hz, SPI mode 0..3. */
#define spi_lld_config_fields                                               \
  uint32_t                  clock;                                          \
  uint32_t                  mode;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if (IMX23_SPI_USE_SSP0 == TRUE) && !defined(__DOXYGEN__)
extern SPIDriver SPID1;
#endif
#if (IMX23_SPI_USE_SSP1 == TRUE) && !defined(__DOXYGEN__)
extern SPIDriver SPID2;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void spi_lld_init(void);
  void spi_lld_start(SPIDriver *spip);
  void spi_lld_stop(SPIDriver *spip);
  void spi_lld_select(SPIDriver *spip);
  void spi_lld_unselect(SPIDriver *spip);
  void spi_lld_ignore(SPIDriver *spip, size_t n);
  void spi_lld_exchange(SPIDriver *spip, size_t n, const void *txbuf, void *rxbuf);
  void spi_lld_send(SPIDriver *spip, size_t n, const void *txbuf);
  void spi_lld_receive(SPIDriver *spip, size_t n, void *rxbuf);
  void spi_lld_abort(SPIDriver *spip);
  uint16_t spi_lld_polled_exchange(SPIDriver *spip, uint16_t frame);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SPI == TRUE */

#endif /* HAL_SPI_LLD_H */

/** @} */
