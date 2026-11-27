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

/* FG23: USERDATA page 1 KB at 0x0FE00000 = flash offset EFR32_FLASH_SIZE. */
#define TEST_LEN    256U

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nEFL test: USERDATA page (last sector)\r\n");

  static uint8_t wbuf[TEST_LEN] __attribute__((aligned(4)));
  static uint8_t rbuf[EFR32_USERDATA_SIZE];
  static uint8_t original[EFR32_USERDATA_SIZE] __attribute__((aligned(4)));
  for (unsigned i = 0; i < TEST_LEN; i++) wbuf[i] = (uint8_t)(i * 7 + 1);

  eflStart(&EFLD1, NULL);

  const flash_descriptor_t *d = flashGetDescriptor((BaseFlash *)&EFLD1);
  flash_sector_t sec = d->sectors_count - 1U;   /* last sector = USERDATA */
  flash_offset_t TEST_ADDR = flashGetSectorOffset((BaseFlash *)&EFLD1, sec);
  PRINTF("sector %u @ offset 0x%08lX covers the USERDATA page\r\n",
         (unsigned)sec, (unsigned long)TEST_ADDR);

  if (flashGetSectorSize((BaseFlash *)&EFLD1, sec) != sizeof(original) ||
      flashRead((BaseFlash *)&EFLD1, TEST_ADDR, sizeof(original), original)
        != FLASH_NO_ERROR) {
    PRINTF("USERDATA backup: FAIL; no erase attempted\r\n");
    while (true) chThdSleepMilliseconds(1000);
  }

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

  fe = flashStartEraseSector((BaseFlash *)&EFLD1, sec);
  while (fe == FLASH_BUSY_ERASING) {
    uint32_t ms = 1;
    fe = flashQueryErase((BaseFlash *)&EFLD1, &ms);
    chThdSleepMilliseconds(ms != 0U ? ms : 1U);
  }
  bool restored = (fe == FLASH_NO_ERROR);
  if (restored) {
    restored = flashProgram((BaseFlash *)&EFLD1, TEST_ADDR, sizeof(original),
                            original) == FLASH_NO_ERROR;
  }
  if (restored) {
    restored = flashRead((BaseFlash *)&EFLD1, TEST_ADDR, sizeof(rbuf), rbuf)
                 == FLASH_NO_ERROR && memcmp(original, rbuf, sizeof(rbuf)) == 0;
  }
  PRINTF("USERDATA restored and verified: %s\r\n", restored ? "PASS" : "FAIL");
  ok = ok && restored;
  PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL");
  eflStop(&EFLD1);
  while (true) chThdSleepMilliseconds(1000);
}
