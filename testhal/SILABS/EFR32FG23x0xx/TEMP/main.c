/*
    ChibiOS - Copyright (C) 2026 Xael South
    Apache License 2.0 — see repository root.

    TEMP test: die temperature sensor (EMU->TEMP, Kelvin x 4).
    Reads the temperature 3 times over 3 s; expects -40..125 degC and
    a stable reading (max spread <= 3 degC). Factory calibration is
    baked in, so absolute accuracy is the silicon grade (+/-2 degC class).
*/
#include "ch.h"
#include "hal.h"
#include "rtt_console.h"
#include "hal_temp_lld.h"

int main(void) {
  halInit();
  chSysInit();
  console_init();

  PRINTF("\r\nTEMP test: EMU die temperature sensor\r\n");

  bool ok = true;

  temp_lld_init();
  temp_lld_start();
  bool first_done = ((EMU->STATUS & EMU_STATUS_FIRSTTEMPDONE) != 0U);
  PRINTF("FIRSTTEMPDONE=%d %s\r\n", (int)first_done, first_done ? "PASS" : "FAIL");
  ok = ok && first_done;

  float min_c = 1000.0f, max_c = -1000.0f;
  for (int i = 0; i < 3; i++) {
    chThdSleepMilliseconds(1000);
    uint32_t raw = temp_lld_read_raw();
    float kelvin = temp_lld_read_kelvin();
    float celsius = temp_lld_read_celsius();
    PRINTF("read %d: raw=%lu (%.2f K) = %.2f C\r\n",
           i, (unsigned long)raw, (double)kelvin, (double)celsius);
    if (celsius < min_c) min_c = celsius;
    if (celsius > max_c) max_c = celsius;
  }

  bool range_ok = (min_c >= -40.0f) && (max_c <= 125.0f);
  float spread = max_c - min_c;
  bool stable_ok = (spread <= 3.0f);
  PRINTF("range [-40..125]C: %s, spread %.2f C (<=3): %s\r\n",
         range_ok ? "PASS" : "FAIL", (double)spread, stable_ok ? "PASS" : "FAIL");
  ok = ok && range_ok && stable_ok;

  PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL");

  while (true) { chThdSleepMilliseconds(1000); }
}
