/*
    ChibiOS - Copyright (C) 2026 Xael South

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/*
 * BURTC peripheral test (beyond the RTC calendar test):
 *  1. BURAM retention + BURTC CNT continuity across a warm reset
 *     (the test resets itself once via NVIC_SystemReset).
 *  2. CNT counting rate vs. EFR32_BURTCCLK (LFXO 32768 Hz).
 *  3. 32-bit overflow: CNT set near the top, OF interrupt must fire and
 *     the BURAM-resident ovf_counter (high word of the 64-bit tick) must
 *     increment. This trashes the calendar CNT — it is the last test.
 *
 * HAL_USE_RTC=TRUE so rtc_lld_init() has already enabled the BURTC clock,
 * configured CNTPRESC and started the counter at halInit(); the BURTC IRQ
 * vector is owned by BURTCv1 (rtc_lld_serve_interrupt).
 */

#include "ch.h"
#include "hal.h"
#include "hal_buram.h"   /* BURAM1_ADDRESS */
#include "rtt_console.h"

/* BURAM words used by this test: the RTC driver owns the low 20 bytes
   (efr32_rtc_buram_state_t at EFR32_RTC_BURAM_BASE=0), so use the last
   two words of the 128-byte BURAM1. Raw access on purpose: going through
   buramAllocateAtI() would assert (the RTC chunk covers the low words and
   the allocator bitmap would flag overlap only below its chunk — the
   top words are free, but raw access keeps this test independent). */
#define BURTC_TEST_MAGIC   0xC0FFEE01UL
#define BURTC_TEST_WORD0   (*(volatile uint32_t *)(BURAM1_ADDRESS + 0x78U))
#define BURTC_TEST_WORD1   (*(volatile uint32_t *)(BURAM1_ADDRESS + 0x7CU))

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nBURTC test: peripheral-level (BURAM retention, CNT rate, overflow)\r\n");
  PRINTF("BURTCCLK=%lu Hz CNTPRESC=%lu CNT=%lu\r\n",
         (unsigned long)EFR32_BURTCCLK,
         (unsigned long)((BURTC->CFG & _BURTC_CFG_CNTPRESC_MASK) >> _BURTC_CFG_CNTPRESC_SHIFT),
         (unsigned long)BURTC->CNT);

  bool ok = true;

  /*
   * 0. Diagnostic: is a raw CNT read stable? Read it 8 times back-to-back
   *    — at 32768 Hz the counter changes every ~30 us, so consecutive
   *    reads ~30 ns apart should almost always be equal; spread shows
   *    read-sync artifacts.
   */
  {
    uint32_t first = BURTC->CNT;
    uint32_t mn = first, mx = first;
    for (int i = 0; i < 8; i++) {
      uint32_t v = BURTC->CNT;
      if (v < mn) mn = v;
      if (v > mx) mx = v;
    }
    PRINTF("diag: 9 rapid CNT reads, min=%lu max=%lu (spread %lu)\r\n",
           (unsigned long)mn, (unsigned long)mx, (unsigned long)(mx - mn));

    /* P1: plain reads across 100 ms sleeps */
    uint32_t a = BURTC->CNT;
    chThdSleepMilliseconds(100);
    uint32_t b = BURTC->CNT;
    chThdSleepMilliseconds(100);
    uint32_t c = BURTC->CNT;
    PRINTF("diag P1: %lu -> %lu -> %lu (deltas %ld, %ld; expect ~3277)\r\n",
           (unsigned long)a, (unsigned long)b, (unsigned long)c,
           (long)(b - a), (long)(c - b));
    /* P2: write CNT (syncs to LF domain), then read across sleeps */
    BURTC->CNT = c;   /* re-write same value */
    while ((BURTC->SYNCBUSY & BURTC_SYNCBUSY_CNT) != 0U) { }
    uint32_t d = BURTC->CNT;
    chThdSleepMilliseconds(100);
    uint32_t e = BURTC->CNT;
    chThdSleepMilliseconds(100);
    uint32_t f = BURTC->CNT;
    PRINTF("diag P2 after CNT write: %lu -> %lu -> %lu (deltas %ld, %ld; expect ~3277)\r\n",
           (unsigned long)d, (unsigned long)e, (unsigned long)f,
           (long)(e - d), (long)(f - e));
    /* P3: 1 s gap */
    uint32_t g = BURTC->CNT;
    chThdSleepMilliseconds(1000);
    uint32_t h = BURTC->CNT;
    PRINTF("diag P3 1s gap: %lu -> %lu (delta %ld; expect ~32768)\r\n",
           (unsigned long)g, (unsigned long)h, (long)(h - g));
  }

  /*
   * 1. Warm-reset retention. First pass: store a magic word in BURAM and
   *    issue a warm reset. Second pass: magic present -> BURAM survived.
   *    (CNT continuity is NOT checked via raw reads — see diag above; the
   *    overflow test below proves the counter keeps running.)
   */
  if (BURTC_TEST_WORD0 == BURTC_TEST_MAGIC) {
    PRINTF("warm reset: BURAM magic retained across reset: PASS\r\n");
    BURTC_TEST_WORD0 = 0U;   /* disarm, so a later power-on starts fresh */
  }
  else {
    BURTC_TEST_WORD1 = BURTC->CNT;
    BURTC_TEST_WORD0 = BURTC_TEST_MAGIC;
    PRINTF("warm reset: arming BURAM magic, resetting...\r\n");
    chThdSleepMilliseconds(100);   /* let RTT drain before the reset */
    SCB->AIRCR = (0x5FAUL << SCB_AIRCR_VECTKEYSTAT_Pos) | SCB_AIRCR_SYSRESETREQ_Msk;
    for (;;) { }   /* wait for the reset */
  }

  /*
   * 2. Calendar tick: COMP fires every EFR32_RTC_COMP_PERIOD ticks (1 s) and
   *    increments comp_cnt in BURAM. Re-anchor first with rtcSetTime (resets
   *    CNT, comp_cnt=0, COMP=period) so the CNT<->comp_cnt relation is
   *    aligned — the OF sub-test below deliberately breaks it.
   */
  {
    RTCDateTime ts = { .year = 46, .month = 11, .day = 22, .dayofweek = 0,
                       .millisecond = 0, .dstflag = 0 };
    rtcSetTime(&RTCD1, &ts);
    uint32_t c0 = RTCD1.buram_state->comp_cnt;
    chThdSleepMilliseconds(2000);
    uint32_t c1 = RTCD1.buram_state->comp_cnt;
    uint32_t delta = c1 - c0;
    bool pass = (delta >= 2U) && (delta <= 3U);   /* 2 s -> 2 ticks, +1 slack */
    PRINTF("calendar tick: comp_cnt %lu -> %lu over 2000 ms (expect 2): %s\r\n",
           (unsigned long)c0, (unsigned long)c1, pass ? "PASS" : "FAIL");
    ok = ok && pass;
  }

  /*
   * 3. Rate + 32-bit overflow in one shot, measured against the SYSRTC
   *      system tick (accurate 32768 Hz): set CNT to (top - 32768) so it
   *      must wrap after exactly 32768 BURTC ticks, then wait for the OF
   *      flag. If BURTCCLK really is 32768 Hz this takes ~1000 ms of
   *      system time; the OF interrupt must also increment the
   *      BURAM-resident ovf_counter (high word of the 64-bit tick).
   *      NOTE: this deliberately corrupts the calendar CNT — last test.
   */
  {
    uint32_t ovf0 = RTCD1.buram_state->ovf_counter;
    const uint32_t n = 32768U;
    BURTC->CNT = _BURTC_CNT_CNT_MASK - n + 1U;
    while ((BURTC->SYNCBUSY & _BURTC_SYNCBUSY_MASK) != 0U) { }
    BURTC->IF_CLR = BURTC_IF_OF;
    syssts_t t0 = chVTGetSystemTimeX();
    syssts_t t1 = t0;
    /* The BURTC ISR clears IF.Of when it increments ovf_counter, so poll
       ovf_counter (in BURAM, written from ISR context) as the OF evidence. */
    while ((RTCD1.buram_state->ovf_counter == ovf0) &&
           ((t1 - t0) < 3U * CH_CFG_ST_FREQUENCY)) {
      t1 = chVTGetSystemTimeX();
    }
    uint32_t ovf1 = RTCD1.buram_state->ovf_counter;
    uint32_t dt_sys = (uint32_t)(t1 - t0);   /* systick ticks, 32768 Hz */
    bool of_fired = (ovf1 == ovf0 + 1U);
    /* expected dt_sys ≈ n if BURTCCLK == 32768 Hz; allow 5% */
    bool rate_ok = of_fired && (dt_sys >= n - n / 20U) && (dt_sys <= n + n / 20U);
    PRINTF("overflow/rate: OF fired=%u after %lu sys-ticks for %lu CNT ticks "
           "(~1 s expected), ovf_counter %lu -> %lu: %s\r\n",
           of_fired ? 1U : 0U, (unsigned long)dt_sys, (unsigned long)n,
           (unsigned long)ovf0, (unsigned long)ovf1,
           (rate_ok && of_fired) ? "PASS" : "FAIL");
    ok = ok && rate_ok && of_fired;
    BURTC->IF_CLR = BURTC_IF_OF;
  }

  PRINTF("BURTC: %s\r\n", ok ? "ALL PASS" : "FAIL");
  while (true) {
    chThdSleepMilliseconds(1000);
  }
}
