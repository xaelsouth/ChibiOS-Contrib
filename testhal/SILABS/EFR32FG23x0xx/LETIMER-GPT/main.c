/*
    ChibiOS - Copyright (C) 2026 Xael South
    Apache License 2.0 — see repository root.

    LETIMER-GPT test: LETIMER0 used as a generic GPT timer (system tick runs
    on SYSTICKv2/SYSRTC0 in this build). 1 Hz overflow callback toggles LED0.
*/
#include "ch.h"
#include "hal.h"
#include "rtt_console.h"

extern GPTDriver GPTD_LETIMER0;

static volatile uint32_t of_count;

static void gpt_cb(GPTDriver *gptp) {
  (void)gptp;
  of_count++;
  palTogglePad(GPIOB, 2);
}

static const GPTConfig gpt_cfg = {
  .frequency = 32768U,          /* LETIMER0 clocked from LFXO = 32768 Hz */
  .callback  = gpt_cb,
};

int main(void) {
  halInit();
  chSysInit();

  palSetPadMode(GPIOB, 2, PAL_MODE_OUTPUT_PUSHPULL);
  console_init();
  PRINTF("\r\nLETIMER-GPT test: LETIMER0 as GPT, 1 Hz overflow toggles LED0\r\n");

  gptStart(&GPTD_LETIMER0, &gpt_cfg);
  gptStartContinuous(&GPTD_LETIMER0, 32768U);   /* 1 s period */

  uint32_t last = 0;
  bool ok = false;
  while (true) {
    chThdSleepMilliseconds(5000);
    uint32_t now = of_count;
    ok = (now - last) >= 4U && (now - last) <= 6U;
    PRINTF("overflows: %u (+%u in 5 s = %.1f Hz) %s\r\n",
           (unsigned)now, (unsigned)(now - last),
           (float)(now - last) / 5.0f, ok ? "PASS" : "FAIL");
    last = now;
    if (now > 10U) { PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL"); }
  }
}
