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

/**
 * @file    main.c
 * @brief   SYSTICKv2 (SYSRTC0-based 32-bit) system tick test.
 *
 * @details Tests the optional SYSTICKv2 driver (built with
 *          EFR32_USE_SYSTICKV2=yes, see Makefile):
 *          - measures the system time delta of chThdSleepMilliseconds()
 *            against OSAL_ST_FREQUENCY (tick rate + alarm accuracy)
 *          - blinks the LED and prints "tick" every second
 *
 *          Default build is FREERUNNING (CH_CFG_ST_TIMEDELTA != 0);
 *          "make ST_PERIODIC=yes" builds OSAL_ST_MODE_PERIODIC.
 *
 *          Console via SEGGER RTT (up channel 0), see runtest.sh.
 */

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

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

/**
 * @brief   Sleeps @p ms milliseconds and returns the measured system time
 *          delta in ticks.
 */
static systime_t measure_sleep(uint32_t ms) {
  systime_t t0, t1;

  t0 = osalOsGetSystemTimeX();
  chThdSleepMilliseconds(ms);
  t1 = osalOsGetSystemTimeX();

  return t1 - t0;
}

/**
 * @brief   Checks that a sleep of @p ms lasted about @p ms.
 * @return  true if within 1% tolerance.
 */
static bool check_sleep(uint32_t ms) {
  systime_t delta = measure_sleep(ms);
  systime_t expected = (systime_t)(((uint64_t)ms * OSAL_ST_FREQUENCY) / 1000U);
  systime_t tol = expected / 100U;
  bool ok = (delta >= expected - tol) && (delta <= expected + tol + 1U);

  PRINTF("sleep %4u ms -> %6u ticks (expected %6u +/- %u): %s\r\n",
         (unsigned)ms, (unsigned)delta, (unsigned)expected,
         (unsigned)tol, ok ? "PASS" : "FAIL");
  return ok;
}

int main(void) {
  bool all_ok = true;

  halInit();
  chSysInit();

  palSetPadMode(GPIOB, 2, PAL_MODE_OUTPUT_PUSHPULL);
  console_init();

#if OSAL_ST_MODE == OSAL_ST_MODE_FREERUNNING
  PRINTF("\r\nSYSTICKv2 test: EFR32FG23 DK2600A, SYSRTC0, FREERUNNING mode\r\n");
#elif OSAL_ST_MODE == OSAL_ST_MODE_PERIODIC
  PRINTF("\r\nSYSTICKv2 test: EFR32FG23 DK2600A, core SysTick, PERIODIC mode\r\n");
#else
  PRINTF("\r\nSYSTICKv2 test: EFR32FG23 DK2600A, core SysTick, ST disabled?!\r\n");
#endif
  PRINTF("OSAL_ST_FREQUENCY=%u Hz, OSAL_ST_RESOLUTION=%u\r\n",
         (unsigned)OSAL_ST_FREQUENCY, (unsigned)OSAL_ST_RESOLUTION);
  PRINTF("system time at start: %u ticks\r\n", (unsigned)osalOsGetSystemTimeX());

  /* Tick rate + alarm accuracy at several sleep lengths. */
  all_ok &= check_sleep(10U);
  all_ok &= check_sleep(100U);
  all_ok &= check_sleep(1000U);

  PRINTF("SYSTICKv2: %s\r\n", all_ok ? "ALL PASS" : "FAIL");

  while (true) {
    palTogglePad(GPIOB, 2);
    PRINTF("tick\r\n");
    chThdSleepMilliseconds(1000);
  }
}
