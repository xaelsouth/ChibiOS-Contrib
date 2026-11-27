/*
    ChibiOS - Copyright (C) 2026 Xael South
    Apache License 2.0 — see repository root.

    LESENSE scan-rate, sparse-channel interrupt mask and restart test.
    No sensor or analog measurement is required.
*/
#include "ch.h"
#include "hal.h"
#include "rtt_console.h"
#include "hal_lesense_lld.h"

static volatile uint32_t scandone_cnt;
static volatile uint32_t decoder_cnt;

static void lesense_cb(uint32_t scanres, void *arg) {
  (void)scanres;
  (void)arg;
  scandone_cnt++;
}

static void decoder_cb(void *arg) {
  (void)arg;
  decoder_cnt++;
}

static const LESENSEConfig lesense_cfg = {
  .scanmode     = EFR32_LESENSE_SCAN_PERIODIC,
  .timctrl      = 255U << _LESENSE_TIMCTRL_PCTOP_SHIFT,
  .num_channels = 3,
  .channels     = {
    { .port = 0xFFU },
    { .port = 0xFFU },
    { .port = 0, .pin = 0 },
  },
  .dac_port     = 0xFFU,
  .dac_pin      = 0xFFU,
  .callback     = lesense_cb,
  .decoder_callback = decoder_cb,
};

static bool check_rate(uint32_t seconds, uint32_t min_count, uint32_t max_count) {
  uint32_t start = scandone_cnt;
  chThdSleepMilliseconds(seconds * 1000U);
  uint32_t count = scandone_cnt - start;
  bool ok = count >= min_count && count <= max_count;
  PRINTF("%lu scans in %lu s (expected %lu..%lu): %s\r\n",
         (unsigned long)count, (unsigned long)seconds,
         (unsigned long)min_count, (unsigned long)max_count,
         ok ? "PASS" : "FAIL");
  return ok;
}

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nLESENSE test\r\n");

  lesense_lld_start(&LESENSED1, &lesense_cfg);
  LESENSE->IF_SET = LESENSE_IF_DEC;
  chThdSleepMilliseconds(5);
  bool ok = decoder_cnt == 1U;
  lesense_lld_set_decoder_irq(&LESENSED1, false);
  LESENSE->IF_SET = LESENSE_IF_DEC;
  chThdSleepMilliseconds(5);
  ok = (decoder_cnt == 1U) && ok;
  lesense_lld_set_decoder_irq(&LESENSED1, true);
  chThdSleepMilliseconds(5);
  ok = (decoder_cnt == 2U) && ok;
  lesense_lld_set_decoder_irq(&LESENSED1, false);
  PRINTF("decoder callback and mask: %s\r\n", ok ? "PASS" : "FAIL");

  lesense_lld_set_hit_irq(&LESENSED1, true);
  bool sparse_ok = (LESENSE->IEN & 0xFFFFU) == (1U << 2);
  PRINTF("sparse channel IRQ mask: %s\r\n", sparse_ok ? "PASS" : "FAIL");
  ok = sparse_ok && ok;
  lesense_lld_set_hit_irq(&LESENSED1, false);

  lesense_lld_start_scan(&LESENSED1);
  ok = check_rate(2, 250, 262) && ok; /* 32768 / 256 = 128 Hz. */
  lesense_lld_set_scan_hz(&LESENSED1, 100.0f);
  ok = check_rate(2, 197, 205) && ok;
  lesense_lld_stop_scan(&LESENSED1);
  lesense_lld_stop(&LESENSED1);

  lesense_lld_start(&LESENSED1, &lesense_cfg);
  lesense_lld_set_scan_hz(&LESENSED1, 200.0f);
  lesense_lld_start_scan(&LESENSED1);
  ok = check_rate(2, 397, 406) && ok;
  lesense_lld_stop_scan(&LESENSED1);
  lesense_lld_stop(&LESENSED1);
  PRINTF("LESENSE: %s\r\n", ok ? "ALL PASS" : "FAIL");

  while (true) { chThdSleepMilliseconds(1000); }
}
