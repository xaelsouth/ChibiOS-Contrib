testhal/SILABS/EFR32FG23x0xx - ChibiOS HAL test suite for Silicon Labs EFR32FG23
=============================================================================
Target board: EFR32FG23 DK2600A (BRD2600A, EFR32FG23B010F512IM48)
Connections used (BRD2600A):
  LED0  = PB2          BTN0 = PB1, BTN1 = PA5
  VCOM  = EUSART PA9 (TX) / PA10 (RX), 115200 8N1  -> /dev/ttyACM0
  Sensor I2C bus = I2C0 SCL=PA8 / SDA=PA7
  Optical/wMBus port = EUSART0 on PA9/PA10 (loopback tests)

System tick: SYSTICKv2 (SYSRTC0 @ 32768 Hz). RTC calendar: BURTC (RTCD1).

Tests: TEMPLATE GPIO SIO RTC SYSRTC GPT-LETIMER PWM-TIMER I2C SPI DMA
       EFL CRYPTO-AES WDG RAIL-WMBUS

Build:  cd <test> && make -j4
Flash:  make flash        (J-Link SWD)
Console: screen /dev/ttyACM0 115200  (or: cat /dev/ttyACM0)
