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

static const I2CConfig i2c_cfg = {
  .clock_speed = 400000U,
  .duty_cycle = 0,
};

int main(void) {
  halInit();
  chSysInit();

  console_init();
  PRINTF("\r\nI2C test: I2C0 bus scan (SCL=PA8 SDA=PA7)\r\n");

  palSetPadMode(GPIOA, 8, PAL_MODE_OUTPUT_OPENDRAIN);  /* SCL */
  palSetPadMode(GPIOA, 7, PAL_MODE_OUTPUT_OPENDRAIN);  /* SDA */
  GPIO->I2CROUTE[0].ROUTEEN = GPIO_I2C_ROUTEEN_SDAPEN | GPIO_I2C_ROUTEEN_SCLPEN;
  GPIO->I2CROUTE[0].SDAROUTE = (0U << _GPIO_I2C_SDAROUTE_PORT_SHIFT) | (7U << _GPIO_I2C_SDAROUTE_PIN_SHIFT);
  GPIO->I2CROUTE[0].SCLROUTE = (0U << _GPIO_I2C_SCLROUTE_PORT_SHIFT) | (8U << _GPIO_I2C_SCLROUTE_PIN_SHIFT);

  i2cStart(&I2CD1, &i2c_cfg);

  int found = 0;
  uint8_t dummy = 0;
  for (uint8_t addr = 0; addr < 0x80; addr++) {
    msg_t msg = i2cMasterTransmitTimeout(&I2CD1, addr, &dummy, 1, NULL, 0, TIME_MS2I(10));
    if (msg == MSG_OK) {
      PRINTF("  device at 0x%02X\r\n", addr);
      found++;
    }
  }
  PRINTF("scan done, %d device(s)\r\n", found);
  while (true) chThdSleepMilliseconds(1000);
}
