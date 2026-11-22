/*
    ChibiOS - Copyright (C) 2026 Xael South
    Apache License 2.0 — see repository root.

    ADC test: IADC0 single conversion.
      1) portpos=GND, portneg=GND  -> result must be ~0
      2) portpos=VDDX via refsel    -> sanity: just runs and returns a value
    Uses the synchronous adcConvert() API (waits on SINGLEFIFODVL IRQ).
*/
#include "ch.h"
#include "hal.h"
#include "rtt_console.h"
#include "hal_adc.h"

static adcsample_t sample_buf[4];

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nADC test: IADC0 single conversions\r\n");

  adcInit();

  static const ADCConfig cfg = { };
  msg_t r = adcStart(&ADCD1, &cfg);
  PRINTF("adcStart=%d %s\r\n", (int)r,
         (r == HAL_RET_SUCCESS) ? "PASS" : "FAIL");
  bool ok = (r == HAL_RET_SUCCESS);

  /* Case 1: GND vs GND -> code ~0 */
  static const ADCConversionGroup grp_gnd = {
    .circular     = false,
    .num_channels = 1,
    .end_cb       = NULL,
    .error_cb     = NULL,
    .portpos      = EFR32_ADC_PORT_GND,
    .portneg      = EFR32_ADC_PORT_GND,
    .pin          = 0,
    .refsel       = 0,   /* VBGR */
    .osrhs        = 0,
  };
  r = adcConvert(&ADCD1, &grp_gnd, sample_buf, 1);
  PRINTF("GND vs GND: ret=%d code=%u %s\r\n", (int)r,
         (unsigned)sample_buf[0],
         (r == HAL_RET_SUCCESS && sample_buf[0] < 64U) ? "PASS" : "FAIL");
  ok = ok && (r == HAL_RET_SUCCESS) && (sample_buf[0] < 64U);

  /* Case 2: port A pin 0 (floating/board-dependent) — just must complete */
  static const ADCConversionGroup grp_pa0 = {
    .circular     = false,
    .num_channels = 1,
    .end_cb       = NULL,
    .error_cb     = NULL,
    .portpos      = EFR32_ADC_PORT_PORTA,
    .portneg      = EFR32_ADC_PORT_GND,
    .pin          = 0,
    .refsel       = 3,   /* VDDX */
    .osrhs        = 0,
  };
  r = adcConvert(&ADCD1, &grp_pa0, sample_buf, 1);
  PRINTF("PA0 vs GND (VDDX ref): ret=%d code=%u %s\r\n", (int)r,
         (unsigned)sample_buf[0],
         (r == HAL_RET_SUCCESS) ? "PASS" : "FAIL");
  ok = ok && (r == HAL_RET_SUCCESS);

  adcStop(&ADCD1);
  uint32_t clk = CMU->CLKEN0 & CMU_CLKEN0_IADC0;
  bool ok2 = (clk == 0U);
  PRINTF("after stop CLKEN0.IADC0=0x%lX (expect 0): %s\r\n",
         (unsigned long)clk, ok2 ? "PASS" : "FAIL");
  ok = ok && ok2;

  PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL");

  while (true) { chThdSleepMilliseconds(1000); }
}
