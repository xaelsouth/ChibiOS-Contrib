SYSTICKv1 — system tick test for the optional SYSTICKv1 driver
(ARM Cortex-M core SysTick, os/hal/ports/SILABS/LLD/SYSTICKv1).

The Makefile sets EFR32_USE_SYSTICKV1=yes, so the build uses the core
SysTick instead of the default SYSTICKv2 (SYSRTC0).

Modes:
  make            -> OSAL_ST_MODE_FREERUNNING (CH_CFG_ST_TIMEDELTA=8,
                     CH_CFG_ST_FREQUENCY=10000, CH_CFG_ST_RESOLUTION=32)
  make ST_PERIODIC=yes -> OSAL_ST_MODE_PERIODIC (CH_CFG_ST_TIMEDELTA=0)

Note: SYSTICKv1 requires HCLK % CH_CFG_ST_FREQUENCY == 0; with the
19 MHz HFRCODPLL clock use 10000 Hz (32768 does not divide 19 MHz).

The test measures the system-time delta of chThdSleepMilliseconds()
(10/100/1000 ms) against OSAL_ST_FREQUENCY — this checks both the tick
rate and the alarm accuracy — then blinks the LED and prints "tick"
every second. Console is SEGGER RTT channel 0 (see runtest.sh).

Expected: "SYSTICKv1: ALL PASS" followed by one "tick" per second.

Known limitation: the core SysTick stops in EM2/EM3 (it is clocked from
HCLK) — for low-power use the platform default SYSTICKv2 (SYSRTC, EM2/EM4).
