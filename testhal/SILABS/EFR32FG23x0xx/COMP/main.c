/*
    ChibiOS - Copyright (C) 2026 Xael South
    Apache License 2.0 — see repository root.

    COMP test: ACMP0 analog comparator with internal references only
    (no external wiring needed):
      A) pos = VDD/64 (~0.05V), neg = 2.5V bandgap  -> output 0
      B) pos = VDD/64 (~0.05V), neg = VSS            -> output 1
    Also exercises the RISE/FALL interrupt callback.
*/
#include "ch.h"
#include "hal.h"
#include "rtt_console.h"
#include "hal_comp.h"

static volatile uint32_t cb_count;

static void comp_cb(COMPDriver *cp) {
  (void)cp;
  cb_count++;
}

static bool run_case(const COMPConfig *cfg, bool expect) {
  compStart(&COMPD1, cfg);
  chThdSleepMilliseconds(50);   /* let the analog front-end settle */
  bool out = comp_lld_get_output(&COMPD1);
  compStop(&COMPD1);
  PRINTF("  ACMPOUT=%d (expect %d) %s\r\n", (int)out, (int)expect,
         (out == expect) ? "PASS" : "FAIL");
  return out == expect;
}

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nCOMP test: ACMP0 with internal references\r\n");

  compInit();

  /* pos = VSS (0V), neg = VDD -> output must be 0 */
  static const COMPConfig cfgA = {
    .cfg       = ACMP_CFG_BIAS_DEFAULT,
    .ctrl      = 0,
    .inputctrl = ACMP_INPUTCTRL_POSSEL_VSS |
                 ACMP_INPUTCTRL_NEGSEL_VREFDIVAVDD |
                 (0U << _ACMP_INPUTCTRL_VREFDIV_SHIFT),
    .cb        = comp_cb,
  };
  /* pos = VDD, neg = VSS -> output must be 1 */
  static const COMPConfig cfgB = {
    .cfg       = ACMP_CFG_BIAS_DEFAULT,
    .ctrl      = 0,
    .inputctrl = ACMP_INPUTCTRL_POSSEL_VREFDIVAVDD |
                 ACMP_INPUTCTRL_NEGSEL_VSS |
                 (0U << _ACMP_INPUTCTRL_VREFDIV_SHIFT),
    .cb        = comp_cb,
  };

  bool ok = true;

  PRINTF("case A: pos=VSS vs neg=VDD (expect 0):\r\n");
  ok &= run_case(&cfgA, false);

  PRINTF("case B: pos=VDD vs neg=VSS (expect 1):\r\n");
  ok &= run_case(&cfgB, true);

  PRINTF("interrupts seen: %lu %s\r\n", (unsigned long)cb_count,
         (cb_count != 0U) ? "PASS" : "FAIL");
  ok &= (cb_count != 0U);

  PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL");

  while (true) { chThdSleepMilliseconds(1000); }
}
