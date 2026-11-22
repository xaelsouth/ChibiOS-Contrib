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
#include "hal_sysrtc_lld.h"

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

static volatile uint32_t cmp0_hit = 0;

static void sysrtc_cb(efr32_sysrtc_evt_t evt, uint32_t value, void *arg) {
  (void)arg;
  if (evt == EFR32_SYSRTC_EVT_CMP1) cmp0_hit = value;  /* CMP1: CMP0 is owned by the system tick */
}

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nSYSRTC test (SYSRTC0, SYSTICKv3 (LETIMER0) system tick, SYSRTC0 as RTC)\r\n");

  sysrtc_lld_start(&SYSRTCD1, sysrtc_cb, NULL);

  uint32_t c0 = sysrtc_lld_get_counter();
  chThdSleepMilliseconds(100);
  uint32_t c1 = sysrtc_lld_get_counter();
  uint32_t delta = c1 - c0;
  PRINTF("counter %u -> %u, delta=%u (expect ~3277 for 100 ms @32768 Hz)\r\n",
         (unsigned)c0, (unsigned)c1, (unsigned)delta);
  PRINTF("rate: %s\r\n", (delta > 3000 && delta < 3600) ? "PASS" : "FAIL");

  cmp0_hit = 0;
  uint32_t cmp_val = c1 + 1000;
  sysrtc_lld_set_compare(&SYSRTCD1, 1, cmp_val);
  sysrtc_lld_enable_compare(&SYSRTCD1, 1);
  uint32_t start = sysrtc_lld_get_counter();
  while (cmp0_hit == 0 && (sysrtc_lld_get_counter() - start) < 5000) {
    chThdSleepMilliseconds(1);
  }
  PRINTF("compare0 alarm: %s (hit=%u want=%u)\r\n",
         cmp0_hit == cmp_val ? "PASS" : "FAIL", (unsigned)cmp0_hit, (unsigned)cmp_val);

  sysrtc_lld_disable_compare(&SYSRTCD1, 1);
  sysrtc_lld_stop(&SYSRTCD1);
  while (true) chThdSleepMilliseconds(1000);
}
