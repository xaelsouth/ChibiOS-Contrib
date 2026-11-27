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

static bool loopback(const uint8_t *pattern, size_t size) {
  uint8_t rxbuf[64] = {0U};
  size_t written = 0U, received = 0U;
  systime_t start = chVTGetSystemTimeX();

  while ((written < size || received < size) &&
         chVTTimeElapsedSinceX(start) < TIME_MS2I(2000)) {
    if (written < size) {
      written += sioAsyncWrite(&SIOD1, &pattern[written], size - written);
    }
    if (received < size) {
      received += sioAsyncRead(&SIOD1, &rxbuf[received], size - received);
    }
  }
  msg_t end = sioSynchronizeTXEnd(&SIOD1, TIME_MS2I(2000));
  bool ok = written == size && received == size && end == MSG_OK &&
            memcmp(pattern, rxbuf, size) == 0;
  PRINTF("loopback: TX=%u RX=%u end=%d %s\r\n",
         (unsigned)written, (unsigned)received, (int)end,
         ok ? "PASS" : "FAIL");
  return ok;
}

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nSIO test: EUSART0 internal loopback 115200\r\n");

  palSetPadMode(GPIOA, 9, PAL_MODE_OUTPUT_PUSHPULL | PAL_MODE_ALTERNATE(EUSART0_TX));
  palSetPadMode(GPIOA, 10, PAL_MODE_INPUT_PULLUP | PAL_MODE_ALTERNATE(EUSART0_RX));
  sioStart(&SIOD1, &sio_cfg);

  uint8_t pattern[64];
  for (size_t i = 0; i < sizeof(pattern); i++) {
    pattern[i] = (uint8_t)(0x30U + (i % 40U));
  }
  bool small_ok = loopback(pattern, 4U);
  bool dma_allocated = SIOD1.dma_tx != NULL;
  bool dma_ok = loopback(pattern, sizeof(pattern));

  sioStop(&SIOD1);
  PRINTF("DMA allocated=%u; result: %s\r\n", (unsigned)dma_allocated,
         small_ok && dma_allocated && dma_ok ? "PASS" : "FAIL");
  while (true) { chThdSleepMilliseconds(1000); }
}
