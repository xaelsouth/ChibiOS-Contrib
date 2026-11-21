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

static volatile uint32_t cmp0_hit = 0;

static void sysrtc_cb(efr32_sysrtc_evt_t evt, uint32_t value, void *arg) {
  (void)arg;
  if (evt == EFR32_SYSRTC_EVT_CMP1) cmp0_hit = value;  /* CMP1: CMP0 is owned by SYSTICKv2 */
}

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nSYSRTC test (SYSRTC0, SYSTICKv2 system tick)\r\n");

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
