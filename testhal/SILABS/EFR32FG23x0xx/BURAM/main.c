/*
    ChibiOS - Copyright (C) 2026 Xael South
    Apache License 2.0 — see repository root.

    BURAM test: backup RAM write/read via the HAL BURAM driver.
    BURAM1 is 32 bytes of battery-backed RAM; allocate a block,
    write a pattern, read it back and verify.
*/
#include "ch.h"
#include "hal.h"
#include "rtt_console.h"
#include "hal_buram.h"

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nBURAM test: BURAMD1 (32 bytes backup RAM)\r\n");

  buramInit();

  static const BURAMConfig cfg = { };
  buramStart(&BURAMD1, &cfg);
  PRINTF("state=%d start=0x%08lX end=0x%08lX\r\n",
         (int)BURAMD1.state, (unsigned long)BURAMD1.start,
         (unsigned long)BURAMD1.end);

  volatile uint8_t *p = (volatile uint8_t *)buramAllocateAt(&BURAMD1, 0, 16);
  bool ok = (p != NULL);
  PRINTF("allocate 16 bytes at offset 0: %s (0x%08lX)\r\n",
         ok ? "OK" : "FAIL", (unsigned long)(uintptr_t)p);

  if (ok) {
    for (int i = 0; i < 16; i++) {
      p[i] = (uint8_t)(0xA0U + i);
    }
    for (int i = 0; i < 16; i++) {
      if (p[i] != (uint8_t)(0xA0U + i)) { ok = false; }
    }
    PRINTF("write/read pattern 0xA0..0xAF: %s\r\n", ok ? "PASS" : "FAIL");

    /* second allocation must not overlap the first */
    volatile uint8_t *q = (volatile uint8_t *)buramAllocateAt(&BURAMD1, 16, 16);
    bool ok2 = (q != NULL) && ((uintptr_t)q == (uintptr_t)p + 16);
    PRINTF("allocate 16 bytes at offset 16: %s (0x%08lX)\r\n",
           ok2 ? "OK" : "FAIL", (unsigned long)(uintptr_t)q);
    ok = ok && ok2;

    /* Note: an overlapping buramAllocateAt() triggers osalDbgAssert in
       debug builds, so it is intentionally not tested here. */
  }

  /* raw register access smoke test: BURAM is plain memory */
  volatile uint32_t *w = (volatile uint32_t *)BURAM1_ADDRESS;
  w[0] = 0xDEADBEEFUL;
  bool ok4 = (w[0] == 0xDEADBEEFUL);
  PRINTF("raw word write/read 0xDEADBEEF: %s\r\n", ok4 ? "PASS" : "FAIL");
  ok = ok && ok4;

  buramStop(&BURAMD1);

  PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL");

  while (true) { chThdSleepMilliseconds(1000); }
}
