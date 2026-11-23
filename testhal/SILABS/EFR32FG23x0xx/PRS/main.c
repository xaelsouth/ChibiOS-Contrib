/*
    ChibiOS - Copyright (C) 2026 Xael South
    Apache License 2.0 — see repository root.

    PRS test (generic hal_prs driver): connect async ch0 to GPIO pin 0
    (PA0, driven push-pull), route ch0 out to PA4, and verify ASYNC_PEEK
    bit0 and the PA4 pad follow PA0. Also checks per-channel logic
    (FNSEL), a peripheral consumer route, software level/pulse, sync
    channel connect/disconnect, and clock gating on stop.
*/
#include "ch.h"
#include "hal.h"
#include "rtt_console.h"
#include "hal_prs.h"

int main(void) {
  halInit();
  chSysInit();
  console_init();

  PRINTF("\r\nPRS test: async ch0 GPIO.PA0 -> PEEK + pin PA4 (PA1/PA2 are SWCLK/SWDIO)\r\n");

  bool ok = true;

  prsInit();
  prsObjectInit(&PRSD1);
  prsStart(&PRSD1, NULL);
  bool clk = ((CMU->CLKEN0 & CMU_CLKEN0_PRS) != 0U);
  PRINTF("after start: CLKEN0.PRS=%d %s\r\n", (int)clk, clk ? "PASS" : "FAIL");
  ok = ok && clk;

  /* PA0 = driven source pin, PA4 = PRS async ch0 output pin. */
  palSetPadMode(GPIOA, 0, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOA, 4, PRS_PAD_MODE_OUT);

  /* 1) connect async ch0 to GPIO pin 0. */
  prsConnect(&PRSD1, PRS_PATH_ASYNC, 0, PRS_GEN_GPIO_PIN0);
  uint32_t ctrl = PRS->ASYNC_CH[0].CTRL;
  bool conn_ok = (ctrl & _PRS_ASYNC_CH_CTRL_SOURCESEL_MASK) != 0U;
  PRINTF("async ch0 CTRL=0x%08lX %s\r\n", (unsigned long)ctrl, conn_ok ? "PASS" : "FAIL");
  ok = ok && conn_ok;

  /* 2) route async ch0 to PA4. */
  prsRoutePin(&PRSD1, PRS_PATH_ASYNC, 0, GPIOA, 4);
  uint32_t ren = GPIO->PRSROUTE[0].ROUTEEN;
  uint32_t r0  = GPIO->PRSROUTE[0].ASYNCH0ROUTE;
  bool route_ok = ((ren & GPIO_PRS_ROUTEEN_ASYNCH0PEN) != 0U) &&
                  (r0 == ((0UL << _GPIO_PRS_ASYNCH0ROUTE_PORT_SHIFT) |
                          (4UL << _GPIO_PRS_ASYNCH0ROUTE_PIN_SHIFT)));
  PRINTF("route: ROUTEEN=0x%lX ASYNCH0ROUTE=0x%lX %s\r\n",
         (unsigned long)ren, (unsigned long)r0, route_ok ? "PASS" : "FAIL");
  ok = ok && route_ok;

  /* 3) PA0 low -> PEEK bit0 = 0 and PA4 low; PA0 high -> both high. */
  palClearPad(GPIOA, 0);
  chThdSleepMilliseconds(10);
  uint32_t peek_lo = prsPeek(&PRSD1, PRS_PATH_ASYNC);
  bool pa4_lo = !palReadPad(GPIOA, 4);
  palSetPad(GPIOA, 0);
  chThdSleepMilliseconds(10);
  uint32_t peek_hi = prsPeek(&PRSD1, PRS_PATH_ASYNC);
  bool pa4_hi = palReadPad(GPIOA, 4);
  PRINTF("PA0=0: PEEK=0x%lX PA4=%d; PA0=1: PEEK=0x%lX PA4=%d %s\r\n",
         (unsigned long)peek_lo, (int)!pa4_lo,
         (unsigned long)peek_hi, (int)pa4_hi,
         (((peek_lo & 1U) == 0U) && pa4_lo && ((peek_hi & 1U) != 0U) && pa4_hi)
           ? "PASS" : "FAIL");
  ok = ok && ((peek_lo & 1U) == 0U) && pa4_lo && ((peek_hi & 1U) != 0U) && pa4_hi;

  /* 4) per-channel logic: NOT_A on ch0 with aux ch1 (ch1 = SWLEVEL 0 -> 0). */
  prsSetLogic(&PRSD1, 0, PRS_FN_NOT_A, 1);
  uint32_t fnsel = (PRS->ASYNC_CH[0].CTRL & _PRS_ASYNC_CH_CTRL_FNSEL_MASK);
  bool fn_ok = (fnsel == PRS_ASYNC_CH_CTRL_FNSEL_NOT_A);
  PRINTF("set_logic NOT_A: FNSEL=0x%lX %s\r\n",
         (unsigned long)fnsel, fn_ok ? "PASS" : "FAIL");
  ok = ok && fn_ok;
  /* Restore pass-through (connect rewrites FNSEL_A). */
  prsConnect(&PRSD1, PRS_PATH_ASYNC, 0, PRS_GEN_GPIO_PIN0);

  /* 5) peripheral consumer: route async ch0 to LETIMER0 START. */
  prsConnectConsumer(&PRSD1, PRS_CON_LETIMER0_START, PRS_PATH_ASYNC, 0);
  uint32_t cons = PRS->CONSUMER_LETIMER0_START;
  bool cons_ok = (cons == 0U);   /* PRSSEL = async ch0 */
  PRINTF("consumer LETIMER0_START=0x%lX %s\r\n",
         (unsigned long)cons, cons_ok ? "PASS" : "FAIL");
  ok = ok && cons_ok;

  /* 6) software level on ch3 (no source selected). */
  prsSwLevelSet(&PRSD1, 0x8U);
  chThdSleepMilliseconds(10);
  uint32_t peek_sw1 = prsPeek(&PRSD1, PRS_PATH_ASYNC);
  prsSwLevelClear(&PRSD1, 0x8U);
  chThdSleepMilliseconds(10);
  uint32_t peek_sw0 = prsPeek(&PRSD1, PRS_PATH_ASYNC);
  bool sw_ok = (((peek_sw1 >> 3) & 1U) == 1U) && (((peek_sw0 >> 3) & 1U) == 0U);
  PRINTF("sw level ch3: set PEEK=0x%lX, clear PEEK=0x%lX %s\r\n",
         (unsigned long)peek_sw1, (unsigned long)peek_sw0, sw_ok ? "PASS" : "FAIL");
  ok = ok && sw_ok;

  /* 7) software pulse on ch2 — must not fault; pulse is momentary. */
  prsSwPulse(&PRSD1, 0x4U);
  uint32_t peek_pulse = prsPeek(&PRSD1, PRS_PATH_ASYNC);
  PRINTF("sw pulse ch2: PEEK after pulse=0x%lX (no fault) PASS\r\n",
         (unsigned long)peek_pulse);

  /* 8) sync channel connect/disconnect (TIMER0 CC0, timer not running). */
  prsConnect(&PRSD1, PRS_PATH_SYNC, 0, PRS_GEN_SYNC_TIMER0_CC0);
  uint32_t sctrl = PRS->SYNC_CH[0].CTRL;
  prsDisconnect(&PRSD1, PRS_PATH_SYNC, 0);
  uint32_t sctrl0 = PRS->SYNC_CH[0].CTRL;
  bool sync_ok = (sctrl != 0U) && (sctrl0 == 0U);
  PRINTF("sync ch0: CTRL=0x%lX -> after disconnect 0x%lX %s\r\n",
         (unsigned long)sctrl, (unsigned long)sctrl0, sync_ok ? "PASS" : "FAIL");
  ok = ok && sync_ok;

  /* 9) disconnect async ch0, unroute, stop. */
  prsDisconnect(&PRSD1, PRS_PATH_ASYNC, 0);
  prsUnroutePin(&PRSD1, PRS_PATH_ASYNC, 0);
  uint32_t ctrl0 = PRS->ASYNC_CH[0].CTRL;
  uint32_t ren0 = GPIO->PRSROUTE[0].ROUTEEN & GPIO_PRS_ROUTEEN_ASYNCH0PEN;
  prsStop(&PRSD1);
  uint32_t clk2 = CMU->CLKEN0 & CMU_CLKEN0_PRS;
  bool stop_ok = (ctrl0 == 0U) && (ren0 == 0U) && (clk2 == 0U);
  PRINTF("after stop: CTRL=0x%lX ROUTEEN bit0=%lu CLKEN0.PRS=0x%lX %s\r\n",
         (unsigned long)ctrl0, (unsigned long)ren0, (unsigned long)clk2,
         stop_ok ? "PASS" : "FAIL");
  ok = ok && stop_ok;

  PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL");

  while (true) { chThdSleepMilliseconds(1000); }
}
