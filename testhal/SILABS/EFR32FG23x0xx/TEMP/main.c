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
  tempObjectInit(&TEMPD2);

  /* Two objects over the same die sensor: TEMPD1 uncorrected, TEMPD2 with
     a -2 C offset correction given at start. */
  static const float corr2[3] = { -2.0f, 1.0f, 0.0f };
  static const TEMPERATUREConfig temp_cfg1 = { .corr = NULL,   .corr_num = 0U };
  static const TEMPERATUREConfig temp_cfg2 = { .corr = corr2,  .corr_num = 3U };
  tempStart(&TEMPD1, &temp_cfg1);
  tempStart(&TEMPD2, &temp_cfg2);
  bool first_done = ((EMU->STATUS & EMU_STATUS_FIRSTTEMPDONE) != 0U);
  PRINTF("TEMPD1 state=%d TEMPD2 state=%d (READY=%d) FIRSTTEMPDONE=%d %s\r\n",
         (int)TEMPD1.state, (int)TEMPD2.state, (int)TEMP_READY, (int)first_done,
         (TEMPD1.state == TEMP_READY && TEMPD2.state == TEMP_READY && first_done)
           ? "PASS" : "FAIL");
  ok = ok && (TEMPD1.state == TEMP_READY) && (TEMPD2.state == TEMP_READY)
       && first_done;

  /* Objects are independent: TEMPD2 must read ~2 C below TEMPD1. */
  float d1 = tempReadCelsius(&TEMPD1);
  float d2 = tempReadCelsius(&TEMPD2);
  bool indep_ok = (d2 >= d1 - 2.6f) && (d2 <= d1 - 1.4f);
  PRINTF("two objects: TEMPD1 %.2f C, TEMPD2 %.2f C (expect ~%.2f) %s\r\n",
         (double)d1, (double)d2, (double)(d1 - 2.0f), indep_ok ? "PASS" : "FAIL");
  ok = ok && indep_ok;

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
  tempStop(&TEMPD2);
  bool stop_ok = (TEMPD1.state == TEMP_STOP) && (TEMPD2.state == TEMP_STOP);
  PRINTF("after stop TEMPD1 state=%d TEMPD2 state=%d (STOP=%d) %s\r\n",
         (int)TEMPD1.state, (int)TEMPD2.state, (int)TEMP_STOP,
         stop_ok ? "PASS" : "FAIL");
  ok = ok && stop_ok;

  PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL");

  while (true) { chThdSleepMilliseconds(1000); }
}
