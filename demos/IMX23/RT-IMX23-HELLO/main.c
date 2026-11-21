/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

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

#include "board.h"

/*
 * Console on the PL011 DUART (pins configured in board.c).
 */
static const SIOConfig sio_console = {
  .baud     = 115200U,
  .framecfg = IMX23_SIO_LLD_8N1,
};

/*
 * Application entry point.
 */
int main(void) {
  static const uint8_t hello[] = "\r\nChibiOS/RT on i.MX23 (ARM926EJ-S) ready\r\n";

  halInit();
  chSysInit();

  /* Console. */
  sioStart(&SIOD1, &sio_console);
  sioAsyncWrite(&SIOD1, hello, sizeof(hello) - 1U);

  /* LED. */
  palSetPadMode(BOARD_LED_PORT, BOARD_LED_PAD, PAL_MODE_OUTPUT_PUSHPULL);
  palClearPad(BOARD_LED_PORT, BOARD_LED_PAD);

  while (true) {
    palTogglePad(BOARD_LED_PORT, BOARD_LED_PAD);
    chThdSleepMilliseconds(500);
  }

  return 0;
}
