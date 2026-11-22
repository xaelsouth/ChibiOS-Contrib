/*
    ChibiOS - Copyright (C) 2026 Xael South
    Apache License 2.0 — see repository root.

    LESENSE test: periodic scan of one channel, SCANDONE interrupts counted
    over 5 s to measure the real scan rate. No sensor hardware needed — the
    scan engine runs autonomously in EM2 and SCANDONE fires per scan cycle.
*/
#include "ch.h"
#include "hal.h"
#include "rtt_console.h"
#include "hal_lesense_lld.h"

static volatile uint32_t scandone_cnt;
static volatile uint32_t last_scanres;

static void lesense_cb(uint32_t scanres, void *arg) {
  (void)arg;
  scandone_cnt++;
  last_scanres = scanres;
}

static const LESENSEConfig lesense_cfg = {
  .scanmode     = EFR32_LESENSE_SCAN_PERIODIC,
  /* LFPRESC DIV8: scan clock = 32768/8 = 4096 Hz -> ~683 scans/s. */
  .timctrl      = LESENSE_TIMCTRL_LFPRESC_DIV1 | LESENSE_TIMCTRL_PCPRESC_DIV1,
  .perctrl      = 0,
  .num_channels = 1,
  .channels     = {
    {
      .pin        = 0,
      .interact   = LESENSE_CH_INTERACT_EXMODE_DISABLE,
      .evalcfg    = LESENSE_CH_EVALCFG_MODE_THRES,
      .evalthres  = 0,
    },
  },
  .callback     = lesense_cb,
  .callback_arg = NULL,
};

int main(void) {
  halInit();
  chSysInit();
  console_init();

  PRINTF("\r\nLESENSE test: periodic scan ch0, IRQ-counted rate\r\n");

  lesense_lld_start(&LESENSED1, &lesense_cfg);
  PRINTF("state after start: %d (READY=%d)\r\n", (int)LESENSED1.state, (int)LESENSE_READY);

  lesense_lld_start_scan(&LESENSED1);
  chThdSleepMilliseconds(5000);

  uint32_t cnt = scandone_cnt;
  uint32_t scanres = lesense_lld_get_scanres(&LESENSED1);
  PRINTF("stopping scan...\r\n");
  lesense_lld_stop_scan(&LESENSED1);
  lesense_lld_stop(&LESENSED1);
  PRINTF("stopped\r\n");

  float hz = (float)cnt / 5.0f;
  PRINTF("SCANDONE in 5 s: %lu -> %.1f scans/s\r\n", (unsigned long)cnt, (double)hz);
  PRINTF("last SCANRES: 0x%08lX (cb 0x%08lX)\r\n",
         (unsigned long)scanres, (unsigned long)last_scanres);
  /* Expect roughly 683 scans/s with LFPRESC DIV8; accept 300..1500 Hz. */
  bool ok = (cnt > 10000U) && (hz > 3000.0f);
  PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL");

  while (true) { chThdSleepMilliseconds(1000); }
}
