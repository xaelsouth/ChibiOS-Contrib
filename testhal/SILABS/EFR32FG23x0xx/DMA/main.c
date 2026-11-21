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

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nDMA test: LDMA mem-to-mem 256 bytes\r\n");

  static uint8_t src[256], dst[256];
  for (int i = 0; i < 256; i++) { src[i] = (uint8_t)(i ^ 0x5A); dst[i] = 0; }

  const efr32_dma_stream_t *st = dmaStreamAlloc(EFR32_DMA_STREAM_ID_ANY, 4, NULL, NULL);
  if (st == NULL) {
    PRINTF("FAIL: no DMA stream\r\n");
  } else {
    dmaStartMemCopy(st, 0, src, dst, 256);
    dmaWaitCompletion(st);
    dmaStreamFree(st);
    PRINTF("copy verified: %s\r\n", memcmp(src, dst, 256) == 0 ? "PASS" : "FAIL");
  }
  while (true) chThdSleepMilliseconds(1000);
}
