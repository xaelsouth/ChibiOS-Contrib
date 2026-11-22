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
#include "efr32_clocks.h"

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
static uint8_t  rtt_buf[4096];

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

/* RTT-backed BaseSequentialStream so chprintf()-based helpers
   (e.g. efr32_clock_points_print/efr32_clock_tree_print) can write
   to the RTT console too. */
static size_t rtt_stream_write(void *ip, const uint8_t *bp, size_t n) {
  (void)ip;
  rtt_write((const char *)bp, n);
  return n;
}
static msg_t rtt_stream_put(void *ip, uint8_t b) {
  (void)ip;
  rtt_write((const char *)&b, 1);
  return MSG_OK;
}
static const struct BaseSequentialStreamVMT rtt_stream_vmt = {
  .instance_offset = 0,
  .write = rtt_stream_write,
  .read  = NULL,
  .put   = rtt_stream_put,
  .get   = NULL
};
static BaseSequentialStream rtt_stream = { .vmt = &rtt_stream_vmt };

int main(void) {
  halInit();
  chSysInit();

  palSetPadMode(GPIOB, 2, PAL_MODE_OUTPUT_PUSHPULL);
  console_init();
  PRINTF("\r\nTEMPLATE: EFR32FG23 DK2600A hello\r\n");

  /* Dump the clock points via the port helper (same as "clocks" shell cmd). */
  PRINTF("clock points (efr32_clock_points_print):\r\n");
  efr32_clock_points_print(&rtt_stream);

  /* Dump the clock tree (same as "clocks w" shell cmd). */
  efr32_clock_tree_print(&rtt_stream);

#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) || defined(__DOXYGEN__)
  /* Exercise run-time clock switching (both configs are identical here,
     so this only exercises the code path). */
  {
    bool r;
    r = halClockSwitchMode(&hal_clkcfg_reset);
    PRINTF("clock switch reset:   %s\r\n", r ? "failed" : "done");
    r = halClockSwitchMode(&hal_clkcfg_default);
    PRINTF("clock switch default: %s\r\n", r ? "failed" : "done");
    PRINTF("  SYSCLK now %u Hz, HCLK %u Hz\r\n",
           (unsigned)halClockGetPointX(EFR32_CLK_SYSCLK),
           (unsigned)halClockGetPointX(EFR32_CLK_HCLK));
  }
#endif

  while (true) {
    palTogglePad(GPIOB, 2);
    PRINTF("tick\r\n");
    chThdSleepMilliseconds(1000);
  }
}
