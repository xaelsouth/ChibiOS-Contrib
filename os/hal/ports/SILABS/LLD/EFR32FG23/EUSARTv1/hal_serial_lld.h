/*
    ChibiOS - Copyright (C) 2024 Xael South

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
 * @file    hal_serial_lld.h
 * @brief   PLATFORM serial subsystem low level driver header.
 *
 * @addtogroup SERIAL
 * @{
 */

#ifndef HAL_SERIAL_LLD_H
#define HAL_SERIAL_LLD_H

#if (HAL_USE_SERIAL == TRUE) || defined(__DOXYGEN__)

#define SD_LLD_ENHANCED_API

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    PLATFORM configuration options
 * @{
 */
/**
 * @brief   USART1 driver enable switch.
 * @details If set to @p TRUE the support for USART1 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(EFR32_SERIAL_USE_USART1) || defined(__DOXYGEN__)
#define EFR32_SERIAL_USE_USART1             FALSE
#endif

#if !defined(EFR32_SERIAL_USE_DMA) || defined(__DOXYGEN__)
#define EFR32_SERIAL_USE_DMA                FALSE
#endif
/**
 * @brief   Enables USART0 TX and RX DMA independently.
 * @details Starting returns @p HAL_RET_NO_RESOURCE if a required LDMA
 *          channel cannot be allocated.
 */
#if !defined(EFR32_SERIAL_USE_TX_DMA) || defined(__DOXYGEN__)
#define EFR32_SERIAL_USE_TX_DMA             EFR32_SERIAL_USE_DMA
#endif
#if !defined(EFR32_SERIAL_USE_RX_DMA) || defined(__DOXYGEN__)
#define EFR32_SERIAL_USE_RX_DMA             EFR32_SERIAL_USE_DMA
#endif
#if !defined(EFR32_SERIAL_DMA_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_SERIAL_DMA_IRQ_PRIORITY       5
#endif
#if !defined(EFR32_SERIAL_DMA_ERROR_HOOK) || defined(__DOXYGEN__)
#define EFR32_SERIAL_DMA_ERROR_HOOK(sdp)   osalSysHalt("SERIAL DMA failure")
#endif
#if (EFR32_SERIAL_USE_TX_DMA || EFR32_SERIAL_USE_RX_DMA) && !defined(EFR32_DMA_REQUIRED)
#define EFR32_DMA_REQUIRED
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   PLATFORM Serial Driver configuration structure.
 * @details An instance of this structure must be passed to @p sdStart()
 *          in order to configure and start a serial driver operations.
 * @note    This structure content is architecture dependent, each driver
 *          implementation defines its own version and the custom static
 *          initializers.
 */
typedef struct hal_serial_config {
  /**
   * @brief Bit rate.
   */
  uint32_t                  speed;
  /* End of the mandatory fields.*/
  /**
   * @brief   IrDA HF config (EUSART only, 0=disabled).
   */
  uint32_t                  irhfcg;
  /**
   * @brief   IrDA LF config (EUSART only, 0=disabled).
   */
  uint32_t                  irlfcfg;
  /**
   * @brief   IrDA control: 0=none, 1=HF, 2=LF, 3=AUTO.
   */
  uint32_t                  irctrl;
} SerialConfig;

/**
 * @brief   @p SerialDriver specific data.
 */
#if EFR32_SERIAL_USE_TX_DMA
#define _serial_tx_dma_fields                                               \
  const efr32_dma_stream_t *dma_tx;                                          \
  uint8_t                   dma_tx_buf[64];                                 \
  bool                      dma_tx_busy;
#else
#define _serial_tx_dma_fields
#endif
#if EFR32_SERIAL_USE_RX_DMA
#include "efr32_stream_rx_dma.h"
#define _serial_rx_dma_fields                                               \
  efr32_stream_rx_dma_t     dma_rx;
#else
#define _serial_rx_dma_fields
#endif

#define _serial_driver_data                                                 \
  _base_asynchronous_channel_data                                           \
  /* Driver state.*/                                                        \
  sdstate_t                 state;                                          \
  /* Input queue.*/                                                         \
  input_queue_t             iqueue;                                         \
  /* Output queue.*/                                                        \
  output_queue_t            oqueue;                                         \
  /* Input circular buffer.*/                                               \
  uint8_t                   ib[SERIAL_BUFFERS_SIZE];                        \
  /* Output circular buffer.*/                                              \
  uint8_t                   ob[SERIAL_BUFFERS_SIZE];                        \
  /* End of the mandatory fields.*/                                         \
  /* Pointer to the USART registers block.*/                                \
  void                     *usart;                                          \
  /* Clock frequency for the associated USART.*/                            \
  uint32_t                  clock;                                          \
  _serial_tx_dma_fields                                                     \
  _serial_rx_dma_fields

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if (EFR32_SERIAL_USE_USART1 == TRUE) && !defined(__DOXYGEN__)
extern SerialDriver SD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void sd_lld_init(void);
  msg_t sd_lld_start(SerialDriver *sdp, const SerialConfig *config);
  void sd_lld_stop(SerialDriver *sdp);
  void sd_lld_serve_interrupt(SerialDriver *sdp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SERIAL == TRUE */

#endif /* HAL_SERIAL_LLD_H */

/** @} */
