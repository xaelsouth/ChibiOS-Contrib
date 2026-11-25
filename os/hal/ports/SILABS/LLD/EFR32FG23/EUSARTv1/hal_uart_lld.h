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

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

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
#define uart_lld_driver_fields                                              \
  /* Pointer to the EUSART/USART registers (as void*). */                   \
  void                      *usart;                                         \
  /* True if this is a USART (not EUSART). */                               \
  bool                      is_usart;                                      \
  /* UART instance index (0=EUSART0, 1=EUSART1, 2=EUSART2, 3=USART0). */   \
  uint32_t                  index;                                         \
  /* Clock frequency for baud rate calculation. */                          \
  uint32_t                  clock;


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
  msg_t uart_lld_start(UARTDriver *uartp, const UARTConfig *config);
  void uart_lld_stop(UARTDriver *uartp);
  void uart_lld_start_send(UARTDriver *uartp, size_t n, const void *txbuf);
  size_t uart_lld_stop_send(UARTDriver *uartp);
  void uart_lld_start_receive(UARTDriver *uartp, size_t n, void *rxbuf);
  size_t uart_lld_stop_receive(UARTDriver *uartp);
  void uart_lld_serve_interrupt(UARTDriver *uartp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_UART == TRUE */

#endif /* HAL_UART_LLD_H */

/** @} */

/*===========================================================================*/
/* UART IrDA presets (8E1, 3/16 pulse).                                       */
/*===========================================================================*/

/* 2400 baud, 8E1, LF IrDA (LFXO/LFRCO clocked, EM2/EM3). */
#define EFR32_UART_LLD_IRDA_2400_8E1_LF                                        \
  {                                                                            \
    .speed    = 2400U,                                                         \
    .cfg0     = (0U << 0) | (4U << 5),   /* ASYNC, OVS disabled */             \
    .framecfg = EFR32_SIO_LLD_EUSART_8E1,                                      \
    .irhfcg   = 0,                                                             \
    .irlfcfg  = 0,                                                             \
    .irctrl   = EFR32_IRDA_LF,                                                 \
  }

/* 2400 baud, 8E1, HF IrDA (HFRCO/HFXO clocked, EM0/EM1). */
#define EFR32_UART_LLD_IRDA_2400_8E1_HF                                        \
  {                                                                            \
    .speed    = 2400U,                                                         \
    .cfg0     = (0U << 0) | (0U << 5),   /* ASYNC, 16x OVS */                  \
    .framecfg = EFR32_SIO_LLD_EUSART_8E1,                                      \
    .irhfcg   = EFR32_IRDA_PW_3_16 | EUSART_IRHFCFG_IRHFFILT,                  \
    .irlfcfg  = 0,                                                             \
    .irctrl   = EFR32_IRDA_HF,                                                 \
  }

/* 9600 baud, 8E1, HF IrDA. */
#define EFR32_UART_LLD_IRDA_9600_8E1_HF                                        \
  {                                                                            \
    .speed    = 9600U,                                                         \
    .cfg0     = (0U << 0) | (0U << 5),                                         \
    .framecfg = EFR32_SIO_LLD_EUSART_8E1,                                      \
    .irhfcg   = EFR32_IRDA_PW_3_16 | EUSART_IRHFCFG_IRHFFILT,                  \
    .irlfcfg  = 0,                                                             \
    .irctrl   = EFR32_IRDA_HF,                                                 \
  }

/* 9600 baud, 8E1, LF IrDA (marginal from 32768 Hz). */
#define EFR32_UART_LLD_IRDA_9600_8E1_LF                                        \
  {                                                                            \
    .speed    = 9600U,                                                         \
    .cfg0     = (0U << 0) | (4U << 5),                                         \
    .framecfg = EFR32_SIO_LLD_EUSART_8E1,                                      \
    .irhfcg   = 0,                                                             \
    .irlfcfg  = 0,                                                             \
    .irctrl   = EFR32_IRDA_LF,                                                 \
  }
