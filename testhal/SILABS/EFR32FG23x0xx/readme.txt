EFR32FG23 testhal
================

These are individual ChibiOS peripheral test programs for the BRD2600A
(EFR32FG23B010F512IM48). They are not an automated certification suite:
some only test a subset of a driver, require external wiring or human
observation, or access persistent state. A build or a printed PASS does not
establish that every operation of the corresponding LLD works.

Build a test in its directory with "make -j4". Its Makefile's "flash" target
programs the board. The tests use RTT; attach an RTT viewer or read the
test's rtt_buf symbol with a debugger. The GPIO, debugger and board resource
assignments must be checked before running a test alongside other hardware.
PA1 and PA2 are SWCLK and SWDIO; do not reconfigure them as outputs.

Tests and coverage
------------------
ADC, COMP, DAC, LESENSE, LETIMER-GPT, SIGROUTE, PULSECAP, GPT-TIMER,
PWM-TIMER and GPIO cover portions of the analog, sensor, routing and timer
drivers. PULSECAP also exercises PCNT/QEI, DMA and ring-buffer handling.
The DAC test checks two CH0 output levels through the IADC internal input,
but does not measure absolute output voltage or verify DMA conversion groups.
The LESENSE test measures scan scheduling, not metal detection. ADC checks a
ground-to-ground conversion and a PA0 completion smoke test; the DAC test
also compares VBGR and VDDX references. External analog routing and
scan/streaming remain unverified.

SIO and SPI use EUSART0 internal loopback; neither requires an external
loopback wire. The I2C test uses the onboard Si7021 at 0x40 on PA7/PA8;
it does not scan or write to arbitrary bus addresses. GPIO button checks
and LCD visual checks require physical observation. PWM-TIMER samples the
PB2 input while driving duty cycles and checking channel disable and polarity.

DMA, CRYPTO-AES, CRC, TEMP, KEYSCAN, SYSRTC, RTC, SYSTICKv1 and SYSTICKv2
exercise their respective peripherals. Some standalone APIs and modes do not
have dedicated tests; inspect each test before assuming a particular path is
covered. There is no standalone ICU test.

Persistent state and side effects
---------------------------------
EFL erases and programs the USERDATA page, then attempts to restore and
verify its original contents. BURAM temporarily overwrites 16 bytes and
restores them. A reset or power failure during either test can lose data;
make an independent backup first. RTC and BURTC change retained calendar
state; BURTC also writes backup RAM and resets the device. PULSECAP resets
the BURTC counter and changes its configuration. WDG starts a watchdog,
which can survive reset. RAIL-TX transmits radio signals and requires a
suitable configuration and authorization. Tests can change pin routing and
overwrite the running firmware; restore the intended application afterwards.

Known limitations
-----------------
LESENSE coil excitation, thresholds and metal detection remain uncalibrated.
PULSECAP passed its injected-pulse and DMA checks, but restarting PCNT took
many seconds with the system locked on one run; it is not a latency test.
RTC calendar progression and a forced BURTC overflow were checked, but
calendar continuity through a warm reset has not been verified.
The ADC LLD does not implement the ChibiOS scan/streaming conversion contract.
The DAC DMA conversion-group implementation does not satisfy the ChibiOS
circular conversion and callback contract. LESENSE's use of ACMP and VDAC
is not arbitrated with the COMP and DAC drivers. SPI internal loopback has
checked 100-byte DMA send followed by receive, as well as full-duplex
exchange; other sizes, modes and DMA users need separate validation. The
polled DMA completion wait has no timeout for a missing request or DMA error.
The TIMER GPT/PWM prescaler selection rounds to powers of two and may not
produce the requested counter frequency. RAIL-TX has reported packet-sent
events but RF output and reception have not been independently measured.
Its SYSCLK uses HFXO to match the generated 39 MHz radio configuration.
LCD register checks do not prove that the glass is displaying correctly;
the LCD DMA update entry point currently uses CPU writes. On BRD2600A,
LCD SEG10 (PA4, digit 5A-5D) shares JTAG TDI. Its default pull-up prevents
those segments from displaying; the LCD test releases TDI after six seconds
while leaving SWD enabled. Debug routing cannot be changed while a debugger
is attached, so disconnect it before the test attempts the release. A reset
restores the JTAG pin routing.

Clock, register and pin requirements depend on the exact part, board and
energy mode. Consult the EFR32xG23 reference manual and the BRD2600A
schematic instead of treating previous test results as hardware guarantees.
