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

static const PWMConfig pwm_cfg = {
  .frequency = 10000U,          /* 10 kHz PWM */
  .period    = 1000U,           /* 1000 ticks -> 10 Hz frame, 1 tick = 1% */
  .callback  = NULL,
  .channels  = {
    {PWM_OUTPUT_DISABLED, NULL},
    {PWM_OUTPUT_DISABLED, NULL},
    {PWM_OUTPUT_ACTIVE_HIGH, NULL},
  },
};

int main(void) {
  halInit();
  chSysInit();

  palSetPadMode(GPIOB, 2, PAL_MODE_OUTPUT_PUSHPULL); /* TIMER3 CC0 routed below */
  console_init();
  PRINTF("\r\nPWM-TIMER test: TIMER3 CC0 -> PB2 (LED0), duty sweep\r\n");

  /* Route TIMER0 CC0 to PB2. */
  GPIO->TIMERROUTE[3].ROUTEEN = GPIO_TIMER_ROUTEEN_CC0PEN;
  GPIO->TIMERROUTE[3].CC0ROUTE = (0U << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT) |
                                 (2U << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);

  pwmStart(&PWMD4, &pwm_cfg);
  pwmEnableChannel(&PWMD4, 2, 100U);

  uint32_t duty = 100;
  int dir = 1;
  while (true) {
    pwmEnableChannel(&PWMD4, 2, duty);  /* width in ticks (period=1000) */
    PRINTF("duty=%u%%\r\n", (unsigned)(duty / 10));
    duty += dir * 50;
    if (duty > 900) { duty = 900; dir = -1; }
    if (duty < 100) { duty = 100; dir = 1; }
    chThdSleepMilliseconds(200);
  }
}
