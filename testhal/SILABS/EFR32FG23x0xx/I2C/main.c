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
  .duty_cycle  = 0,
  .sda_port    = 0,   /* PA7 */
  .sda_pin     = 7,
  .scl_port    = 0,   /* PA8 */
  .scl_pin     = 8,
};

static uint8_t si7021_crc(const uint8_t *data, size_t size) {
  uint8_t crc = 0U;
  for (size_t i = 0; i < size; i++) {
    crc ^= data[i];
    for (unsigned bit = 0; bit < 8U; bit++) {
      crc = (crc & 0x80U) != 0U ? (uint8_t)((crc << 1) ^ 0x31U)
                                 : (uint8_t)(crc << 1);
    }
  }
  return crc;
}

int main(void) {
  halInit();
  chSysInit();

  console_init();
  PRINTF("\r\nI2C test: Si7021 at 0x40 (SCL=PA8 SDA=PA7)\r\n");

  palSetPad(GPIOA, 8);
  palSetPad(GPIOA, 7);
  palSetPadMode(GPIOA, 8, PAL_MODE_OUTPUT_OPENDRAIN);  /* SCL */
  palSetPadMode(GPIOA, 7, PAL_MODE_OUTPUT_OPENDRAIN);  /* SDA */
  /* GPIO->I2CROUTE is programmed by the I2C LLD from i2c_cfg. */
  i2cStart(&I2CD1, &i2c_cfg);

  const uint8_t read_user = 0xE7U;
  const uint8_t measure_temp = 0xF3U;
  uint8_t user_reg = 0U;
  uint8_t sample[3] = {0U};
  i2cAcquireBus(&I2CD1);
  msg_t combined = i2cMasterTransmitTimeout(&I2CD1, 0x40, &read_user, 1U,
                                            &user_reg, 1U, TIME_MS2I(200));
  msg_t tx = i2cMasterTransmitTimeout(&I2CD1, 0x40, &measure_temp, 1U,
                                       NULL, 0U, TIME_MS2I(200));
  chThdSleepMilliseconds(50);
  msg_t rx = tx == MSG_OK
           ? i2cMasterReceiveTimeout(&I2CD1, 0x40, sample, sizeof(sample),
                                     TIME_MS2I(200))
           : MSG_RESET;
  i2cReleaseBus(&I2CD1);
  bool crc_ok = rx == MSG_OK && si7021_crc(sample, 2U) == sample[2];
  PRINTF("combined=%d user=%02X tx=%d rx=%d sample=%02X%02X crc=%02X %s\r\n",
         (int)combined, user_reg, (int)tx, (int)rx,
         sample[0], sample[1], sample[2],
         combined == MSG_OK && tx == MSG_OK && crc_ok ? "PASS" : "FAIL");
  while (true) chThdSleepMilliseconds(1000);
}
