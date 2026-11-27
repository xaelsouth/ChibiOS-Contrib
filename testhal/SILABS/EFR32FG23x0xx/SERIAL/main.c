#include <string.h>
#include "ch.h"
#include "hal.h"

/* Read these fields with a debugger after done becomes one. */
volatile struct {
  uint32_t done;
  uint32_t start;
  uint32_t single;
  uint32_t repeated;
  uint32_t restart;
#if EFR32_SERIAL_USE_TX_DMA || EFR32_SERIAL_USE_RX_DMA
  uint32_t dma_exhaustion;
#endif
#if EFR32_SERIAL_USE_TX_DMA
  uint32_t dma_bulk;
#endif
#if EFR32_SERIAL_USE_RX_DMA
  uint32_t dma_rx_burst;
  uint32_t dma_rx_overrun;
  uint32_t dma_rx_recovery;
#endif
} serial_results;

__attribute__((noinline, used)) void serial_test_finished(void) {
  while (true)
    chThdSleepMilliseconds(1000);
}

#if EFR32_SERIAL_USE_TX_DMA || EFR32_SERIAL_USE_RX_DMA
static bool dma_exhaustion(const SerialConfig *config) {
  const efr32_dma_stream_t *streams[EFR32_DMA_STREAMS];
  unsigned count = 0U;
  const unsigned reserve = EFR32_DMA_STREAMS -
                           (EFR32_SERIAL_USE_TX_DMA && EFR32_SERIAL_USE_RX_DMA ? 1U : 0U);
  while (count < reserve) {
    streams[count] = dmaStreamAlloc(EFR32_DMA_STREAM_ID_ANY,
                                    EFR32_SERIAL_DMA_IRQ_PRIORITY, NULL, NULL);
    if (streams[count] == NULL)
      break;
    count++;
  }
  bool ok = count == reserve &&
            sdStart(&SD1, config) == HAL_RET_NO_RESOURCE &&
            SD1.state == SD_STOP;
#if EFR32_SERIAL_USE_TX_DMA && EFR32_SERIAL_USE_RX_DMA
  const efr32_dma_stream_t *last = dmaStreamAlloc(EFR32_DMA_STREAM_ID_ANY,
                                                   EFR32_SERIAL_DMA_IRQ_PRIORITY,
                                                   NULL, NULL);
  ok = ok && last != NULL;
  if (last != NULL)
    dmaStreamFree(last);
#endif
  while (count > 0U)
    dmaStreamFree(streams[--count]);
  return ok;
}
#endif

int main(void) {
  static const SerialConfig config = {.speed = 115200U};
  halInit();
  chSysInit();
#if EFR32_SERIAL_USE_TX_DMA || EFR32_SERIAL_USE_RX_DMA
  serial_results.dma_exhaustion = dma_exhaustion(&config);
#endif

  palSetPadMode(GPIOA, 9, PAL_MODE_OUTPUT_PUSHPULL |
                PAL_MODE_ALTERNATE(USART0_TX));
  serial_results.start = sdStart(&SD1, &config) == HAL_RET_SUCCESS;
  USART0->CTRL_SET = USART_CTRL_LOOPBK;
  chThdSleepMilliseconds(5);

  serial_results.single =
    sdPutTimeout(&SD1, 0x5AU, TIME_MS2I(500)) == MSG_OK &&
    sdGetTimeout(&SD1, TIME_MS2I(500)) == 0x5A;
  serial_results.repeated = 1U;
  for (unsigned i = 0U; i < 300U; i++) {
    if (sdPutTimeout(&SD1, (uint8_t)i, TIME_MS2I(500)) != MSG_OK ||
        sdGetTimeout(&SD1, TIME_MS2I(500)) != (msg_t)(uint8_t)i) {
      serial_results.repeated = 0U;
      break;
    }
  }
#if EFR32_SERIAL_USE_TX_DMA
  const uint8_t tx[] = {0, 1, 2, 3, 4, 5, 6, 7};
  uint8_t rx[sizeof(tx)] = {0};
  serial_results.dma_bulk =
    chnWriteTimeout(&SD1, tx, sizeof(tx), TIME_MS2I(500)) == sizeof(tx) &&
    chnReadTimeout(&SD1, rx, sizeof(rx), TIME_MS2I(500)) == sizeof(rx) &&
    memcmp(tx, rx, sizeof(tx)) == 0;
#endif
#if EFR32_SERIAL_USE_RX_DMA
  uint8_t burst_tx[96], burst_rx[96] = {0U};
  for (size_t i = 0U; i < sizeof(burst_tx); i++)
    burst_tx[i] = (uint8_t)(i * 3U);
  size_t burst_written = chnWriteTimeout(&SD1, burst_tx, sizeof(burst_tx),
                                         TIME_MS2I(500));
  chThdSleepMilliseconds(30);
  size_t burst_read = chnReadTimeout(&SD1, burst_rx, sizeof(burst_rx),
                                     TIME_MS2I(500));
  serial_results.dma_rx_burst = burst_written == sizeof(burst_tx) &&
                                burst_read == sizeof(burst_rx) &&
                                memcmp(burst_tx, burst_rx, sizeof(burst_tx)) == 0;
#endif
  sdStop(&SD1);
  serial_results.restart = sdStart(&SD1, &config) == HAL_RET_SUCCESS;
  USART0->CTRL_SET = USART_CTRL_LOOPBK;
  chThdSleepMilliseconds(5);
  serial_results.restart = serial_results.restart &&
    sdPutTimeout(&SD1, 0xA5U, TIME_MS2I(500)) == MSG_OK &&
    sdGetTimeout(&SD1, TIME_MS2I(500)) == 0xA5;
#if EFR32_SERIAL_USE_RX_DMA
  event_listener_t listener;
  uint8_t overflow[220];
  memset(overflow, 0x5AU, sizeof(overflow));
  chEvtRegisterMaskWithFlags(chnGetEventSource(&SD1), &listener,
                             EVENT_MASK(0), SD_OVERRUN_ERROR);
  size_t overflow_written = chnWriteTimeout(&SD1, overflow, sizeof(overflow),
                                           TIME_MS2I(500));
  chThdSleepMilliseconds(30);
  eventflags_t flags = chEvtGetAndClearFlags(&listener);
  chEvtUnregister(chnGetEventSource(&SD1), &listener);
  serial_results.dma_rx_overrun = overflow_written == sizeof(overflow) &&
                                  (flags & SD_OVERRUN_ERROR) != 0U;
  while (sdGetTimeout(&SD1, TIME_MS2I(10)) >= MSG_OK) {}
  serial_results.dma_rx_recovery =
    sdPutTimeout(&SD1, 0xC3U, TIME_MS2I(500)) == MSG_OK &&
    sdGetTimeout(&SD1, TIME_MS2I(500)) == 0xC3;
#endif
  sdStop(&SD1);
  serial_results.done = 1U;
  serial_test_finished();
}
