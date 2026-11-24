BURTC: peripheral-level test of the backup RTC (beyond the RTC calendar test).

HAL_USE_RTC=TRUE (BURTCv1 owns the BURTC IRQ vector and starts the counter
at halInit), HAL_USE_BURAM=TRUE. EM4GRPACLK = LFXO 32768 Hz.

Tests:
  1. BURAM retention + BURTC CNT continuity across a warm reset — the test
     stores a magic word + CNT in the top two BURAM words and resets itself
     once (NVIC_SystemReset); on the second boot it verifies both survived.
  2. CNT counting rate over 1 s vs. EFR32_BURTCCLK (32768 +/- 5%).
  3. 32-bit overflow: CNT is set to (mask - 200), OF must fire and the
     BURAM-resident ovf_counter (high word of the 64-bit tick) must
     increment. This corrupts the calendar CNT, so it runs last.

The calendar set/get path is covered by the RTC test, not repeated
here.

Expected: "BURTC: ALL PASS" (after one self-reset for test 1).
