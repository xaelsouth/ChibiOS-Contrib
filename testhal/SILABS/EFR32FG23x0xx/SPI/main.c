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
#include "chprintf.h"

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

static const SPIConfig spi_cfg = {
  .end_cb   = NULL,
#if (SPI_SELECT_MODE == SPI_SELECT_MODE_PAD) || defined(__DOXYGEN__)
  .ssport   = GPIOA,
  .sspad    = 8,
#endif
  .bitrate  = 1000000U,
  .cfg0     = (0U << 0) | (0U << 5),
  .framecfg = EFR32_SIO_LLD_EUSART_8N1,
};

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nSPI test: EUSART0 (SPID1) loopback PA9/PA10 @1MHz\r\n");

  palSetPadMode(GPIOA, 8,  PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOA, 9,  PAL_MODE_OUTPUT_PUSHPULL | PAL_MODE_ALTERNATE(EUSART0_TX));
  palSetPadMode(GPIOA, 10, PAL_MODE_INPUT_PULLUP  | PAL_MODE_ALTERNATE(EUSART0_RX));

  spiStart(&SPID1, &spi_cfg);

  uint8_t tx[16], rx[16];
  for (int i = 0; i < 16; i++) { tx[i] = (uint8_t)(0xA0 + i); rx[i] = 0; }
  /* Polled LLD: spiStartExchangeI completes the transfer synchronously. */
  osalSysLock();
  spiSelectI(&SPID1);
  spiStartExchangeI(&SPID1, 16, tx, rx);
  spiUnselectI(&SPID1);
  osalSysUnlock();
  int ok = memcmp(tx, rx, 16) == 0;
  PRINTF("sent/received %s\r\n", ok ? "match: PASS" : "MISMATCH: FAIL (loopback wire?)");
  spiStop(&SPID1);
  while (true) chThdSleepMilliseconds(1000);
}
