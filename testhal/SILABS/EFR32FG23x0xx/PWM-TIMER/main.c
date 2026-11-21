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

  palSetPadMode(GPIOB, 2, PAL_MODE_OUTPUT_PUSHPULL); /* TIMER0 CC0 routed below */
  console_init();
  PRINTF("\r\nPWM-TIMER test: TIMER0 CC0 -> PB2 (LED0), duty sweep\r\n");

  /* Route TIMER0 CC0 to PB2. */
  GPIO->TIMERROUTE[0].ROUTEEN = GPIO_TIMER_ROUTEEN_CC0PEN;
  GPIO->TIMERROUTE[0].CC0ROUTE = (0U << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT) |
                                 (2U << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);

  pwmStart(&PWMD1, &pwm_cfg);
  pwmEnableChannel(&PWMD1, 2, 100U);

  uint32_t duty = 100;
  int dir = 1;
  while (true) {
    pwmEnableChannel(&PWMD1, 2, duty);  /* width in ticks (period=1000) */
    PRINTF("duty=%u%%\r\n", (unsigned)(duty / 10));
    duty += dir * 50;
    if (duty > 900) { duty = 900; dir = -1; }
    if (duty < 100) { duty = 100; dir = 1; }
    chThdSleepMilliseconds(200);
  }
}
