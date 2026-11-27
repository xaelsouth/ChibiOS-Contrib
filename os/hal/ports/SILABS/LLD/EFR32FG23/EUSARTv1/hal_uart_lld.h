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
 * @file    EUSARTv1/hal_uart_lld.h
 * @brief   EFR32FG23 UART subsystem low level driver header.
 *
 * @addtogroup UART
 * @{
 */

#ifndef HAL_UART_LLD_H
#define HAL_UART_LLD_H

#if (HAL_USE_UART == TRUE) || defined(__DOXYGEN__)

#include "efr32_irda.h"

#define UART_LLD_ENHANCED_API

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

#if !defined(EFR32_UART_USE_EUSART1) || defined(__DOXYGEN__)
#define EFR32_UART_USE_EUSART1      FALSE
#endif
#if !defined(EFR32_UART_USE_EUSART2) || defined(__DOXYGEN__)
#define EFR32_UART_USE_EUSART2      FALSE
#endif
#if !defined(EFR32_UART_USE_EUSART3) || defined(__DOXYGEN__)
#define EFR32_UART_USE_EUSART3      FALSE
#endif
#if !defined(EFR32_UART_USE_USART1) || defined(__DOXYGEN__)
#define EFR32_UART_USE_USART1       FALSE
#endif

#if !defined(EFR32_UART_EUSART1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_UART_EUSART1_IRQ_PRIORITY  4
#endif
#if !defined(EFR32_UART_EUSART2_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_UART_EUSART2_IRQ_PRIORITY  4
#endif
#if !defined(EFR32_UART_EUSART3_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_UART_EUSART3_IRQ_PRIORITY  4
#endif
#if !defined(EFR32_UART_USART1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_UART_USART1_IRQ_PRIORITY   4
#endif

/**
 * @brief   Enables TX and RX DMA for each UART instance.
 * @details Starting an instance requires two LDMA channels and returns
 *          @p HAL_RET_NO_RESOURCE if both cannot be allocated. Buffers must
 *          remain valid until completion or cancellation.
 */
#if !defined(EFR32_UART_USE_DMA) || defined(__DOXYGEN__)
#define EFR32_UART_USE_DMA               FALSE
#endif
#if !defined(EFR32_UART_DMA_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_UART_DMA_IRQ_PRIORITY      5
#endif
#if !defined(EFR32_UART_DMA_ERROR_HOOK) || defined(__DOXYGEN__)
#define EFR32_UART_DMA_ERROR_HOOK(uartp) osalSysHalt("UART DMA failure")
#endif
#if EFR32_UART_USE_DMA && !defined(EFR32_DMA_REQUIRED)
#define EFR32_DMA_REQUIRED
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

typedef uint32_t uartflags_t;
typedef struct hal_uart_driver UARTDriver;
typedef void (*uartcb_t)(UARTDriver *uartp);
typedef void (*uartccb_t)(UARTDriver *uartp, uint16_t c);
typedef void (*uartecb_t)(UARTDriver *uartp, uartflags_t errors);

/**
 * @brief   UART driver configuration structure.
 */
typedef struct {
  /**
   * @brief   Bit rate in Hz.
   */
  uint32_t                  speed;
  /**
   * @brief   EUSART CFG0 / USART CTRL register value.
   */
  uint32_t                  cfg0;
  /**
   * @brief   EUSART FRAMECFG / USART FRAME register value.
   */
  uint32_t                  framecfg;
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
  /**
   * @brief   End of transmission buffer callback.
   */
  uartcb_t                  txend1_cb;
  /**
   * @brief   Physical end of transmission callback.
   */
  uartcb_t                  txend2_cb;
  /**
   * @brief   Receive buffer filled callback.
   */
  uartcb_t                  rxend_cb;
  /**
   * @brief   Character received callback.
   */
  uartccb_t                 rxchar_cb;
  /**
   * @brief   Receive error callback.
   */
  uartecb_t                 rxerr_cb;
} UARTConfig;

/**
 * @brief   UART driver structure fields.
 */
struct hal_uart_driver {
  uartstate_t               state;
  uarttxstate_t             txstate;
  uartrxstate_t             rxstate;
  const UARTConfig          *config;
#if UART_USE_WAIT == TRUE
  bool                      early;
  thread_reference_t        threadrx;
  thread_reference_t        threadtx;
#endif
#if UART_USE_MUTUAL_EXCLUSION == TRUE
  mutex_t                   mutex;
#endif
#if defined(UART_DRIVER_EXT_FIELDS)
  UART_DRIVER_EXT_FIELDS
#endif
  void                      *usart;
  bool                      is_usart;
  bool                      wide;
  uint32_t                  index;
  uint32_t                  clock;
  const uint8_t             *txbuf;
  uint8_t                   *rxbuf;
  size_t                    txremaining;
  size_t                    rxremaining;
#if EFR32_UART_USE_DMA
  const efr32_dma_stream_t  *dma_tx;
  const efr32_dma_stream_t  *dma_rx;
  size_t                    dma_tx_count;
  size_t                    dma_rx_count;
  bool                      dma_tx_active;
  bool                      dma_rx_active;
  bool                      dma_tx_done;
  bool                      dma_rx_done;
#endif
};

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if EFR32_UART_USE_EUSART1 && !defined(__DOXYGEN__)
extern UARTDriver UARTD1;
#endif
#if EFR32_UART_USE_EUSART2 && !defined(__DOXYGEN__)
extern UARTDriver UARTD2;
#endif
#if EFR32_UART_USE_EUSART3 && !defined(__DOXYGEN__)
extern UARTDriver UARTD3;
#endif
#if EFR32_UART_USE_USART1 && !defined(__DOXYGEN__)
extern UARTDriver UARTD4;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void uart_lld_init(void);
  msg_t uart_lld_start(UARTDriver *uartp);
  void uart_lld_stop(UARTDriver *uartp);
  void uart_lld_start_send(UARTDriver *uartp, size_t n, const void *txbuf);
  size_t uart_lld_stop_send(UARTDriver *uartp);
  void uart_lld_start_receive(UARTDriver *uartp, size_t n, void *rxbuf);
  size_t uart_lld_stop_receive(UARTDriver *uartp);
  void uart_lld_serve_interrupt(UARTDriver *uartp);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* UART IrDA presets (8E1, 3/16 pulse).                                       */
/*===========================================================================*/

#define EFR32_UART_LLD_EUSART_8E1                                              \
  (EUSART_FRAMECFG_DATABITS_EIGHT | EUSART_FRAMECFG_PARITY_EVEN |             \
   EUSART_FRAMECFG_STOPBITS_ONE)

/* 2400 baud, 8E1, LF IrDA (LFXO/LFRCO clocked, EM2/EM3). */
#define EFR32_UART_LLD_IRDA_2400_8E1_LF                                        \
  {                                                                            \
    .speed    = 2400U,                                                         \
    .cfg0     = (0U << 0) | (4U << 5),   /* ASYNC, OVS disabled */             \
    .framecfg = EFR32_UART_LLD_EUSART_8E1,                                     \
    .irhfcg   = 0,                                                             \
    .irlfcfg  = 0,                                                             \
    .irctrl   = EFR32_IRDA_LF,                                                 \
  }

/* 2400 baud, 8E1, HF IrDA (HFRCO/HFXO clocked, EM0/EM1). */
#define EFR32_UART_LLD_IRDA_2400_8E1_HF                                        \
  {                                                                            \
    .speed    = 2400U,                                                         \
    .cfg0     = (0U << 0) | (0U << 5),   /* ASYNC, 16x OVS */                  \
    .framecfg = EFR32_UART_LLD_EUSART_8E1,                                     \
    .irhfcg   = EFR32_IRDA_PW_3_16 | EUSART_IRHFCFG_IRHFFILT,                  \
    .irlfcfg  = 0,                                                             \
    .irctrl   = EFR32_IRDA_HF,                                                 \
  }

/* 9600 baud, 8E1, HF IrDA. */
#define EFR32_UART_LLD_IRDA_9600_8E1_HF                                        \
  {                                                                            \
    .speed    = 9600U,                                                         \
    .cfg0     = (0U << 0) | (0U << 5),                                         \
    .framecfg = EFR32_UART_LLD_EUSART_8E1,                                     \
    .irhfcg   = EFR32_IRDA_PW_3_16 | EUSART_IRHFCFG_IRHFFILT,                  \
    .irlfcfg  = 0,                                                             \
    .irctrl   = EFR32_IRDA_HF,                                                 \
  }

/* 9600 baud, 8E1, LF IrDA (marginal from 32768 Hz). */
#define EFR32_UART_LLD_IRDA_9600_8E1_LF                                        \
  {                                                                            \
    .speed    = 9600U,                                                         \
    .cfg0     = (0U << 0) | (4U << 5),                                         \
    .framecfg = EFR32_UART_LLD_EUSART_8E1,                                     \
    .irhfcg   = 0,                                                             \
    .irlfcfg  = 0,                                                             \
    .irctrl   = EFR32_IRDA_LF,                                                 \
  }

#endif /* HAL_USE_UART == TRUE */

#endif /* HAL_UART_LLD_H */

/** @} */
