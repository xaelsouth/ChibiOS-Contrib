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

static const SPIConfig spi_cfg = {
  .end_cb   = NULL,
#if (SPI_SELECT_MODE == SPI_SELECT_MODE_PAD) || defined(__DOXYGEN__)
  .ssport   = GPIOA,
  .sspad    = 8,
#endif
  .bitrate  = 1000000U,
  .cfg0     = (0U << 0) | (16U << 5),
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
  spiSelect(&SPID1);

  uint8_t tx[16], rx[16];
  for (int i = 0; i < 16; i++) { tx[i] = (uint8_t)(0xA0 + i); rx[i] = 0; }
  spiExchange(&SPID1, 16, tx, rx);
  spiUnselect(&SPID1);
  int ok = memcmp(tx, rx, 16) == 0;
  PRINTF("sent/received %s\r\n", ok ? "match: PASS" : "MISMATCH: FAIL (loopback wire?)");
  spiStop(&SPID1);
  while (true) chThdSleepMilliseconds(1000);
}
