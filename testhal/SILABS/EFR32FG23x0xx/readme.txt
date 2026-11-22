testhal/SILABS/EFR32FG23x0xx - ChibiOS HAL test suite for Silicon Labs EFR32FG23
=============================================================================
Target board: EFR32FG23 DK2600A (BRD2600A, EFR32FG23B010F512IM48)
Debugger: SEGGER J-Link Pro OB (USB 1366:0105), SWD @ 4000 kHz.

Connections used (BRD2600A):
  LED0  = PB2          BTN0 = PB1, BTN1 = PA5
  Sensor I2C bus = I2C0 SCL=PA8 / SDA=PA7
  EUSART0 (SIOD1/SPID1) on PA9 (TX/MOSI) / PA10 (RX/MISO)
  TIMER3 CC0 routed to PB2 (LED0) for PWM-TIMER

Console: SEGGER RTT up-channel 0 (each test embeds a minimal RTT control
         block). View with:
           JLinkRTTLoggerExe -Device EFR32FG23B010F512IM48 -If SWD -Speed 4000 -RTTChannel 0 out.log
         or dump the rtt_buf symbol after a run (see /tmp/runtest.sh pattern).

System tick: SYSTICKv3 (LETIMER0 @ 32768 Hz, EM23GRPACLK from LFXO).
RTC calendar: BURTC (RTCD1). SYSRTC0 free counter + compare: SYSRTCv1.

Tests: TEMPLATE GPIO SIO RTC SYSRTC GPT-TIMER PWM-TIMER I2C SPI DMA
       EFL CRYPTO-AES WDG RAIL-TX SYSTICKv1
       (SYSTICKv1 builds with EFR32_USE_SYSTICKV1=yes = core SysTick;
        default: FREERUNNING, "make ST_PERIODIC=yes": PERIODIC)

Build:  cd <test> && make -j4
Flash:  make flash        (writes build/flash.jlink, runs JLinkExe)

On-target results (2026-11-22, J-Link SWD, RTT console):
  TEMPLATE    PASS  (boot banner + 1 s ticks)
  GPIO        PASS  (LED toggle, BTN0/BTN1 read)
  SIO         PASS  (EUSART0 async+sync TX/RX, 115200)
  RTC         PASS  (BURTC set/get round-trip, 1 s ticks)
  SYSRTC      PASS  (counter rate ~32768 Hz, CMP0 alarm)
  I2C         PASS  (bus scan: devices at 0x00 and 0x40)
  DMA         PASS  (LDMA mem-to-mem 256 B verified)
  EFL         PASS  (USERDATA last sector erase/program/verify)
  WDG         PASS  (10 kicks at ~1 s timeout, survived)
  GPT-TIMER   PASS  (hw TIMER3 overflow at 1.0 Hz, IRQ counted)
  PWM-TIMER   PASS  (hw TIMER3 CC0 -> PB2, duty sweep 10..90%)
  CRYPTO-AES  PASS  (RADIOAES NIST: ECB-128/256, CBC-128, CTR-128)
  SPI         NEEDS loopback wire PA9<->PA10 (TX path verified; without
              the jumper RX mismatches as expected)
  RAIL-TX     KNOWN ISSUE: HFXO now starts (DEVINFO ctune loaded in
              hal_lld.c) and HFRCO0 clock is enabled in RAILv1 glue, but
              RAIL_ConfigChannels asserts RAIL_ASSERT_INVALID_XTAL_FREQUENCY
              (66) - needs a Radio Configurator autogen channel config with
              matching xtalFrequencyHz. Radio AES (CRYPTO-AES) works.
  SYSTICKv1   PASS  (core SysTick @ HCLK, 10000 Hz: sleep 10/100/1000 ms
              measured as 100/1000/10000 ticks exactly; both FREERUNNING
              and PERIODIC ("make ST_PERIODIC=yes") modes ALL PASS)

Notes / driver quirks discovered on FG23 (all handled in the LLDs):
  - Peripheral bus clocks must be enabled (CMU CLKEN0) before register
    access, otherwise reads/writes bus-fault (TIMER0-4, HFRCO0, WDOG...).
  - TIMER CC channel registers: CCx_CFG writable only while EN=0;
    CCx_CTRL / CCx_OC bus-fault while EN=0 (writable once EN=1).
  - TIMER CFG PRESC field encoding is (1<<n)-1, not linear n.
  - WDOG0 lives in the always-on domain and survives resets; efr32_chip_init
    disables it at boot.
  - LFXO is in the backup domain; its start wait is bounded so a dead
    crystal cannot hang boot.
  - Stack sizes: tests use 0x1000 process/exceptions stacks (snprintf %f
    needs more than the 0x400 default).
