/*
    ChibiOS - Copyright (C) 2026 Xael South
    Apache License 2.0 — see repository root.

    TEMP test: generic hal_temperature driver on top of TEMPv1 (EMU->TEMP,
    Kelvin x 4). Reads the die temperature 3 times over 3 s via the upper
    layer; expects -40..125 degC and a stable reading (<= 3 degC spread).
    Then verifies the upper-layer correction polynomial (Horner schema,
    any degree) with constant, quadratic and cubic coefficients.
*/
#include "ch.h"
#include "hal.h"
#include "rtt_console.h"
#include "hal_temperature.h"

int main(void) {
  halInit();
  chSysInit();
  console_init();

  PRINTF("\r\nTEMP test: hal_temperature + TEMPv1 (EMU die sensor)\r\n");

  bool ok = true;

  tempInit();
  tempObjectInit(&TEMPD1);

  static const TEMPERATUREConfig temp_cfg = {
    .corr     = NULL,   /* no correction at start */
    .corr_num = 0U,
  };
  tempStart(&TEMPD1, &temp_cfg);
  bool first_done = ((EMU->STATUS & EMU_STATUS_FIRSTTEMPDONE) != 0U);
  PRINTF("state=%d (READY=%d) FIRSTTEMPDONE=%d %s\r\n", (int)TEMPD1.state,
         (int)TEMP_READY, (int)first_done,
         (TEMPD1.state == TEMP_READY && first_done) ? "PASS" : "FAIL");
  ok = ok && (TEMPD1.state == TEMP_READY) && first_done;

  float min_c = 1000.0f, max_c = -1000.0f;
  for (int i = 0; i < 3; i++) {
    chThdSleepMilliseconds(1000);
    uint32_t raw = tempReadRaw(&TEMPD1);
    float celsius = tempReadCelsius(&TEMPD1);
    PRINTF("read %d: raw=%lu = %.2f C\r\n", i, (unsigned long)raw, (double)celsius);
    if (celsius < min_c) min_c = celsius;
    if (celsius > max_c) max_c = celsius;
  }

  bool range_ok = (min_c >= -40.0f) && (max_c <= 125.0f);
  float spread = max_c - min_c;
  bool stable_ok = (spread <= 3.0f);
  PRINTF("range [-40..125]C: %s, spread %.2f C (<=3): %s\r\n",
         range_ok ? "PASS" : "FAIL", (double)spread, stable_ok ? "PASS" : "FAIL");
  ok = ok && range_ok && stable_ok;

  /* Correction polynomial (upper layer, Horner, any degree). */
  float raw_c = tempReadCelsius(&TEMPD1);

  const float offs_corr[3] = { -2.0f, 1.0f, 0.0f };       /* T' = T - 2 */
  tempSetCorrection(&TEMPD1, offs_corr, 3U);
  float c_offs = tempReadCelsius(&TEMPD1);
  bool offs_ok = (c_offs >= raw_c - 2.6f) && (c_offs <= raw_c - 1.4f);
  PRINTF("corr { -2, 1, 0 }: raw %.2f -> %.2f (expect ~%.2f) %s\r\n",
         (double)raw_c, (double)c_offs, (double)(raw_c - 2.0f),
         offs_ok ? "PASS" : "FAIL");
  ok = ok && offs_ok;

  const float quad_corr[3] = { 0.0f, 1.0f, 0.01f };       /* T' = T + 0.01*T^2 */
  tempSetCorrection(&TEMPD1, quad_corr, 3U);
  float c_quad = tempReadCelsius(&TEMPD1);
  float expect_quad = raw_c + 0.01f * raw_c * raw_c;
  bool quad_ok = (c_quad >= expect_quad - 1.0f) && (c_quad <= expect_quad + 1.0f);
  PRINTF("corr { 0, 1, 0.01 }: %.2f (expect ~%.2f) %s\r\n",
         (double)c_quad, (double)expect_quad, quad_ok ? "PASS" : "FAIL");
  ok = ok && quad_ok;

  const float cubic_corr[4] = { 0.0f, 1.0f, 0.0f, 0.0001f };  /* T' = T + 1e-4*T^3 */
  tempSetCorrection(&TEMPD1, cubic_corr, 4U);
  float c_cub = tempReadCelsius(&TEMPD1);
  float expect_cub = raw_c + 0.0001f * raw_c * raw_c * raw_c;
  bool cub_ok = (c_cub >= expect_cub - 1.5f) && (c_cub <= expect_cub + 1.5f);
  PRINTF("corr { 0, 1, 0, 1e-4 } (degree 3): %.2f (expect ~%.2f) %s\r\n",
         (double)c_cub, (double)expect_cub, cub_ok ? "PASS" : "FAIL");
  ok = ok && cub_ok;

  tempClearCorrection(&TEMPD1);   /* no coefficients set -> no correction */
  float c_clear = tempReadCelsius(&TEMPD1);
  bool clear_ok = (c_clear >= raw_c - 1.0f) && (c_clear <= raw_c + 1.0f);
  PRINTF("correction cleared: %.2f (back to raw ~%.2f) %s\r\n",
         (double)c_clear, (double)raw_c, clear_ok ? "PASS" : "FAIL");
  ok = ok && clear_ok;

  tempStop(&TEMPD1);
  PRINTF("after stop state=%d (STOP=%d) %s\r\n", (int)TEMPD1.state,
         (int)TEMP_STOP, (TEMPD1.state == TEMP_STOP) ? "PASS" : "FAIL");
  ok = ok && (TEMPD1.state == TEMP_STOP);

  PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL");

  while (true) { chThdSleepMilliseconds(1000); }
}
