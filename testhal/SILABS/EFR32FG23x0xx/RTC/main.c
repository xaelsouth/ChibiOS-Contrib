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

static volatile uint32_t alarm_fired;

static void rtc_alarm_cb(RTCDriver *rtcp, rtcevent_t evt) {
  (void)rtcp;
  if (evt == RTC_EVENT_ALARM) {
    alarm_fired = 1U;
  }
}

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

  /* Alarm test: alarm lives on SYSRTC CMP1. Set +2 s, wait for the
     callback, measure the elapsed system time. */
  {
    alarm_fired = 0;
    rtcSetCallback(&RTCD1, rtc_alarm_cb);
    RTCAlarm al = { .tv_sec = 2, .tv_usec = 0 };
    systime_t t0 = chVTGetSystemTimeX();
    rtcSetAlarm(&RTCD1, 0, &al);
    RTCAlarm al_back;
    rtcGetAlarm(&RTCD1, 0, &al_back);
    PRINTF("alarm readback: %lu s %lu us (expect ~2 s)\r\n",
           (unsigned long)al_back.tv_sec, (unsigned long)al_back.tv_usec);
    while (alarm_fired == 0U) {
      chThdSleepMilliseconds(10);
    }
    systime_t dt = chVTGetSystemTimeX() - t0;
    uint32_t ms = (uint32_t)(dt * 1000U / CH_CFG_ST_FREQUENCY);
    PRINTF("alarm fired after %lu ms (expect ~2000): %s\r\n",
           (unsigned long)ms,
           (ms >= 1900U && ms <= 2100U) ? "PASS" : "FAIL");
    rtcSetCallback(&RTCD1, NULL);
  }

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
