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
#include "hal_letimer_lld.h"

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

static volatile uint32_t of_count = 0;

static void gpt_cb(GPTDriver *gptp) {
  (void)gptp;
  of_count++;
  palTogglePad(GPIOB, 2);
}

static const GPTConfig gpt_cfg = {
  .frequency = 1000U,          /* 1 kHz GPT clock; period below in ticks */
  .callback  = gpt_cb,
};

int main(void) {
  halInit();
  chSysInit();

  palSetPadMode(GPIOB, 2, PAL_MODE_OUTPUT_PUSHPULL);
  console_init();
  PRINTF("\r\nGPT-LETIMER test: LETIMER0 as GPT, 1 Hz overflow toggles LED0\r\n");

  gptStart(&GPTD_LETIMER0, &gpt_cfg);
  gptStartContinuous(&GPTD_LETIMER0, 1000U);  /* 1000 ticks @1kHz = 1 s */

  while (true) {
    chThdSleepMilliseconds(5000);
    PRINTF("overflows: %u (expect ~5)\r\n", (unsigned)of_count);
  }
}
