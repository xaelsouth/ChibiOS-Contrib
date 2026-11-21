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
 * @file    SPIv1/hal_spi_lld.h
 * @brief   EFR32FG23 SPI subsystem low level driver header (via EUSART).
 *
 * @addtogroup SPI
 * @{
 */

#ifndef HAL_SPI_LLD_H
#define HAL_SPI_LLD_H

#if (HAL_USE_SPI == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

#if !defined(EFR32_SPI_USE_EUSART1) || defined(__DOXYGEN__)
#define EFR32_SPI_USE_EUSART1   FALSE
#endif
#if !defined(EFR32_SPI_USE_EUSART2) || defined(__DOXYGEN__)
#define EFR32_SPI_USE_EUSART2   FALSE
#endif
#if !defined(EFR32_SPI_USE_EUSART3) || defined(__DOXYGEN__)
#define EFR32_SPI_USE_EUSART3   FALSE
#endif

#if !defined(EFR32_SPI_EUSART1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_SPI_EUSART1_IRQ_PRIORITY  4
#endif
#if !defined(EFR32_SPI_EUSART2_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_SPI_EUSART2_IRQ_PRIORITY  4
#endif
#if !defined(EFR32_SPI_EUSART3_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_SPI_EUSART3_IRQ_PRIORITY  4
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   SPI driver configuration structure.
 */
typedef struct {
  /**
   * @brief   Bit rate in Hz.
   */
  uint32_t                  bitrate;
  /**
   * @brief   EUSART CFG0 register value (clock mode, oversampling).
   */
  uint32_t                  cfg0;
  /**
   * @brief   EUSART FRAMECFG register value (frame format).
   */
  uint32_t                  framecfg;
  /**
   * @brief   Callback for end-of-transfer, or NULL.
   */
  spicallback_t             end_cb;
  /**
   * @brief   Callback argument.
   */
  void                      *end_cb_param;
} SPIConfig;

/**
 * @brief   SPI driver structure fields.
 */
#define spi_lld_driver_fields                                               \
  /* Pointer to EUSART registers. */                                        \
  EUSART_TypeDef            *eusart;                                        \
  /* EUSART index (0,1,2). */                                               \
  uint32_t                  index;                                         \
  /* Current config. */                                                     \
  const SPIConfig           *config;                                       \
  /* CS port index. */                                                      \
  ioportid_t                cs_port;                                       \
  /* CS pad. */                                                             \
  iopadid_t                 cs_pad

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if EFR32_SPI_USE_EUSART1 && !defined(__DOXYGEN__)
extern SPIDriver SPID1;
#endif
#if EFR32_SPI_USE_EUSART2 && !defined(__DOXYGEN__)
extern SPIDriver SPID2;
#endif
#if EFR32_SPI_USE_EUSART3 && !defined(__DOXYGEN__)
extern SPIDriver SPID3;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void spi_lld_init(void);
  void spi_lld_start(SPIDriver *spip, const SPIConfig *config);
  void spi_lld_stop(SPIDriver *spip);
  void spi_lld_select(SPIDriver *spip);
  void spi_lld_unselect(SPIDriver *spip);
  void spi_lld_ignore(SPIDriver *spip, size_t n);
  void spi_lld_exchange(SPIDriver *spip, size_t n, const void *txbuf, void *rxbuf);
  void spi_lld_send(SPIDriver *spip, size_t n, const void *txbuf);
  void spi_lld_receive(SPIDriver *spip, size_t n, void *rxbuf);
  uint16_t spi_lld_polled_exchange(SPIDriver *spip, uint16_t frame);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SPI == TRUE */

#endif /* HAL_SPI_LLD_H */

/** @} */
