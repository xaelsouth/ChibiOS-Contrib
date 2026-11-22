*****************************************************************************
** ChibiOS/RT standard demo for the SILABS BRD2600A REV A03 radio board     **
** (EFR32FG23B010F512IM48, radio board of the FG23 DK2600A dev kit).        **
*****************************************************************************

** TARGET **

The demo runs on a BRD2600A REV A03 (e.g. plugged into a DK2600A kit).

** FEATURES **

- Serial console on the J-Link VCOM: SIOD1 (EUSART0) on PA9=TX / PA10=RX,
  SIO_DEFAULT_BITRATE (38400 unless overridden in cfg/halconf.h), 8N1.
  Connect a serial terminal to the kit's USB VCOM.
- LED0 (PB2) blinks: 1 s on, 1 s off.
- Press BTN0 (PB1) to execute the ChibiOS RT and OSLIB test suites on the
  serial console.
- Press BTN1 (PA5) to print a status line (board name, uptime).

** BUILD **

    make
    make flash        # via J-Link SWD

** NOTES **

Some files used by the demo are not part of ChibiOS/RT but are copyright of
Silicon Labs and are licensed under a different license.
                             https://www.silabs.com
