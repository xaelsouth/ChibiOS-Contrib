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

  palSetPadMode(GPIOB, 2, PAL_MODE_OUTPUT_PUSHPULL);   /* LED0 */
  palSetPadMode(GPIOB, 1, PAL_MODE_INPUT_PULLUP);       /* BTN0 */
  palSetPadMode(GPIOA, 5, PAL_MODE_INPUT_PULLUP);       /* BTN1 */
  console_init();
  PRINTF("\r\nGPIO test: LED0=PB2 BTN0=PB1 BTN1=PA5\r\n");

  bool btn0_old = true, btn1_old = true;
  while (true) {
    palTogglePad(GPIOB, 2);
    bool b0 = palReadPad(GPIOB, 1) == PAL_LOW;
    bool b1 = palReadPad(GPIOA, 5) == PAL_LOW;
    if (b0 != btn0_old) { PRINTF("BTN0 %s\r\n", b0 ? "pressed" : "released"); btn0_old = b0; }
    if (b1 != btn1_old) { PRINTF("BTN1 %s\r\n", b1 ? "pressed" : "released"); btn1_old = b1; }
    chThdSleepMilliseconds(200);
  }
}
