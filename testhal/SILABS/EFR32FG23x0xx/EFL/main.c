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

/* FG23: USERDATA page 1 KB at 0x0FE00000 (outside the 512 KB main flash). */
#define TEST_ADDR   0x0FE00000UL
#define TEST_LEN    256U

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nEFL test: USERDATA page @0x%08lX\r\n", (unsigned long)TEST_ADDR);

  static uint8_t wbuf[TEST_LEN], rbuf[TEST_LEN];
  for (unsigned i = 0; i < TEST_LEN; i++) wbuf[i] = (uint8_t)(i * 7 + 1);

  eflStart(&EFLD1, NULL);

  /* Find the sector containing TEST_ADDR. */
  const flash_descriptor_t *d = flashGetDescriptor((BaseFlash *)&EFLD1);
  flash_sector_t sec = 0;
  for (flash_sector_t s = 0; s < d->sectors_count; s++) {
    flash_offset_t off = flashGetSectorOffset((BaseFlash *)&EFLD1, s);
    if (TEST_ADDR >= off && TEST_ADDR < off + flashGetSectorSize((BaseFlash *)&EFLD1, s)) {
      sec = s;
      break;
    }
  }
  PRINTF("sector %u covers the USERDATA page\r\n", (unsigned)sec);

  bool ok = true;
  flash_error_t fe = flashStartEraseSector((BaseFlash *)&EFLD1, sec);
  while (fe == FLASH_BUSY_ERASING) {
    uint32_t ms = 1;
    fe = flashQueryErase((BaseFlash *)&EFLD1, &ms);
    chThdSleepMilliseconds(ms != 0U ? ms : 1U);
  }
  ok &= (fe == FLASH_NO_ERROR);
  ok &= (flashVerifyErase((BaseFlash *)&EFLD1, sec) == FLASH_NO_ERROR);
  ok &= (flashProgram((BaseFlash *)&EFLD1, TEST_ADDR, TEST_LEN, wbuf) == FLASH_NO_ERROR);
  ok &= (flashRead((BaseFlash *)&EFLD1, TEST_ADDR, TEST_LEN, rbuf) == FLASH_NO_ERROR);
  ok &= (memcmp(wbuf, rbuf, TEST_LEN) == 0);

  PRINTF("erase+program+read+verify: %s\r\n", ok ? "PASS" : "FAIL");
  eflStop(&EFLD1);
  while (true) chThdSleepMilliseconds(1000);
}
