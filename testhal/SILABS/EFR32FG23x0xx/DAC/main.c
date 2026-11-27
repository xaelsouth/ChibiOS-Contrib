/*
    ChibiOS - Copyright (C) 2026 Xael South
    Apache License 2.0 — see repository root.

    DAC test: VDAC0 channels 1/2 (DACD1/DACD2), single-ended continuous
    mode. Checks direct CH0 output through the IADC internal connection.
*/
#include "ch.h"
#include "hal.h"
#include "rtt_console.h"
#include "hal_dac.h"

static const ADCConversionGroup dac_adc_group = {
  .circular = false,
  .num_channels = 1,
  .portpos = EFR32_ADC_PORT_DAC1,
  .portneg = EFR32_ADC_PORT_GND,
  .pin = 0,
  .refsel = 0,
  .osrhs = 0,
};

static const ADCConversionGroup dac_adc_vddx_group = {
  .circular = false,
  .num_channels = 1,
  .portpos = EFR32_ADC_PORT_DAC1,
  .portneg = EFR32_ADC_PORT_GND,
  .pin = 0,
  .refsel = 3,
  .osrhs = 0,
};

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nDAC test: VDAC0 CH0/CH1 single-ended\r\n");

  static const DACConfig cfg = { };
  const efr32_dma_stream_t *streams[EFR32_DMA_STREAMS];
  unsigned allocated = 0U;
  while (allocated < EFR32_DMA_STREAMS) {
    const efr32_dma_stream_t *st = dmaStreamAlloc(EFR32_DMA_STREAM_ID_ANY,
                                                  EFR32_DAC_DAC1_IRQ_PRIORITY,
                                                  NULL, NULL);
    if (st == NULL) {
      break;
    }
    streams[allocated++] = st;
  }
  msg_t no_dma = dacStart(&DACD1, &cfg);
  bool no_dma_ok = no_dma == HAL_RET_NO_RESOURCE && DACD1.state == DAC_STOP;
  PRINTF("DMA exhaustion rejected: %s\r\n", no_dma_ok ? "PASS" : "FAIL");
  while (allocated > 0U) {
    dmaStreamFree(streams[--allocated]);
  }

  msg_t r1 = dacStart(&DACD1, &cfg);
  msg_t r2 = dacStart(&DACD2, &cfg);
  PRINTF("dacStart CH0=%d CH1=%d %s\r\n", (int)r1, (int)r2,
         (r1 == HAL_RET_SUCCESS && r2 == HAL_RET_SUCCESS) ? "PASS" : "FAIL");

  chThdSleepMilliseconds(50);   /* warmup */

  uint32_t st = VDAC0->STATUS;
  PRINTF("STATUS=0x%08lX (CH0ENS|CH0WARM|CH1ENS|CH1WARM=0x33)\r\n",
         (unsigned long)st);
  bool ok = no_dma_ok && (r1 == HAL_RET_SUCCESS) && (r2 == HAL_RET_SUCCESS) &&
            ((st & 0x33U) == 0x33U);
  PRINTF("channels enabled+warm: %s\r\n", ok ? "PASS" : "FAIL");

  msg_t p1 = dacPutChannelX(&DACD1, 0, 0x0000U);
  msg_t p2 = dacPutChannelX(&DACD1, 0, 0x0FFFU);
  msg_t p3 = dacPutChannelX(&DACD2, 1, 0x0800U);
  bool ok2 = (p1 == HAL_RET_SUCCESS) && (p2 == HAL_RET_SUCCESS) &&
             (p3 == HAL_RET_SUCCESS);
  PRINTF("put 0x0000/0x0FFF (CH0), 0x0800 (CH1): %s\r\n",
         ok2 ? "PASS" : "FAIL");
  ok = ok && ok2;

  /* Connect CH0 to the internal IADC input while the channel is disabled. */
  VDAC0->CMD_SET = VDAC_CMD_CH0DIS;
  for (unsigned i = 0; i < 100U &&
       (VDAC0->STATUS & VDAC_STATUS_CH0ENS) != 0U; i++) {
    chThdSleepMilliseconds(1);
  }
  bool disabled = (VDAC0->STATUS & VDAC_STATUS_CH0ENS) == 0U;

  static const ADCConfig adc_cfg = { };
  adcsample_t analog[3] = {0U, 0U, 0U};
  msg_t a1 = HAL_RET_CONFIG_ERROR, a2 = HAL_RET_CONFIG_ERROR;
  msg_t a3 = HAL_RET_CONFIG_ERROR;
  if (disabled) {
    VDAC0->OUTCTRL_SET = VDAC_OUTCTRL_AUXOUTENCH0 | VDAC_OUTCTRL_SHORTCH0;
    VDAC0->CMD_SET = VDAC_CMD_CH0EN;
    chThdSleepMilliseconds(50);
    if (adcStart(&ADCD1, &adc_cfg) == HAL_RET_SUCCESS) {
      dacPutChannelX(&DACD1, 0, 0x0400U);
      chThdSleepMilliseconds(10);
      a1 = adcConvert(&ADCD1, &dac_adc_group, &analog[0], 1);
      dacPutChannelX(&DACD1, 0, 0x0C00U);
      chThdSleepMilliseconds(10);
      a2 = adcConvert(&ADCD1, &dac_adc_group, &analog[1], 1);
      a3 = adcConvert(&ADCD1, &dac_adc_vddx_group, &analog[2], 1);
      adcStop(&ADCD1);
    }
  } else {
    PRINTF("CH0 failed to disable for internal routing\r\n");
  }
  bool analog_ok = a1 == HAL_RET_SUCCESS && a2 == HAL_RET_SUCCESS &&
                   a3 == HAL_RET_SUCCESS &&
                   analog[0] > 100U && analog[1] > analog[0] * 2U &&
                   analog[1] < analog[0] * 4U &&
                   analog[1] > analog[2] * 2U &&
                   analog[1] < analog[2] * 4U;
  PRINTF("CH0 internal IADC VBGR 1/4=%u 3/4=%u, VDDX 3/4=%u: %s\r\n",
         (unsigned)analog[0], (unsigned)analog[1], (unsigned)analog[2],
         analog_ok ? "PASS" : "FAIL");
  ok &= analog_ok;

  dacStop(&DACD1);
  chThdSleepMilliseconds(5);
  st = VDAC0->STATUS;
  bool stopped0 = (st & VDAC_STATUS_CH0ENS) == 0U &&
                  (st & VDAC_STATUS_CH1ENS) != 0U;
  PRINTF("stop CH0 while CH1 remains enabled: %s\r\n",
         stopped0 ? "PASS" : "FAIL");
  ok = ok && stopped0;
  dacStop(&DACD2);
  /* After both channels stop, the LLD gates the VDAC bus clock. */
  uint32_t clk = CMU->CLKEN1 & CMU_CLKEN1_VDAC0;
  bool ok3 = (clk == 0U);
  PRINTF("after stop CLKEN1.VDAC0=0x%lX (expect 0): %s\r\n",
         (unsigned long)clk, ok3 ? "PASS" : "FAIL");
  ok = ok && ok3;

  PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL");

  while (true) { chThdSleepMilliseconds(1000); }
}
