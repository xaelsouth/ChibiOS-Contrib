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
 * @file    EUSARTv1/hal_eusart_irq.c
 * @brief   Shared EUSART/USART interrupt handlers for SIO and UART drivers.
 *
 * @details Each EUSART/USART IRQ handler dispatches to the active driver
 *          (SIO or UART) for that instance. This avoids duplicate symbol
 *          definitions when both HAL_USE_SIO and HAL_USE_UART are enabled.
 *
 * @addtogroup EUSART
 * @{
 */

#include "hal.h"

#if (HAL_USE_SIO == TRUE) || (HAL_USE_UART == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#if HAL_USE_SIO == TRUE
#if EFR32_SIO_USE_EUSART1 || defined(__DOXYGEN__)
extern SIODriver SIOD1;
#endif
#if EFR32_SIO_USE_EUSART2 || defined(__DOXYGEN__)
extern SIODriver SIOD2;
#endif
#if EFR32_SIO_USE_EUSART3 || defined(__DOXYGEN__)
extern SIODriver SIOD3;
#endif
#if EFR32_SIO_USE_USART1 || defined(__DOXYGEN__)
extern SIODriver SIOD4;
#endif
#endif /* HAL_USE_SIO */

#if HAL_USE_UART == TRUE
#if EFR32_UART_USE_EUSART1 || defined(__DOXYGEN__)
extern UARTDriver UARTD1;
#endif
#if EFR32_UART_USE_EUSART2 || defined(__DOXYGEN__)
extern UARTDriver UARTD2;
#endif
#if EFR32_UART_USE_EUSART3 || defined(__DOXYGEN__)
extern UARTDriver UARTD3;
#endif
#if EFR32_UART_USE_USART1 || defined(__DOXYGEN__)
extern UARTDriver UARTD4;
#endif
#endif /* HAL_USE_UART */

/*===========================================================================*/
/* Shared IRQ handlers.                                                      */
/*===========================================================================*/

#define EUSART_IRQ_DISPATCH(idx, sio_en, uart_en)                          \
  OSAL_IRQ_PROLOGUE();                                                      \
  if (sio_en) { sio_lld_serve_interrupt(&SIOD##idx); }                     \
  else if (uart_en) { uart_lld_serve_interrupt(&UARTD##idx); }             \
  OSAL_IRQ_EPILOGUE()

#if (EFR32_SIO_USE_EUSART1 || EFR32_UART_USE_EUSART1) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_EUSART1_RX_HANDLER) {
  EUSART_IRQ_DISPATCH(1, EFR32_SIO_USE_EUSART1, EFR32_UART_USE_EUSART1);
}
OSAL_IRQ_HANDLER(EFR32_EUSART1_TX_HANDLER) {
  EUSART_IRQ_DISPATCH(1, EFR32_SIO_USE_EUSART1, EFR32_UART_USE_EUSART1);
}
#endif

#if (EFR32_SIO_USE_EUSART2 || EFR32_UART_USE_EUSART2) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_EUSART2_RX_HANDLER) {
  EUSART_IRQ_DISPATCH(2, EFR32_SIO_USE_EUSART2, EFR32_UART_USE_EUSART2);
}
OSAL_IRQ_HANDLER(EFR32_EUSART2_TX_HANDLER) {
  EUSART_IRQ_DISPATCH(2, EFR32_SIO_USE_EUSART2, EFR32_UART_USE_EUSART2);
}
#endif

#if (EFR32_SIO_USE_EUSART3 || EFR32_UART_USE_EUSART3) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_EUSART3_RX_HANDLER) {
  EUSART_IRQ_DISPATCH(3, EFR32_SIO_USE_EUSART3, EFR32_UART_USE_EUSART3);
}
OSAL_IRQ_HANDLER(EFR32_EUSART3_TX_HANDLER) {
  EUSART_IRQ_DISPATCH(3, EFR32_SIO_USE_EUSART3, EFR32_UART_USE_EUSART3);
}
#endif

#if (EFR32_SIO_USE_USART1 || EFR32_UART_USE_USART1) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_USART1_RX_HANDLER) {
  EUSART_IRQ_DISPATCH(4, EFR32_SIO_USE_USART1, EFR32_UART_USE_USART1);
}
OSAL_IRQ_HANDLER(EFR32_USART1_TX_HANDLER) {
  EUSART_IRQ_DISPATCH(4, EFR32_SIO_USE_USART1, EFR32_UART_USE_USART1);
}
#endif

#endif /* HAL_USE_SIO || HAL_USE_UART */

/** @} */
