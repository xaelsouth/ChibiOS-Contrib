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

#include <string.h>
#include "ch.h"
#include "hal.h"

/* Console via SEGGER RTT (up channel 0) — works over J-Link SWD.
   View with: JLinkRTTLoggerExe -Device EFR32FG23B010F512IM48 -If SWD -Speed 4000 -RTTChannel 0 out.log */
#include <string.h>
#include <stdio.h>

typedef struct {
  char           id[16];        /* "SEGGER RTT" */
  uint32_t       num_up;
  uint32_t       num_down;
  struct {
    const char  *name;
    uint8_t     *buf;
    uint32_t     size;
    volatile uint32_t wr;
    volatile uint32_t rd;
    uint32_t     flags;
  } up[1];
} rtt_cb_t;

static rtt_cb_t rtt_cb __attribute__((aligned(32)));
static uint8_t  rtt_buf[1024];

static void console_init(void) {
  memset(&rtt_cb, 0, sizeof(rtt_cb));
  memcpy(rtt_cb.id, "SEGGER RTT", 10);
  rtt_cb.num_up = 1;
  rtt_cb.num_down = 0;
  rtt_cb.up[0].name  = "Terminal";
  rtt_cb.up[0].buf   = rtt_buf;
  rtt_cb.up[0].size  = sizeof(rtt_buf);
  rtt_cb.up[0].flags = 0;
}

__attribute__((noinline, used)) void sio_test_finished(void) {
  while (true)
    chThdSleepMilliseconds(1000);
}

static void rtt_write(const char *s, size_t n) {
  uint32_t wp = rtt_cb.up[0].wr;
  for (size_t i = 0; i < n; i++) {
    rtt_buf[wp] = (uint8_t)s[i];
    wp = (wp + 1U) % rtt_cb.up[0].size;
  }
  rtt_cb.up[0].wr = wp;
}

#define PRINTF(...) do {   char _b[256];   int _n = snprintf(_b, sizeof(_b), __VA_ARGS__);   if (_n > 0) rtt_write(_b, (size_t)_n); } while (0)


static const SIOConfig sio_cfg = {
  .baud = 115200U,
  .cfg0 = EUSART_CFG0_LOOPBK,
  .framecfg = EFR32_SIO_LLD_EUSART_8N1,
};

static bool loopback(SIODriver *siop, const uint8_t *pattern, size_t size) {
  uint8_t rxbuf[64] = {0U};
  size_t written = 0U, received = 0U;
  systime_t start = chVTGetSystemTimeX();

  while ((written < size || received < size) &&
         chVTTimeElapsedSinceX(start) < TIME_MS2I(2000)) {
    if (written < size) {
      written += sioAsyncWrite(siop, &pattern[written], size - written);
    }
    if (received < size) {
      received += sioAsyncRead(siop, &rxbuf[received], size - received);
    }
  }
  msg_t end = sioSynchronizeTXEnd(siop, TIME_MS2I(2000));
  bool ok = written == size && received == size && end == MSG_OK &&
            memcmp(pattern, rxbuf, size) == 0;
  PRINTF("loopback: TX=%u RX=%u end=%d %s\r\n",
         (unsigned)written, (unsigned)received, (int)end,
         ok ? "PASS" : "FAIL");
  return ok;
}

static bool blocking_tx_stress(SIODriver *siop) {
  unsigned i;
  for (i = 0U; i < 2048U; i++) {
    if (chnPutTimeout(siop, (uint8_t)i, TIME_MS2I(500)) != MSG_OK)
      break;
  }
  bool ok = i == 2048U && sioSynchronizeTXEnd(siop, TIME_MS2I(500)) == MSG_OK;
  PRINTF("blocking TX stress: %s\r\n", ok ? "PASS" : "FAIL");
  return ok;
}

#if EFR32_SIO_USE_DMA
static bool dma_write_order(SIODriver *siop) {
  uint8_t tx[65], rx[65] = {0U};
  for (size_t i = 0U; i < sizeof(tx); i++)
    tx[i] = (uint8_t)i;

  size_t written = sioAsyncWrite(siop, tx, 64U);
  size_t early = sioAsyncWrite(siop, &tx[64], 1U);
  written += early;
  size_t received = 0U;
  systime_t start = chVTGetSystemTimeX();
  while ((written < sizeof(tx) || received < sizeof(rx)) &&
         chVTTimeElapsedSinceX(start) < TIME_MS2I(2000)) {
    if (written < sizeof(tx))
      written += sioAsyncWrite(siop, &tx[written], sizeof(tx) - written);
    if (received < sizeof(rx))
      received += sioAsyncRead(siop, &rx[received], sizeof(rx) - received);
  }
  bool ok = early == 0U && written == sizeof(tx) && received == sizeof(rx) &&
            sioSynchronizeTXEnd(siop, TIME_MS2I(2000)) == MSG_OK &&
            memcmp(tx, rx, sizeof(tx)) == 0;
  PRINTF("DMA write ordering: %s\r\n", ok ? "PASS" : "FAIL");
  return ok;
}

static bool dma_receive_burst(SIODriver *siop) {
  uint8_t tx[96], rx[96] = {0U};
  for (size_t i = 0U; i < sizeof(tx); i++)
    tx[i] = (uint8_t)(i * 3U);
  size_t written = 0U;
  systime_t start = chVTGetSystemTimeX();
  while (written < sizeof(tx) &&
         chVTTimeElapsedSinceX(start) < TIME_MS2I(2000))
    written += sioAsyncWrite(siop, &tx[written], sizeof(tx) - written);
  msg_t end = sioSynchronizeTXEnd(siop, TIME_MS2I(2000));
  msg_t ready = sioSynchronizeRX(siop, TIME_MS2I(2000));
  size_t received = sioAsyncRead(siop, rx, sizeof(rx));
  bool ok = written == sizeof(tx) && end == MSG_OK && ready == MSG_OK &&
            received == sizeof(rx) && memcmp(tx, rx, sizeof(tx)) == 0;
  PRINTF("DMA buffered RX burst: %s\r\n", ok ? "PASS" : "FAIL");
  return ok;
}

static bool dma_ring_wrap(SIODriver *siop) {
  uint8_t tx[512], rx[512] = {0U};
  for (size_t i = 0U; i < sizeof(tx); i++)
    tx[i] = (uint8_t)(i * 7U);
  size_t written = 0U, received = 0U;
  systime_t start = chVTGetSystemTimeX();
  while ((written < sizeof(tx) || received < sizeof(rx)) &&
         chVTTimeElapsedSinceX(start) < TIME_MS2I(2000)) {
    if (written < sizeof(tx))
      written += sioAsyncWrite(siop, &tx[written], sizeof(tx) - written);
    if (received < sizeof(rx))
      received += sioAsyncRead(siop, &rx[received], sizeof(rx) - received);
  }
  bool ok = written == sizeof(tx) && received == sizeof(rx) &&
            sioSynchronizeTXEnd(siop, TIME_MS2I(2000)) == MSG_OK &&
            memcmp(tx, rx, sizeof(tx)) == 0;
  PRINTF("DMA RX ring wrap: %s\r\n", ok ? "PASS" : "FAIL");
  return ok;
}

static bool dma_receive_overrun(SIODriver *siop) {
  uint8_t tx[200];
  memset(tx, 0x5AU, sizeof(tx));
  size_t written = 0U;
  systime_t start = chVTGetSystemTimeX();
  while (written < sizeof(tx) &&
         chVTTimeElapsedSinceX(start) < TIME_MS2I(2000))
    written += sioAsyncWrite(siop, &tx[written], sizeof(tx) - written);
  msg_t end = sioSynchronizeTXEnd(siop, TIME_MS2I(2000));
  sioevents_t errors = sioGetAndClearErrors(siop);
  bool ok = written == sizeof(tx) && end == MSG_OK &&
            (errors & SIO_EV_OVERRUN_ERR) != 0U;
  uint8_t drain[32], marker = 0xC3U, received = 0U;
  for (unsigned i = 0U; i < 10U; i++) {
    while (sioAsyncRead(siop, drain, sizeof(drain)) != 0U) {}
    chThdSleepMilliseconds(5);
  }
  (void)sioGetAndClearErrors(siop);
  bool recovered = sioAsyncWrite(siop, &marker, 1U) == 1U &&
                  sioSynchronizeRX(siop, TIME_MS2I(500)) == MSG_OK &&
                  sioAsyncRead(siop, &received, 1U) == 1U &&
                  received == marker &&
                  sioSynchronizeTXEnd(siop, TIME_MS2I(500)) == MSG_OK;
  PRINTF("DMA RX overrun: TX=%u end=%d errors=0x%x %s\r\n",
         (unsigned)written, (int)end, (unsigned)errors, ok ? "PASS" : "FAIL");
  PRINTF("DMA RX recovery: %s\r\n", recovered ? "PASS" : "FAIL");
  return ok && recovered;
}
#endif

static THD_WORKING_AREA(rx_wa, 512);
static volatile unsigned rx_errors;

static THD_FUNCTION(rx_thread, arg) {
  const uint8_t *expected = arg;
  uint8_t byte;

  for (size_t i = 0U; i < 12U;) {
    msg_t msg = sioSynchronizeRX(&SIOD1, TIME_MS2I(1000));
    if (msg == SIO_MSG_ERRORS) {
      (void)sioGetAndClearErrors(&SIOD1);
      rx_errors++;
    } else if (msg != MSG_OK) {
      chThdExit(MSG_TIMEOUT);
    } else if (sioAsyncRead(&SIOD1, &byte, 1U) == 1U) {
      if (byte != expected[i++]) {
        chThdExit(MSG_RESET);
      }
    }
  }
  chThdExit(MSG_OK);
}

static bool blocking_rx_recovery(void) {
  static const uint8_t pattern[] = "0123456789AB";
  thread_t *tp;

  rx_errors = 0U;
  tp = chThdCreateStatic(rx_wa, sizeof(rx_wa), NORMALPRIO + 1,
                         rx_thread, (void *)pattern);
  chThdSleepMilliseconds(20);
  EUSART0->IF_SET = EUSART_IF_FERR;
  chThdSleepMilliseconds(20);
  for (size_t i = 0U; i < sizeof(pattern) - 1U; i += 4U) {
    if (sioAsyncWrite(&SIOD1, &pattern[i], 4U) != 4U) {
      (void)chThdWait(tp);
      return false;
    }
    chThdSleepMilliseconds(30);
  }
  msg_t result = chThdWait(tp);
  PRINTF("blocking RX after error: result=%d errors=%u %s\r\n",
         (int)result, rx_errors,
         (result == MSG_OK && rx_errors > 0U) ? "PASS" : "FAIL");
  return result == MSG_OK && rx_errors > 0U;
}

static const SIOConfig usart_cfg = {
  .baud = 115200U,
  .cfg0 = USART_CTRL_LOOPBK,
  .framecfg = EFR32_SIO_LLD_USART_8N1,
};

static THD_WORKING_AREA(usart_rx_wa, 512);
static volatile unsigned usart_rx_errors;

static THD_FUNCTION(usart_rx_thread, arg) {
  uint8_t byte;
  msg_t msg;

  (void)arg;
  do {
    msg = sioSynchronizeRX(&SIOD4, TIME_MS2I(500));
    if (msg == SIO_MSG_ERRORS) {
      (void)sioGetAndClearErrors(&SIOD4);
      usart_rx_errors++;
    }
  } while (msg == SIO_MSG_ERRORS);
  if (msg != MSG_OK) {
    chThdExit(msg);
  }
  if (sioAsyncRead(&SIOD4, &byte, 1U) != 1U || byte != 0x5AU) {
    chThdExit(MSG_RESET);
  }
  chThdExit(MSG_OK);
}

static bool usart_single_byte(bool inject_error) {
  const uint8_t byte = 0x5AU;
  usart_rx_errors = 0U;
  thread_t *tp = chThdCreateStatic(usart_rx_wa, sizeof(usart_rx_wa),
                                   NORMALPRIO + 1, usart_rx_thread, NULL);
  chThdSleepMilliseconds(20);
  if (inject_error) {
    USART0->IF_SET = USART_IF_FERR;
    chThdSleepMilliseconds(20);
  }
  size_t written = sioAsyncWrite(&SIOD4, &byte, 1U);
  msg_t result = chThdWait(tp);
  bool ok = written == 1U && result == MSG_OK &&
            (!inject_error || usart_rx_errors > 0U);
  PRINTF("USART0 single-byte RX: error injection=%u TX=%u result=%d errors=%u %s\r\n",
         (unsigned)inject_error, (unsigned)written, (int)result,
         usart_rx_errors, ok ? "PASS" : "FAIL");
  return ok;
}

#if EFR32_SIO_USE_DMA
static bool dma_exhaustion(void) {
  const efr32_dma_stream_t *streams[EFR32_DMA_STREAMS];
  unsigned count = 0U;
  while (count < EFR32_DMA_STREAMS - 1U) {
    streams[count] = dmaStreamAlloc(EFR32_DMA_STREAM_ID_ANY,
                                    EFR32_SIO_DMA_IRQ_PRIORITY, NULL, NULL);
    if (streams[count] == NULL)
      break;
    count++;
  }
  bool ok = count == EFR32_DMA_STREAMS - 1U &&
            sioStart(&SIOD1, &sio_cfg) == HAL_RET_NO_RESOURCE &&
            SIOD1.state == SIO_STOP;
  const efr32_dma_stream_t *last = dmaStreamAlloc(EFR32_DMA_STREAM_ID_ANY,
                                                   EFR32_SIO_DMA_IRQ_PRIORITY,
                                                   NULL, NULL);
  ok = ok && last != NULL;
  if (last != NULL)
    dmaStreamFree(last);
  while (count > 0U)
    dmaStreamFree(streams[--count]);
  PRINTF("DMA exhaustion: %s\r\n", ok ? "PASS" : "FAIL");
  return ok;
}
#endif

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nSIO test: EUSART0 internal loopback 115200\r\n");
#if EFR32_SIO_USE_DMA
  bool exhaustion_ok = dma_exhaustion();
#endif

  palSetPadMode(GPIOA, 9, PAL_MODE_OUTPUT_PUSHPULL | PAL_MODE_ALTERNATE(EUSART0_TX));
  palSetPadMode(GPIOA, 10, PAL_MODE_INPUT_PULLUP | PAL_MODE_ALTERNATE(EUSART0_RX));
  sioStart(&SIOD1, &sio_cfg);

  uint8_t pattern[64];
  for (size_t i = 0; i < sizeof(pattern); i++) {
    pattern[i] = (uint8_t)(0x30U + (i % 40U));
  }
  bool small_ok = loopback(&SIOD1, pattern, 4U);
  bool bulk_ok = loopback(&SIOD1, pattern, sizeof(pattern));
#if EFR32_SIO_USE_DMA
  bool dma_allocated = SIOD1.dma_tx != NULL;
  bool dma_order_ok = dma_write_order(&SIOD1);
  bool dma_burst_ok = dma_receive_burst(&SIOD1);
  bool dma_wrap_ok = dma_ring_wrap(&SIOD1);
#endif
  bool rx_ok = blocking_rx_recovery();
#if EFR32_SIO_USE_DMA
  bool dma_overrun_ok = dma_receive_overrun(&SIOD1);
#endif
  bool tx_stress_ok = blocking_tx_stress(&SIOD1);

  sioStop(&SIOD1);
#if EFR32_SIO_USE_DMA
  PRINTF("DMA allocated=%u; result: %s\r\n", (unsigned)dma_allocated,
         exhaustion_ok && small_ok && dma_allocated && bulk_ok && dma_order_ok &&
         dma_burst_ok && dma_wrap_ok && dma_overrun_ok && rx_ok && tx_stress_ok ?
         "PASS" : "FAIL");
#else
  PRINTF("EUSART0 result: %s\r\n",
         small_ok && bulk_ok && rx_ok && tx_stress_ok ? "PASS" : "FAIL");
#endif

  palSetPadMode(GPIOA, 9, PAL_MODE_OUTPUT_PUSHPULL | PAL_MODE_ALTERNATE(EUSART0_TX_DIS));
  palSetPadMode(GPIOA, 9, PAL_MODE_OUTPUT_PUSHPULL | PAL_MODE_ALTERNATE(USART0_TX));
  sioStart(&SIOD4, &usart_cfg);
  bool usart_ok = usart_single_byte(false);
  usart_ok = usart_single_byte(true) && usart_ok;
  usart_ok = usart_single_byte(false) && usart_ok;
  bool usart_small_ok = loopback(&SIOD4, pattern, 4U);
  bool usart_bulk_ok = loopback(&SIOD4, pattern, sizeof(pattern));
#if EFR32_SIO_USE_DMA
  bool usart_dma_allocated = SIOD4.dma_tx != NULL;
  bool usart_burst_ok = dma_receive_burst(&SIOD4);
  bool usart_wrap_ok = dma_ring_wrap(&SIOD4);
  bool usart_overrun_ok = dma_receive_overrun(&SIOD4);
#endif
  bool usart_tx_stress_ok = blocking_tx_stress(&SIOD4);
  sioStop(&SIOD4);
#if EFR32_SIO_USE_DMA
  PRINTF("USART0 DMA allocated=%u; result: %s\r\n",
         (unsigned)usart_dma_allocated,
         usart_ok && usart_small_ok && usart_dma_allocated && usart_bulk_ok &&
         usart_burst_ok && usart_wrap_ok && usart_overrun_ok && usart_tx_stress_ok ?
         "PASS" : "FAIL");
#else
  PRINTF("USART0 result: %s\r\n",
         usart_ok && usart_small_ok && usart_bulk_ok && usart_tx_stress_ok ?
         "PASS" : "FAIL");
#endif
  sio_test_finished();
}
