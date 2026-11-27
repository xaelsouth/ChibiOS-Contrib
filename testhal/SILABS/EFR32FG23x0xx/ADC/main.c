/*
    ChibiOS - Copyright (C) 2026 Xael South
    Apache License 2.0 — see repository root.

    ADC test: IADC0 single conversion.
      1) portpos=GND, portneg=GND  -> signed result must be ~0
      2) portpos=PA0                -> completion smoke test only
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
  int32_t gnd = sample_buf[0] <= INT16_MAX ? (int32_t)sample_buf[0]
                                           : (int32_t)sample_buf[0] - 65536;
  bool gnd_ok = (r == HAL_RET_SUCCESS) && (gnd > -64) && (gnd < 64);
  PRINTF("GND vs GND: ret=%d signed code=%ld %s\r\n", (int)r,
         (long)gnd, gnd_ok ? "PASS" : "FAIL");
  if (!gnd_ok) {
    PRINTF("IADC SINGLE=0x%08lX CFG=0x%08lX DATA=0x%08lX IF=0x%08lX\r\n",
           (unsigned long)IADC0->SINGLE, (unsigned long)IADC0->CFG[0].CFG,
           (unsigned long)IADC0->SINGLEDATA, (unsigned long)IADC0->IF);
  }
  ok = ok && gnd_ok;

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
    .osrhs        = 1,   /* HiSpeed4x */
  };
  r = adcConvert(&ADCD1, &grp_pa0, sample_buf, 1);
  bool cfg_ok = (IADC0->CFG[0].CFG & (_IADC_CFG_REFSEL_MASK |
                                     _IADC_CFG_OSRHS_MASK)) ==
                (IADC_CFG_REFSEL_VDDX | IADC_CFG_OSRHS_HISPD4);
  PRINTF("PA0 vs GND (VDDX ref): ret=%d code=%u %s; group config %s\r\n", (int)r,
         (unsigned)sample_buf[0],
         (r == HAL_RET_SUCCESS) ? "PASS" : "FAIL", cfg_ok ? "PASS" : "FAIL");
  ok = ok && (r == HAL_RET_SUCCESS) && cfg_ok;

  adcStop(&ADCD1);
  uint32_t clk = CMU->CLKEN0 & CMU_CLKEN0_IADC0;
  bool ok2 = (clk == 0U);
  PRINTF("after stop CLKEN0.IADC0=0x%lX (expect 0): %s\r\n",
         (unsigned long)clk, ok2 ? "PASS" : "FAIL");
  ok = ok && ok2;

  PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL");

  while (true) { chThdSleepMilliseconds(1000); }
}
