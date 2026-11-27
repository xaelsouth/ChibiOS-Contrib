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

static const WDGConfig wdg_cfg = {
  .clksel   = EFR32_WDOG_CLKSEL_LFXO,
  .persel   = EFR32_WDOG_PERSEL_12,       /* 2^15 cycles @32768 = 1 s */
  .warnsel  = EFR32_WDOG_WARNSEL_DIS,
  .em1run   = true,
  .em2run   = false,
  .em3run   = false,
  .debugrun = true,
  .lock     = false,
  .warn_cb  = NULL,
};

int main(void) {
  halInit();
  chSysInit();

  palSetPadMode(GPIOB, 2, PAL_MODE_OUTPUT_PUSHPULL);
  console_init();
  PRINTF("\r\nWDG test: kicking watchdog 10x (~1s timeout, kick every 100ms)\r\n");

  wdgStart(&WDGD1, &wdg_cfg);
  bool enabled = (WDOG0->EN & WDOG_EN_EN) != 0U;
  for (int i = 0; i < 10; i++) {
    palTogglePad(GPIOB, 2);
    PRINTF("kick %d\r\n", i + 1);
    wdgReset(&WDGD1);
    chThdSleepMilliseconds(100);
  }
  PRINTF("stopping watchdog - no reset expected\r\n");
  wdgStop(&WDGD1);
  CMU->CLKEN0_SET = CMU_CLKEN0_WDOG0;
  bool stopped = (WDOG0->EN & WDOG_EN_EN) == 0U;
  CMU->CLKEN0_CLR = CMU_CLKEN0_WDOG0;
  chThdSleepMilliseconds(2000);
  PRINTF("enabled=%u stopped=%u survived after stop: %s\r\n",
         (unsigned)enabled, (unsigned)stopped,
         enabled && stopped ? "PASS" : "FAIL");
  while (true) { palTogglePad(GPIOB, 2); chThdSleepMilliseconds(500); }
}
