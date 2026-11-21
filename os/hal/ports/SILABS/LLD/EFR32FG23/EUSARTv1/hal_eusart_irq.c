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
 * @brief   Shared EUSART/USART interrupt handlers for SIO, UART and SERIAL.
 *
 * @details Each EUSART/USART IRQ handler dispatches to the active driver
 *          (SIO, UART or SERIAL) for that instance. This avoids duplicate
 *          symbol definitions when multiple drivers are enabled.
 *
 * @addtogroup EUSART
 * @{
 */

#include "hal.h"

#if (HAL_USE_SIO == TRUE) || (HAL_USE_UART == TRUE) || (HAL_USE_SERIAL == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* Fallback defaults for driver enable macros when the corresponding HAL
   driver is not compiled in. */
#if !defined(EFR32_UART_USE_EUSART1) || defined(__DOXYGEN__)
#define EFR32_UART_USE_EUSART1    FALSE
#endif
#if !defined(EFR32_UART_USE_EUSART2) || defined(__DOXYGEN__)
#define EFR32_UART_USE_EUSART2    FALSE
#endif
#if !defined(EFR32_UART_USE_EUSART3) || defined(__DOXYGEN__)
#define EFR32_UART_USE_EUSART3    FALSE
#endif
#if !defined(EFR32_UART_USE_USART1) || defined(__DOXYGEN__)
#define EFR32_UART_USE_USART1     FALSE
#endif
#if !defined(EFR32_SERIAL_USE_USART1) || defined(__DOXYGEN__)
#define EFR32_SERIAL_USE_USART1   FALSE
#endif

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

#if HAL_USE_SERIAL == TRUE
#if EFR32_SERIAL_USE_USART1 || defined(__DOXYGEN__)
extern SerialDriver SD1;
#endif
#endif /* HAL_USE_SERIAL */

/*===========================================================================*/
/* Shared IRQ handlers.                                                      */
/*===========================================================================*/

#if (EFR32_SIO_USE_EUSART1 || EFR32_UART_USE_EUSART1) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_EUSART1_RX_HANDLER) {
  OSAL_IRQ_PROLOGUE();
#if EFR32_SIO_USE_EUSART1
  sio_lld_serve_interrupt(&SIOD1);
#elif EFR32_UART_USE_EUSART1
  uart_lld_serve_interrupt(&UARTD1);
#endif
  OSAL_IRQ_EPILOGUE();
}
OSAL_IRQ_HANDLER(EFR32_EUSART1_TX_HANDLER) {
  OSAL_IRQ_PROLOGUE();
#if EFR32_SIO_USE_EUSART1
  sio_lld_serve_interrupt(&SIOD1);
#elif EFR32_UART_USE_EUSART1
  uart_lld_serve_interrupt(&UARTD1);
#endif
  OSAL_IRQ_EPILOGUE();
}
#endif

#if (EFR32_SIO_USE_EUSART2 || EFR32_UART_USE_EUSART2) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_EUSART2_RX_HANDLER) {
  OSAL_IRQ_PROLOGUE();
#if EFR32_SIO_USE_EUSART2
  sio_lld_serve_interrupt(&SIOD2);
#elif EFR32_UART_USE_EUSART2
  uart_lld_serve_interrupt(&UARTD2);
#endif
  OSAL_IRQ_EPILOGUE();
}
OSAL_IRQ_HANDLER(EFR32_EUSART2_TX_HANDLER) {
  OSAL_IRQ_PROLOGUE();
#if EFR32_SIO_USE_EUSART2
  sio_lld_serve_interrupt(&SIOD2);
#elif EFR32_UART_USE_EUSART2
  uart_lld_serve_interrupt(&UARTD2);
#endif
  OSAL_IRQ_EPILOGUE();
}
#endif

#if (EFR32_SIO_USE_EUSART3 || EFR32_UART_USE_EUSART3) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_EUSART3_RX_HANDLER) {
  OSAL_IRQ_PROLOGUE();
#if EFR32_SIO_USE_EUSART3
  sio_lld_serve_interrupt(&SIOD3);
#elif EFR32_UART_USE_EUSART3
  uart_lld_serve_interrupt(&UARTD3);
#endif
  OSAL_IRQ_EPILOGUE();
}
OSAL_IRQ_HANDLER(EFR32_EUSART3_TX_HANDLER) {
  OSAL_IRQ_PROLOGUE();
#if EFR32_SIO_USE_EUSART3
  sio_lld_serve_interrupt(&SIOD3);
#elif EFR32_UART_USE_EUSART3
  uart_lld_serve_interrupt(&UARTD3);
#endif
  OSAL_IRQ_EPILOGUE();
}
#endif

#if (EFR32_SIO_USE_USART1 || EFR32_UART_USE_USART1 || EFR32_SERIAL_USE_USART1) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_USART1_RX_HANDLER) {
  OSAL_IRQ_PROLOGUE();
#if EFR32_SIO_USE_USART1
  sio_lld_serve_interrupt(&SIOD4);
#elif EFR32_UART_USE_USART1
  uart_lld_serve_interrupt(&UARTD4);
#elif EFR32_SERIAL_USE_USART1
  sd_lld_serve_interrupt(&SD1);
#endif
  OSAL_IRQ_EPILOGUE();
}
OSAL_IRQ_HANDLER(EFR32_USART1_TX_HANDLER) {
  OSAL_IRQ_PROLOGUE();
#if EFR32_SIO_USE_USART1
  sio_lld_serve_interrupt(&SIOD4);
#elif EFR32_UART_USE_USART1
  uart_lld_serve_interrupt(&UARTD4);
#elif EFR32_SERIAL_USE_USART1
  sd_lld_serve_interrupt(&SD1);
#endif
  OSAL_IRQ_EPILOGUE();
}
#endif

#endif /* HAL_USE_SIO || HAL_USE_UART || HAL_USE_SERIAL */

/** @} */
