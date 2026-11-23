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

  /* Quadratic correction curve: T' = a*T^2 + b*T + c on top of the
     factory-calibrated reading. First a pure -2.0 C offset, then a
     quadratic term; then clear and confirm we are back at raw. */
  float raw_c = temp_lld_read_celsius();

  const temp_corr_t offs_corr = { 0.0f, 1.0f, -2.0f };
  temp_lld_set_correction(&offs_corr);
  float c_offs = temp_lld_read_celsius();
  bool offs_ok = (c_offs >= raw_c - 2.6f) && (c_offs <= raw_c - 1.4f);
  PRINTF("correction offs -2.0: raw %.2f -> %.2f (expect ~%.2f) %s\r\n",
         (double)raw_c, (double)c_offs, (double)(raw_c - 2.0f),
         offs_ok ? "PASS" : "FAIL");
  ok = ok && offs_ok;

  const temp_corr_t quad_corr = { 0.01f, 1.0f, 0.0f };  /* +0.01*T^2 ~ +9 C at 30 C */
  temp_lld_set_correction(&quad_corr);
  float c_quad = temp_lld_read_celsius();
  float expect_quad = 0.01f * raw_c * raw_c + raw_c;
  bool quad_ok = (c_quad >= expect_quad - 1.0f) && (c_quad <= expect_quad + 1.0f);
  PRINTF("correction quad 0.01: %.2f (expect ~%.2f) %s\r\n",
         (double)c_quad, (double)expect_quad, quad_ok ? "PASS" : "FAIL");
  ok = ok && quad_ok;

  temp_lld_clear_correction();
  float c_clear = temp_lld_read_celsius();
  bool clear_ok = (c_clear >= raw_c - 1.0f) && (c_clear <= raw_c + 1.0f);
  PRINTF("correction cleared: %.2f (back to raw ~%.2f) %s\r\n",
         (double)c_clear, (double)raw_c, clear_ok ? "PASS" : "FAIL");
  ok = ok && clear_ok;

  PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL");

  while (true) { chThdSleepMilliseconds(1000); }
}
