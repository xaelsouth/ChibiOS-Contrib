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

static const I2CConfig i2c_cfg = {
  .clock_speed = 400000U,
  .duty_cycle = 0,
};

int main(void) {
  halInit();
  chSysInit();

  palSetPadMode(GPIOA, 8, PAL_MODE_OUTPUT_OPENDRAIN);  /* SCL */
  palSetPadMode(GPIOA, 7, PAL_MODE_OUTPUT_OPENDRAIN);  /* SDA */
  GPIO->I2CROUTE[0].ROUTEEN = GPIO_I2C_ROUTEEN_SDAPEN | GPIO_I2C_ROUTEEN_SCLPEN;
  GPIO->I2CROUTE[0].SDAROUTE = (0U << _GPIO_I2C_SDAROUTE_PORT_SHIFT) | (7U << _GPIO_I2C_SDAROUTE_PIN_SHIFT);
  GPIO->I2CROUTE[0].SCLROUTE = (0U << _GPIO_I2C_SCLROUTE_PORT_SHIFT) | (8U << _GPIO_I2C_SCLROUTE_PIN_SHIFT);

  console_init();
  PRINTF("\r\nI2C test: I2C0 bus scan (SCL=PA8 SDA=PA7)\r\n");

  i2cStart(&I2CD1, &i2c_cfg);

  int found = 0;
  for (uint8_t addr = 0; addr < 0x80; addr++) {
    msg_t msg = i2cMasterTransmitTimeout(&I2CD1, addr, NULL, 0, NULL, 0, TIME_MS2I(10));
    if (msg == MSG_OK) {
      PRINTF("  device at 0x%02X\r\n", addr);
      found++;
    }
  }
  PRINTF("scan done, %d device(s)\r\n", found);
  while (true) chThdSleepMilliseconds(1000);
}
