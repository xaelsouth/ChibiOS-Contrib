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

/* VCOM console on EUSART2 (SIOD2), PA9=TX / PA10=RX, 115200 8N1. */
static const SIOConfig console_cfg = {
  .baud = 115200U,
  .cfg0 = (0U << 0) | (0U << 5),
  .framecfg = EFR32_SIO_LLD_EUSART_8N1,
};

static void console_init(void) {
  palSetPadMode(GPIOA, 9, PAL_MODE_OUTPUT_PUSHPULL | PAL_MODE_ALTERNATE(EUSART1_TX));
  palSetPadMode(GPIOA, 10, PAL_MODE_INPUT_PULLUP | PAL_MODE_ALTERNATE(EUSART1_RX));
  sioStart(&SIOD2, &console_cfg);
}
#define PRINTF(...) chprintf((BaseSequentialStream *)&SIOD2, __VA_ARGS__)

static const SIOConfig sio_cfg = {
  .baud = 2400U,
  .cfg0 = (0U << 0) | (4U << 5),   /* ASYNC, no oversampling (LF operation) */
  .framecfg = EFR32_SIO_LLD_EUSART_8E1,
};

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nSIO test: EUSART0 loopback 2400 8E1 on PA9/PA10\r\n");

  palSetPadMode(GPIOA, 9, PAL_MODE_OUTPUT_PUSHPULL | PAL_MODE_ALTERNATE(EUSART0_TX));
  palSetPadMode(GPIOA, 10, PAL_MODE_INPUT_PULLUP | PAL_MODE_ALTERNATE(EUSART0_RX));

  sioStart(&SIOD1, &sio_cfg);

  const char *msg = "EUSART0 loopback OK";
  uint8_t rx[32];
  size_t tx = 0, rxgot = 0;
  while (tx < strlen(msg)) {
    tx += sioAsyncWrite(&SIOD1, (const uint8_t *)&msg[tx], strlen(msg) - tx);
    while (rxgot < tx) {
      msg_t m = sioGetX(&SIOD1);
      if (m >= 0) rx[rxgot++] = (uint8_t)m;
    }
  }
  chThdSleepMilliseconds(50);
  while (rxgot < tx) {
    msg_t m = sioGetX(&SIOD1);
    if (m >= 0) rx[rxgot++] = (uint8_t)m;
  }
  rx[rxgot] = 0;
  sioStop(&SIOD1);
  PRINTF("sent %u bytes, got back: %s\r\n", (unsigned)tx, rx);
  PRINTF("result: %s\r\n", (rxgot == strlen(msg) && memcmp(rx, msg, rxgot) == 0) ? "PASS" : "FAIL (no loopback wire?)");

  palSetPadMode(GPIOA, 9, PAL_MODE_ALTERNATE(EUSART0_TX_DIS));
  palSetPadMode(GPIOA, 10, PAL_MODE_ALTERNATE(EUSART0_RX_DIS));

  while (true) { chThdSleepMilliseconds(1000); }
}
