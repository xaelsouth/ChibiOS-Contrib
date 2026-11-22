*****************************************************************************
** ChibiOS/RT standard demo for the SILABS BRD4001A REV A01 radio board      **
** (EFR32FG23A010F256GM48, 868-915 MHz +14 dBm WSTK radio board).            **
*****************************************************************************

** TARGET **

The demo runs on a BRD4001A REV A01 radio board (e.g. plugged into a WSTK
mainboard).

** FEATURES **

- Serial console: SIOD4 (USART0) on PC2=TX / PC1=RX, SIO_DEFAULT_BITRATE
  (38400 unless overridden in cfg/halconf.h), 8N1. Typed characters are
  echoed back.
- The BURTC-based RTC time is printed on the console once per second.

** BUILD **

    make
    make program      # via J-Link SWD (see post-build.mk)
    make start_gdb    # J-Link GDB server

** NOTES **

Some files used by the demo are not part of ChibiOS/RT but are copyright of
Silicon Labs and are licensed under a different license. Also note that not
all the files present in the emlib library are distributed with ChibiOS/RT,
you can find the whole library on the Silicon Labs web site:

                             https://www.silabs.com
