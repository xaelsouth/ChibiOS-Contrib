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
       EFL CRYPTO-AES WDG RAIL-TX SYSTICKv1 SYSTICKv2 SYSTICKv3
       (SYSTICKv1 builds with EFR32_USE_SYSTICKV1=yes = core SysTick;
        SYSTICKv2 with EFR32_USE_SYSTICKV2=yes = SYSRTC0;
        SYSTICKv3 is the platform default = LETIMER0, 24-bit + UF ext)

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
  LESENSE     PASS  (periodic scan ch0, SCANDONE IRQs counted over 5 s,
              measured scan rate in range)
  SYSTICKv2   PASS  (SYSRTC as system tick)
  SYSTICKv3   PASS  (LETIMER0 default, 24-bit counter + UF interrupt ext to
              32 bits, FREERUNNING @ 32768 Hz: sleep 10/100/1000 ms
              measured 330/3279/32771 ticks — ALL PASS)
  LETIMER-GPT PASS  (LETIMER0 as GPT, overflow IRQ + measured period)
  CRC         PASS  (GPCRC CRC32 of "123456789" = 0xCBF43926, byte/word fed,
              CRC16 smoke 0x6F91)
  BURAM       PASS  (backup RAM write/read/verify, survives)
  COMP        PASS  (ACMP0 compare both polarities + IRQ callback)
  DAC         PASS  (VDAC0 both channels, polled + IRQ completion)
  ADC         PASS  (IADC0 single conversions: GND vs GND -> code 0,
              PA0 vs GND completes, clock gated off after stop)
  LCD         PASS  (BRD2600A CE322-1002 glass via board_lcd_model/
              board_lcd_config: quadruplex mux, 1/3 bias, low-power wave,
              charge-pump mode, VLCD contrast 15; EN set, clear->SEGD=0,
              glass digit 8/0 checks, seg79->SEGD3=0x80000, icon P2->0x20,
              stop->CLKEN1.LCD=0; ends with ALL 80 segments on - watch LCD)
  TEMP        PASS  (generic hal_temperature driver over TEMPv1 LLD:
              TEMPD1, tempStart/tempReadCelsius/tempStop; EMU->TEMP die temp
              3 reads over 3 s ~33 C, FIRSTTEMPDONE=1, spread <= 3 C;
              correction polynomial of any degree via tempSetCorrection()
              in the upper layer, Horner on the measured value; default
              zero coeffs = no correction. Verified: { -2,1,0 }, { 0,1,0.01 },
              cubic { 0,1,0,1e-4 } exact, clear restores raw)
  KEYSCAN     PASS  (3x4 matrix, cols PA0/3/4/6 wired-and, rows PB0/1/3
              pull-up, unused rows 3-5 tied to row 2 per KEYSCAN_E301;
              SCANNED irqs counted, no phantom KEY, NOKEY status set,
              clock gated off after stop)
  SIGROUTE    PASS  (generic hal_sigroute API, SIGROUTED1: async ch0 = GPIO PA0
              routed to pin PA4: ASYNC_PEEK and pad follow PA0;
              prsSetLogic NOT_A FNSEL check; consumer route to
              LETIMER0_START; sw level/pulse; sync ch connect/disconnect;
              clock gated off after stop)
  PULSECAP    PASS  (PRS async ch4 sw-pulse -> PCNT0 S0IN (CNT++, never
              reset) + LETIMER0 START (FREE mode, PRS START while running
              is a no-op => no restart); BURTC COMP (1 s window, EM2) ->
              PRS ch2 (FNSEL A) -> LDMAXBAR DMAREQ0 -> LDMA ch0
              (REQSEL LDMAXBARPRSREQ0) copies PCNT0->CNT into a circular
              linked-descriptor ring buffer; DONE irq commits the slot +
              signals the app. Window sample == CNT (8) PASS. PCNT
              overflow deliberately does NOT trigger DMA (would
              double-count pulses around a wrap together with the next
              window sample): overflow fires its IRQ, no ring entry
              appears, and the next window sample spans the wrap with
              the correct 16-bit monotonic delta PASS)

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
  - Config-style registers (IADC CFG/SINGLEFIFOCFG/SCANFIFOCFG, GPCRC,
    VDAC CFG/CHxCFG, ACMP INPUTCTRL, TIMER CCx_CFG, LESENSE CFG/TIMCTRL...)
    bus-fault when written while the module is EN=1 — write them while
    EN=0, enable last; after EN_CLR wait EN_DISABLING + STATUS SYNCBUSY.
  - Reading a peripheral whose CMU CLKEN bit is cleared bus-faults —
    check the CLKEN bit, not the peripheral STATUS, after stop().
  - LESENSE TIMCTRL: LFPRESC is 3 bits (shift 4), STARTDLY only 2 bits
    (shift 22); f_scan = f_LF / 2^(LFPRESC+STARTDLY) per channel.
  - dmaStreamAllocI() must not be called from plain thread context
    (osalDbgCheckClassI) — set the driver's DMA stream to -1 for IRQ-only
    use (see EFR32_LESENSE_DMA_STREAM in the test mcuconf.h).
  - LF-domain (low-power) peripherals — LETIMER, BURTC, SYSRTC, WDOG,
    PCNT/QEI, LESENSE — must be waited on via their SYNCBUSY register after
    register/CMD/EN changes (writing CMD while SYNCBUSY is set bus-faults);
    the LLDs do this (LESENSE start/scan/stop included). HF-domain
    peripherals instead use EN_DISABLING + STATUS SYNCBUSY. CMU itself has
    no SYNCBUSY on xG23. Testhal tests rely on the LLD waits and do not poll
    SYNCBUSY directly.
  - PA1 = SWCLK, PA2 = SWDIO on xG23 — never drive them as GPIO/peripheral
    outputs or the debug connection dies (looks like an unhaltable CPU).
    Avoid PA5 (BTN1), PB2 (LED), PA8/PA9 (VCOM) in tests too.
  - Peripheral-driven output pins need a pad MODE *_ALT variant (e.g.
    PUSHPULL_ALT) — with plain PUSHPULL the GPIO DOUT wins and the
    peripheral signal never reaches the pin (SIGROUTE/LCD/KEYSCAN routes).
  - LCD needs its clock: CMU CLKEN0.LFRCO + LCDCLKCTRL.CLKSEL=LFRCO
    (lcd_lld_start does both) and segment outputs allocated via
    GPIO->LCDSEG/LCDCOM; otherwise the panel stays blank while SEGD
    registers still read back fine. Blank panel cause found: BIASCTRL
    must get MODE=CHARGEPUMP and a VLCD (contrast) level - VLCD=0 blanks
    the glass; DISPCTRL BIAS (1/3 for the 4-COM glass) also required.
    SEGDn bit b = segment line b, SEGDn index = COM line.
  - LDMA on xG23: CHEN reads back 0 (real enable status is CHSTATUS);
    CH LINK only takes effect via LINKLOAD (point LINK at the first
    descriptor, then LINKLOAD with a channel MASK); the channel
    auto-disables after each descriptor DONE - re-enable with CHEN_SET
    to keep a circular linked list running; BLOCKSIZE field is 2^n
    transfers per request (BLOCKSIZE(0) = 1 word).
  - PRS async producers: GPIO and BURTC events pulse fine; LETIMER
    CH0/CH1 compare events and PCNT0 UFOF (when PCNT is clocked by the
    S0 PRS signal) do NOT pulse async PRS on xG23 - use BURTC COMP as
    the EM2 window timer and translate PCNT overflow via its IRQ.
  - PCNT in external-clock mode (CLKSEL=PCNTS0) syncs its LF register
    domain on S0 edges: pump SYNCBUSY with PRS SWPULSE on the S0 channel
    and issue CMD STARTCNT (done in qei_lld_start), else CNT never moves
    and PCNT writes stall the bus.
  - LETIMER PRS start/stop/clear are gated by LETIMER0->PRSMODE
    (default NONE = disabled).
  - KEYSCAN: COLOUT pads = WIREDAND+DOUT1, ROWSENSE pads = INPUTPULL+DOUT1;
    unused rows must be routed to a used row pin (KEYSCAN_E301 errata),
    otherwise they float and produce phantom key presses.
  - Stack sizes: tests use 0x1000 process/exceptions stacks (snprintf %f
    needs more than the 0x400 default).
