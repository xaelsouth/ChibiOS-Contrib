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
#include "rt_test_root.h"
#include "oslib_test_root.h"

/*
 * Board resources (BRD2600A REV A03):
 *   LED0 = PB2, BTN0 = PB1, BTN1 = PA5 (escape hatch, see board.h).
 *   J-Link VCOM: EUSART0 (SIOD1) on PA9=TX, PA10=RX.
 */
#define LED0_PORT               GPIOB
#define LED0_PAD                2U
#define BTN0_PORT               GPIOB
#define BTN0_PAD                1U
#define BTN1_PORT               ESCAPE_HATCH_PORT
#define BTN1_PAD                ESCAPE_HATCH_PIN

/*
 * Serial port (J-Link VCOM) configuration: SIO_DEFAULT_BITRATE (38400
 * unless overridden in halconf.h), 8N1 on PA9/PA10.
 */
static const SIOConfig vcom_sio_cfg = {
  .baud     = SIO_DEFAULT_BITRATE,
  .cfg0     = (0U << 0) | (0U << 5),  /* ASYNC, 16x oversampling */
  .framecfg = EFR32_SIO_LLD_EUSART_8N1,
};

/*
 * LED0 blinker thread: 1 s on, 1 s off.
 */
static THD_WORKING_AREA(waBlinker, 128);
static THD_FUNCTION(blinker, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    palSetPad(LED0_PORT, LED0_PAD);
    chThdSleepMilliseconds(1000);
    palClearPad(LED0_PORT, LED0_PAD);
    chThdSleepMilliseconds(1000);
  }
}

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * LED0, BTN0 and BTN1 pad setup.
   */
  palSetPadMode(LED0_PORT, LED0_PAD, PAL_MODE_OUTPUT_PUSHPULL);
  palClearPad(LED0_PORT, LED0_PAD);
  palSetPadMode(BTN0_PORT, BTN0_PAD, PAL_MODE_INPUT_PULLUP);
  palSetPadMode(BTN1_PORT, BTN1_PAD, PAL_MODE_INPUT_PULLUP);

  /*
   * Activates the serial driver SIOD1 (EUSART0 on the J-Link VCOM pins
   * PA9=TX, PA10=RX) at SIO_DEFAULT_BITRATE baud, 8N1.
   */
  palSetPadMode(GPIOA, 9,  PAL_MODE_OUTPUT_PUSHPULL | PAL_MODE_ALTERNATE(EUSART0_TX));
  palSetPadMode(GPIOA, 10, PAL_MODE_INPUT_PULLUP  | PAL_MODE_ALTERNATE(EUSART0_RX));
  sioStart(&SIOD1, &vcom_sio_cfg);

  chprintf((BaseSequentialStream *)&SIOD1,
           "\r\n*** ChibiOS/RT demo on BRD2600A REV A03 (EFR32FG23B010F512IM48) ***\r\n");
  chprintf((BaseSequentialStream *)&SIOD1,
           "Serial: SIOD1 (EUSART0) PA9/PA10 @ %u baud 8N1 (J-Link VCOM)\r\n",
           (unsigned)SIO_DEFAULT_BITRATE);
  chprintf((BaseSequentialStream *)&SIOD1,
           "Press BTN0 (PB1) to run the ChibiOS RT/oslib test suites,\r\n"
           "press BTN1 (PA5) for a status line.\r\n\r\n");

  /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, blinker, NULL);

  /*
   * Normal main() thread activity: when BTN0 is pressed the ChibiOS
   * test suites are executed on the serial port, when BTN1 is pressed
   * a status line is printed, otherwise the LED keeps blinking
   * (1 s on / 1 s off).
   */
  while (true) {
    if (!palReadPad(BTN0_PORT, BTN0_PAD)) {
      chprintf((BaseSequentialStream *)&SIOD1, "Running RT test suite...\r\n");
      test_execute((BaseSequentialStream *)&SIOD1, &rt_test_suite);
      chprintf((BaseSequentialStream *)&SIOD1, "Running OSLIB test suite...\r\n");
      test_execute((BaseSequentialStream *)&SIOD1, &oslib_test_suite);
      chprintf((BaseSequentialStream *)&SIOD1, "Tests done. Press BTN0 to run again.\r\n\r\n");
      /* Wait for the button release to avoid retriggering. */
      while (!palReadPad(BTN0_PORT, BTN0_PAD)) {
        chThdSleepMilliseconds(100);
      }
    }
    else if (!palReadPad(BTN1_PORT, BTN1_PAD)) {
      chprintf((BaseSequentialStream *)&SIOD1,
               "BTN1: %s, uptime %u s\r\n",
               BOARD_NAME,
               (unsigned)(chVTGetSystemTimeX() / CH_CFG_ST_FREQUENCY));
      /* Wait for the button release to avoid retriggering. */
      while (!palReadPad(BTN1_PORT, BTN1_PAD)) {
        chThdSleepMilliseconds(100);
      }
    }
    chThdSleepMilliseconds(200);
  }
}
