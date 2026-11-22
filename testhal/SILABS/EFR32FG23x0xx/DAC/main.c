/*
    ChibiOS - Copyright (C) 2026 Xael South
    Apache License 2.0 — see repository root.

    DAC test: VDAC0 channels 1/2 (DACD1/DACD2), single-ended continuous
    mode. Verifies channel enable/warm status and sample writes.
    No analog measurement — checks the digital path only.
*/
#include "ch.h"
#include "hal.h"
#include "rtt_console.h"
#include "hal_dac.h"

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nDAC test: VDAC0 CH0/CH1 single-ended\r\n");

  dacInit();

  static const DACConfig cfg = { };
  msg_t r1 = dacStart(&DACD1, &cfg);
  msg_t r2 = dacStart(&DACD2, &cfg);
  PRINTF("dacStart CH0=%d CH1=%d %s\r\n", (int)r1, (int)r2,
         (r1 == HAL_RET_SUCCESS && r2 == HAL_RET_SUCCESS) ? "PASS" : "FAIL");

  chThdSleepMilliseconds(50);   /* warmup */

  uint32_t st = VDAC0->STATUS;
  PRINTF("STATUS=0x%08lX (CH0ENS|CH0WARM|CH1ENS|CH1WARM=0x33)\r\n",
         (unsigned long)st);
  bool ok = (r1 == HAL_RET_SUCCESS) && (r2 == HAL_RET_SUCCESS) &&
            ((st & 0x33U) == 0x33U);
  PRINTF("channels enabled+warm: %s\r\n", ok ? "PASS" : "FAIL");

  msg_t p1 = dacPutChannelX(&DACD1, 0, 0x0000U);
  msg_t p2 = dacPutChannelX(&DACD1, 0, 0x0FFFU);
  msg_t p3 = dacPutChannelX(&DACD2, 1, 0x0800U);
  bool ok2 = (p1 == HAL_RET_SUCCESS) && (p2 == HAL_RET_SUCCESS) &&
             (p3 == HAL_RET_SUCCESS);
  PRINTF("put 0x0000/0x0FFF (CH0), 0x0800 (CH1): %s\r\n",
         ok2 ? "PASS" : "FAIL");
  ok = ok && ok2;

  dacStop(&DACD1);
  dacStop(&DACD2);
  /* After both channels stop, the LLD gates the VDAC bus clock. */
  uint32_t clk = CMU->CLKEN1 & CMU_CLKEN1_VDAC0;
  bool ok3 = (clk == 0U);
  PRINTF("after stop CLKEN1.VDAC0=0x%lX (expect 0): %s\r\n",
         (unsigned long)clk, ok3 ? "PASS" : "FAIL");
  ok = ok && ok3;

  PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL");

  while (true) { chThdSleepMilliseconds(1000); }
}
