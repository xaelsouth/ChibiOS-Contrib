/*
    ChibiOS - Copyright (C) 2024 Xael South

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

/*
 * Standard RT demo for the BRD4001A REV A01 board (EFR32FG23A010F256GM48).
 *
 *   - Serial console on SIOD4 (USART0): PC2=TX, PC1=RX, SIO_DEFAULT_BITRATE
 *     (38400 unless overridden in cfg/halconf.h), 8N1. Typed characters are
 *     echoed back.
 *   - The BURTC-based RTC time is printed on the console once per second.
 */

#include <string.h>
#include "cmsis_os.h"
#include "hal.h"
#include "chprintf.h"

/*
 * Serial console (USART0 = SIOD4) configuration: PC2=TX, PC1=RX.
 */
static const SIOConfig console_sio_cfg = {
  .baud     = SIO_DEFAULT_BITRATE,
  .cfg0     = (0U << 0) | (0U << 5),  /* ASYNC, 16x oversampling */
  .framecfg = EFR32_SIO_LLD_USART_8N1,
};

/*
 * Application entry point.
 */
int main(void) {

  /* HAL initialization, this also initializes the configured device drivers
     and performs the board-specific initializations.*/
  halInit();

  /* The kernel is initialized but not started yet, this means that
     main() is executing with absolute priority but interrupts are
     already enabled.*/
  osKernelInitialize();

  /* Console pads: PC2=USART0_TX, PC1=USART0_RX. */
  palSetPadMode(GPIOC, 2, PAL_MODE_OUTPUT_PUSHPULL | PAL_MODE_ALTERNATE(USART0_TX));
  palSetPadMode(GPIOC, 1, PAL_MODE_INPUT_PULLUP  | PAL_MODE_ALTERNATE(USART0_RX));
  sioStart(&SIOD4, &console_sio_cfg);

  /* Kernel started, the main() thread has priority osPriorityNormal
     by default.*/
  osKernelStart();

  chprintf((BaseSequentialStream *)&SIOD4,
           "\r\n*** ChibiOS/RT demo on BRD4001A REV A01 (EFR32FG23A010F256GM48) ***\r\n");
  chprintf((BaseSequentialStream *)&SIOD4,
           "Serial: SIOD4 (USART0) PC2=TX/PC1=RX @ %u baud 8N1, echo on\r\n",
           (unsigned)SIO_DEFAULT_BITRATE);
  chprintf((BaseSequentialStream *)&SIOD4,
           "RTC (BURTC) time is printed once per second.\r\n\r\n");

  /*
   * Normal main() thread activity: print the RTC time every second and
   * echo back any characters received on the console.
   */
  RTCDateTime ts;
  struct tm tim;
  uint32_t tv_msec;
  while (true) {
    osDelay(1000);

    rtcGetTime(&RTCD1, &ts);
    rtcConvertDateTimeToStructTm(&ts, &tim, &tv_msec);
    chprintf((BaseSequentialStream *)&SIOD4,
             "%04u-%02u-%02u %02u:%02u:%02u.%03u dow=%d\r\n",
             (unsigned)(tim.tm_year + 1900), (unsigned)(tim.tm_mon + 1),
             (unsigned)tim.tm_mday, (unsigned)tim.tm_hour,
             (unsigned)tim.tm_min, (unsigned)tim.tm_sec,
             (unsigned)tv_msec, (int)tim.tm_wday);

    msg_t msg;
    while ((msg = sioGetX(&SIOD4)) != MSG_TIMEOUT) {
      sioPutX(&SIOD4, msg);
    }
  }
}
