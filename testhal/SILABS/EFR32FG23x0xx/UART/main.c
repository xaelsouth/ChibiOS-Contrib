#include <string.h>
#include "ch.h"
#include "hal.h"

volatile struct {
  uint32_t done;
  uint32_t eusart;
  uint32_t usart;
  uint32_t timeout;
  uint32_t restart;
  uint32_t usart_9bit;
  uint32_t framing_mask;
#if EFR32_UART_USE_DMA
  uint32_t dma_exhaustion;
  uint32_t dma_partial;
  uint32_t dma_large;
  uint32_t dma_tx_cancel;
#endif
} uart_results;

static volatile unsigned tx1_count, tx2_count, rx_count;
static volatile uartflags_t rx_errors;

__attribute__((noinline, used)) void uart_test_finished(void) {
  while (true)
    chThdSleepMilliseconds(1000);
}

static void tx1_cb(UARTDriver *uartp) {
  (void)uartp;
  tx1_count++;
}

static void tx2_cb(UARTDriver *uartp) {
  (void)uartp;
  tx2_count++;
}

static void rx_cb(UARTDriver *uartp) {
  (void)uartp;
  rx_count++;
}

static void rx_error_cb(UARTDriver *uartp, uartflags_t errors) {
  (void)uartp;
  rx_errors |= errors;
}

#if EFR32_UART_USE_DMA
static bool dma_exhaustion(const UARTConfig *config) {
  const efr32_dma_stream_t *streams[EFR32_DMA_STREAMS];
  unsigned count = 0U;

  while (count < EFR32_DMA_STREAMS - 1U) {
    streams[count] = dmaStreamAlloc(EFR32_DMA_STREAM_ID_ANY,
                                    EFR32_UART_DMA_IRQ_PRIORITY, NULL, NULL);
    if (streams[count] == NULL)
      break;
    count++;
  }
  bool ok = count == EFR32_DMA_STREAMS - 1U &&
            uartStart(&UARTD1, config) == HAL_RET_NO_RESOURCE &&
            UARTD1.state == UART_STOP;
  const efr32_dma_stream_t *last = dmaStreamAlloc(EFR32_DMA_STREAM_ID_ANY,
                                                   EFR32_UART_DMA_IRQ_PRIORITY,
                                                   NULL, NULL);
  ok = ok && last != NULL;
  if (last != NULL)
    dmaStreamFree(last);
  while (count > 0U)
    dmaStreamFree(streams[--count]);
  return ok;
}

static bool dma_partial(UARTDriver *uartp, const UARTConfig *config) {
  const uint8_t tx[] = {1U, 2U, 3U, 4U};
  uint8_t rx[8] = {0U};
  if (uartStart(uartp, config) != HAL_RET_SUCCESS)
    return false;
  tx1_count = tx2_count = rx_count = 0U;
  uartStartReceive(uartp, sizeof(rx), rx);
  uartStartSend(uartp, sizeof(tx), tx);
  systime_t start = chVTGetSystemTimeX();
  while (tx2_count == 0U && chVTTimeElapsedSinceX(start) < TIME_MS2I(500))
    chThdSleepMilliseconds(1);
  chThdSleepMilliseconds(5);
  size_t remaining = uartStopReceive(uartp);
  bool ok = tx2_count == 1U && rx_count == 0U && remaining == 4U &&
            memcmp(tx, rx, sizeof(tx)) == 0;
  uartStop(uartp);
  return ok;
}

static bool dma_large(UARTDriver *uartp, const UARTConfig *config) {
  static uint8_t tx[2300], rx[2300];
  for (size_t i = 0U; i < sizeof(tx); i++)
    tx[i] = (uint8_t)(i * 13U);
  memset(rx, 0, sizeof(rx));
  if (uartStart(uartp, config) != HAL_RET_SUCCESS)
    return false;
  tx1_count = tx2_count = rx_count = 0U;
  uartStartReceive(uartp, sizeof(rx), rx);
  uartStartSend(uartp, sizeof(tx), tx);
  systime_t start = chVTGetSystemTimeX();
  while ((tx1_count == 0U || tx2_count == 0U || rx_count == 0U) &&
         chVTTimeElapsedSinceX(start) < TIME_MS2I(2000))
    chThdSleepMilliseconds(1);
  bool ok = tx1_count == 1U && tx2_count == 1U && rx_count == 1U &&
            memcmp(tx, rx, sizeof(tx)) == 0;
  uartStop(uartp);
  return ok;
}

static bool dma_tx_cancel(UARTDriver *uartp, const UARTConfig *config) {
  static uint8_t large[2300];
  const uint8_t tx[] = {0x31U, 0x72U, 0xA3U, 0xF4U};
  uint8_t rx[sizeof(tx)] = {0U};
  if (uartStart(uartp, config) != HAL_RET_SUCCESS)
    return false;
  tx1_count = tx2_count = rx_count = 0U;
  uartStartSend(uartp, sizeof(large), large);
  chThdSleepMilliseconds(5);
  size_t remaining = uartStopSend(uartp);
  chThdSleepMilliseconds(5);
  bool ok = remaining > 0U && remaining < sizeof(large) &&
            tx1_count == 0U && tx2_count == 0U;
  uartStartReceive(uartp, sizeof(rx), rx);
  uartStartSend(uartp, sizeof(tx), tx);
  systime_t start = chVTGetSystemTimeX();
  while ((tx1_count == 0U || tx2_count == 0U || rx_count == 0U) &&
         chVTTimeElapsedSinceX(start) < TIME_MS2I(500))
    chThdSleepMilliseconds(1);
  ok = ok && tx1_count == 1U && tx2_count == 1U && rx_count == 1U &&
       memcmp(tx, rx, sizeof(tx)) == 0;
  uartStop(uartp);
  return ok;
}
#endif

static bool loopback(UARTDriver *uartp, const UARTConfig *config) {
  uint8_t tx[64], rx[64] = {0};
  for (unsigned i = 0U; i < sizeof(tx); i++)
    tx[i] = (uint8_t)(i + 1U);

  tx1_count = tx2_count = rx_count = 0U;
  if (uartStart(uartp, config) != HAL_RET_SUCCESS)
    return false;
  uartStartReceive(uartp, sizeof(rx), rx);
  uartStartSend(uartp, sizeof(tx), tx);
  systime_t start = chVTGetSystemTimeX();
  while ((tx1_count == 0U || tx2_count == 0U || rx_count == 0U) &&
         chVTTimeElapsedSinceX(start) < TIME_MS2I(2000))
    chThdSleepMilliseconds(1);
  bool ok = tx1_count == 1U && tx2_count == 1U && rx_count == 1U &&
            uartp->txstate == UART_TX_IDLE && uartp->rxstate == UART_RX_IDLE &&
            memcmp(tx, rx, sizeof(tx)) == 0;
  return ok;
}

static bool loopback_9bit(UARTDriver *uartp, const UARTConfig *config) {
  const uint16_t tx[] = {0x000U, 0x001U, 0x0FFU, 0x100U, 0x155U, 0x1FFU};
  uint16_t rx[sizeof(tx) / sizeof(tx[0])] = {0};
  const size_t count = sizeof(tx) / sizeof(tx[0]);

  tx1_count = tx2_count = rx_count = 0U;
  if (uartStart(uartp, config) != HAL_RET_SUCCESS)
    return false;
  uartStartReceive(uartp, count, rx);
  uartStartSend(uartp, count, tx);
  systime_t start = chVTGetSystemTimeX();
  while ((tx1_count == 0U || tx2_count == 0U || rx_count == 0U) &&
         chVTTimeElapsedSinceX(start) < TIME_MS2I(2000))
    chThdSleepMilliseconds(1);
  bool ok = tx1_count == 1U && tx2_count == 1U && rx_count == 1U &&
            memcmp(tx, rx, sizeof(tx)) == 0;
  uartStop(uartp);
  return ok;
}

static bool framing_mask(const UARTConfig *config) {
  uint16_t rx = 0xFFFFU;
  uint32_t raw = 0U;
  if (uartStart(&UARTD4, config) != HAL_RET_SUCCESS)
    return false;
  USART0->IEN_CLR = _USART_IEN_MASK;
  USART0->TXDATAX = USART_TXDATAX_TXBREAK;
  systime_t start = chVTGetSystemTimeX();
  while ((USART0->STATUS & USART_STATUS_RXDATAV) == 0U &&
         chVTTimeElapsedSinceX(start) < TIME_MS2I(500))
    chThdSleepMilliseconds(1);
  if ((USART0->STATUS & USART_STATUS_RXDATAV) != 0U)
    raw = USART0->RXDATAX;
  USART0->IF_CLR = _USART_IF_MASK;
  USART0->IEN_SET = USART_IEN_RXDATAV | USART_IEN_RXOF |
                    USART_IEN_PERR | USART_IEN_FERR;
  rx_count = 0U;
  rx_errors = 0U;
  uartStartReceive(&UARTD4, 1U, &rx);
  USART0->TXDATAX = USART_TXDATAX_TXBREAK;
  start = chVTGetSystemTimeX();
  while (rx_count == 0U && chVTTimeElapsedSinceX(start) < TIME_MS2I(500))
    chThdSleepMilliseconds(1);
  bool ok = (raw & USART_RXDATAX_FERR) != 0U &&
            (rx_errors & UART_FRAMING_ERROR) != 0U && rx_count == 1U &&
            rx == (raw & _USART_RXDATAX_RXDATA_MASK);
  uartStop(&UARTD4);
  return ok;
}

int main(void) {
  static const UARTConfig eusart_config = {
    .speed = 115200U,
    .cfg0 = EUSART_CFG0_LOOPBK,
    .framecfg = EUSART_FRAMECFG_DATABITS_EIGHT |
                EUSART_FRAMECFG_PARITY_NONE | EUSART_FRAMECFG_STOPBITS_ONE,
    .txend1_cb = tx1_cb, .txend2_cb = tx2_cb, .rxend_cb = rx_cb,
  };
  static const UARTConfig usart_config = {
    .speed = 115200U,
    .cfg0 = USART_CTRL_LOOPBK,
    .framecfg = USART_FRAME_DATABITS_EIGHT |
                USART_FRAME_PARITY_NONE | USART_FRAME_STOPBITS_ONE,
    .txend1_cb = tx1_cb, .txend2_cb = tx2_cb, .rxend_cb = rx_cb,
  };
  static const UARTConfig usart_9bit_config = {
    .speed = 115200U,
    .cfg0 = USART_CTRL_LOOPBK,
    .framecfg = USART_FRAME_DATABITS_NINE |
                USART_FRAME_PARITY_NONE | USART_FRAME_STOPBITS_ONE,
    .txend1_cb = tx1_cb, .txend2_cb = tx2_cb, .rxend_cb = rx_cb,
    .rxerr_cb = rx_error_cb,
  };
  halInit();
  chSysInit();

#if EFR32_UART_USE_DMA
  uart_results.dma_exhaustion = dma_exhaustion(&eusart_config);
#endif
  palSetPadMode(GPIOA, 9, PAL_MODE_OUTPUT_PUSHPULL |
                PAL_MODE_ALTERNATE(EUSART0_TX));
  uart_results.eusart = loopback(&UARTD1, &eusart_config);
  uartStop(&UARTD1);
  palSetPadMode(GPIOA, 9, PAL_MODE_OUTPUT_PUSHPULL |
                PAL_MODE_ALTERNATE(EUSART0_TX_DIS));
  palSetPadMode(GPIOA, 9, PAL_MODE_OUTPUT_PUSHPULL |
                PAL_MODE_ALTERNATE(USART0_TX));
  uart_results.usart = loopback(&UARTD4, &usart_config);
  uint8_t byte;
  size_t n = 1U;
  uart_results.timeout = uartReceiveTimeout(&UARTD4, &n, &byte,
                                             TIME_MS2I(30)) == MSG_TIMEOUT &&
                         n == 0U;
  uartStop(&UARTD4);
  uart_results.restart = loopback(&UARTD4, &usart_config);
  uartStop(&UARTD4);
  uart_results.usart_9bit = loopback_9bit(&UARTD4, &usart_9bit_config);
  uart_results.framing_mask = framing_mask(&usart_9bit_config);
#if EFR32_UART_USE_DMA
  uart_results.dma_partial = dma_partial(&UARTD4, &usart_config);
  uart_results.dma_large = dma_large(&UARTD4, &usart_config);
  uart_results.dma_tx_cancel = dma_tx_cancel(&UARTD4, &usart_config);
#endif
  uart_results.done = 1U;
  uart_test_finished();
}
