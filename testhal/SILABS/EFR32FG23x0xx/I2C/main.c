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

int main(void) {
  halInit();
  chSysInit();

  console_init();
  PRINTF("\r\nI2C test: I2C0 bus scan (SCL=PA8 SDA=PA7)\r\n");

  palSetPadMode(GPIOA, 8, PAL_MODE_OUTPUT_OPENDRAIN);  /* SCL */
  palSetPadMode(GPIOA, 7, PAL_MODE_OUTPUT_OPENDRAIN);  /* SDA */
  /* GPIO->I2CROUTE is programmed by the I2C LLD from i2c_cfg. */
  i2cStart(&I2CD1, &i2c_cfg);

  int found = 0;
  uint8_t dummy = 0;
  i2cAcquireBus(&I2CD1);
  for (uint8_t addr = 0; addr < 0x80; addr++) {
    msg_t msg = i2cMasterTransmitTimeout(&I2CD1, addr, &dummy, 1, NULL, 0, TIME_MS2I(10));
    if (msg == MSG_OK) {
      PRINTF("  device at 0x%02X\r\n", addr);
      found++;
    }
  }
  i2cReleaseBus(&I2CD1);
  PRINTF("scan done, %d device(s)\r\n", found);

  /* Si7021 probes at 0x40: pure TX, pure RX, combined TX+RX. After each,
     dump the I2C0 state machine and reset the driver. */
  {
    static const uint8_t idcmd[] = { 0xFC, 0xC9 };
    uint8_t rx[6];
    msg_t msg;

    i2cAcquireBus(&I2CD1);
    msg = i2cMasterTransmitTimeout(&I2CD1, 0x40, idcmd, sizeof(idcmd),
                                   NULL, 0, TIME_MS2I(200));
    i2cReleaseBus(&I2CD1);
    PRINTF("tx-only: msg=%d STATE=%08lX IF=%08lX STATUS=%08lX\r\n", (int)msg,
           (unsigned long)I2C0->STATE, (unsigned long)I2C0->IF,
           (unsigned long)I2C0->STATUS);
    i2cStop(&I2CD1); i2cStart(&I2CD1, &i2c_cfg);

    i2cAcquireBus(&I2CD1);
    msg = i2cMasterReceiveTimeout(&I2CD1, 0x40, rx, sizeof(rx), TIME_MS2I(200));
    i2cReleaseBus(&I2CD1);
    PRINTF("rx-only: msg=%d rx=%02X %02X %02X STATE=%08lX IF=%08lX\r\n",
           (int)msg, rx[0], rx[1], rx[2],
           (unsigned long)I2C0->STATE, (unsigned long)I2C0->IF);
    i2cStop(&I2CD1); i2cStart(&I2CD1, &i2c_cfg);

    i2cAcquireBus(&I2CD1);
    msg = i2cMasterTransmitTimeout(&I2CD1, 0x40, idcmd, sizeof(idcmd),
                                   rx, sizeof(rx), TIME_MS2I(200));
    i2cReleaseBus(&I2CD1);
    PRINTF("tx+rx: msg=%d rx=%02X %02X %02X %02X %02X %02X STATE=%08lX IF=%08lX\r\n",
           (int)msg, rx[0], rx[1], rx[2], rx[3], rx[4], rx[5],
           (unsigned long)I2C0->STATE, (unsigned long)I2C0->IF);
  }
  while (true) chThdSleepMilliseconds(1000);
}
