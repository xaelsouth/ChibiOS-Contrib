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
  PRINTF("\r\nRTC test (BURTC on RTCD1)\r\n");

  RTCDateTime ts;
  rtcGetTime(&RTCD1, &ts);
  PRINTF("current: %04d-%02d-%02d %02d:%02d:%02d.%03u dow=%u\r\n",
         ts.year + 1980, ts.month, ts.day,
         ts.millisecond / 3600000, (ts.millisecond / 60000) % 60,
         (ts.millisecond / 1000) % 60, ts.millisecond % 1000, ts.dayofweek);

  RTCDateTime set_ts = { .year = 46, .month = 11, .day = 22, .dayofweek = 0,
                        .millisecond = 2*3600000UL + 3*60000UL + 53*1000UL, .dstflag = 0 };
  rtcSetTime(&RTCD1, &set_ts);
  chThdSleepMilliseconds(10);
  rtcGetTime(&RTCD1, &ts);
  PRINTF("after set: %04d-%02d-%02d %02d:%02d:%02d.%03u\r\n",
         ts.year + 1980, ts.month, ts.day,
         ts.millisecond / 3600000, (ts.millisecond / 60000) % 60,
         (ts.millisecond / 1000) % 60, ts.millisecond % 1000);
  PRINTF("set/get round-trip: %s\r\n",
         (ts.year == 46 && ts.month == 11 && ts.day == 22) ? "PASS" : "FAIL");

  /* Print the time every second. */
  while (true) {
    rtcGetTime(&RTCD1, &ts);
    PRINTF("now: %04d-%02d-%02d %02d:%02d:%02d.%03u\r\n",
           ts.year + 1980, ts.month, ts.day,
           ts.millisecond / 3600000, (ts.millisecond / 60000) % 60,
           (ts.millisecond / 1000) % 60, ts.millisecond % 1000);
    chThdSleepMilliseconds(1000);
  }
}
