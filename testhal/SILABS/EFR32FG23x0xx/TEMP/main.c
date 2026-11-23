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

  /* Correction polynomial of any degree, applied to the measured value in
     Horner schema: T' = ((c[n-1]*T + c[n-2])*T + ...) + c0. No coefficients
     provided -> measured value unchanged. Test constant, quadratic and
     cubic curves (with identity linear term c1=1), then clear. */
  float raw_c = temp_lld_read_celsius();

  const float offs_corr[3] = { -2.0f, 1.0f, 0.0f };       /* T' = T - 2 */
  temp_lld_set_correction(offs_corr, 3U);
  float c_offs = temp_lld_read_celsius();
  bool offs_ok = (c_offs >= raw_c - 2.6f) && (c_offs <= raw_c - 1.4f);
  PRINTF("corr { -2, 1, 0 }: raw %.2f -> %.2f (expect ~%.2f) %s\r\n",
         (double)raw_c, (double)c_offs, (double)(raw_c - 2.0f),
         offs_ok ? "PASS" : "FAIL");
  ok = ok && offs_ok;

  const float quad_corr[3] = { 0.0f, 1.0f, 0.01f };       /* T' = T + 0.01*T^2 */
  temp_lld_set_correction(quad_corr, 3U);
  float c_quad = temp_lld_read_celsius();
  float expect_quad = raw_c + 0.01f * raw_c * raw_c;
  bool quad_ok = (c_quad >= expect_quad - 1.0f) && (c_quad <= expect_quad + 1.0f);
  PRINTF("corr { 0, 1, 0.01 }: %.2f (expect ~%.2f) %s\r\n",
         (double)c_quad, (double)expect_quad, quad_ok ? "PASS" : "FAIL");
  ok = ok && quad_ok;

  const float cubic_corr[4] = { 0.0f, 1.0f, 0.0f, 0.0001f };  /* T' = T + 1e-4*T^3 */
  temp_lld_set_correction(cubic_corr, 4U);
  float c_cub = temp_lld_read_celsius();
  float expect_cub = raw_c + 0.0001f * raw_c * raw_c * raw_c;
  bool cub_ok = (c_cub >= expect_cub - 1.5f) && (c_cub <= expect_cub + 1.5f);
  PRINTF("corr { 0, 1, 0, 1e-4 } (degree 3): %.2f (expect ~%.2f) %s\r\n",
         (double)c_cub, (double)expect_cub, cub_ok ? "PASS" : "FAIL");
  ok = ok && cub_ok;

  temp_lld_clear_correction();   /* default: all-zero coeffs = identity */
  float c_clear = temp_lld_read_celsius();
  bool clear_ok = (c_clear >= raw_c - 1.0f) && (c_clear <= raw_c + 1.0f);
  PRINTF("correction cleared: %.2f (back to raw ~%.2f) %s\r\n",
         (double)c_clear, (double)raw_c, clear_ok ? "PASS" : "FAIL");
  ok = ok && clear_ok;

  PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL");

  while (true) { chThdSleepMilliseconds(1000); }
}
