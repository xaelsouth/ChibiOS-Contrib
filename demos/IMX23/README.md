<!--
    ChibiOS - Copyright (C) 2026 Xael South.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
-->

# ChibiOS/RT on NXP i.MX23 (ARM926EJ-S)

Branches: `imx23` in both `ChibiOS` (master) and `ChibiOS-Contrib` (chibios-21.11.x).

Boot model: the ROM "bootstream" (USB boot) does power + DRAM init, copies the
`.bin` to `0x40000000` and CALLs it. `__early_init()` copies the 8 exception
vectors into OCRAM at `0x0`, disables MMU/caches and clears the CP15 V bit.

## Layout

- `ChibiOS/os/common/startup/ARM/devices/IMX23/` — armparams, `__early_init`, mem map
- `ChibiOS/os/common/startup/ARM/compilers/GCC/{mk/startup_imx23.mk,ld/IMX23.ld}` —
  the linker script is cpp-preprocessed with the board's `board.h`
- `ChibiOS-Contrib/os/hal/ports/IMX23/IMX23xx/` — platform:
  - `imx23.h` all register maps, `icoll.*` interrupt controller (+ `icoll_tramp.S`)
  - `hal_lld`, `hal_st_lld` (TIMROT 32kHz), `hal_pal_lld` (pinctrl),
    `hal_sio_lld` (DUART PL011 + AUART0/1), `hal_spi_lld` (SSP0/1, PIO),
    `hal_i2c_lld` (I2C0 PIO master), `hal_pwm_lld` (5 ch, 24MHz),
    `imx23_dma` (APBH/APBX CCW engine),
    `imx23_clock` (CPU 480/240/120 MHz + VDDD scaling),
    `imx23_lcdif` (LCDIF dotclock master, XRGB8888 fb, vsync IRQ),
    `imx23_pxp` (pixel pipeline blit/CSC, IRQ 57),
    `imx23_audioout` (mxs-builtin DAC playback, APBX DMA ch1)
- `ChibiOS-Contrib/os/hal/boards/IMX23_OLIMEX_IMX233/` — 64MB, `IMX23_RAM_SIZE 0x4000000`
- `ChibiOS-Contrib/os/hal/boards/IMX23_EVK/` — 128MB, LCD panel + ENC28J60 defines
- `ChibiOS-Contrib/os/various/devices_lib/ethernet/enc28j60_lld.c` — SPI MAC LLD
- `ChibiOS-Contrib/demos/IMX23/RT-IMX23-HELLO/` — demo with xshell on the DUART

RAM size: one `#define IMX23_RAM_SIZE` in the board's `board.h`
(`0x4000000` OLIMEX / `0x8000000` EVK); `heap_end` is 0x44000000 / 0x48000000.
OCRAM (32KB at 0x0) is a second linker region (`ram1`) for vectors/stacks.

## Build

```sh
export PATH=/opt/arm-gnu-toolchain-12.3.rel1-x86_64-arm-none-eabi/bin:$PATH
cd ChibiOS-Contrib/demos/IMX23/RT-IMX23-HELLO
make -j8 BOARD=IMX23_OLIMEX_IMX233   # or BOARD=IMX23_EVK
# -> build/<BOARD>/ch.bin  (load at 0x40000000 via bootstream)
```

## Demo shell commands (DUART, 115200 8N1)

- `freq [0|1|2]` — CPU profile 480/240/120 MHz (+VDDD scale)
- `backlight <0-100>` — EVK LCD backlight (PWM2, GPIO_1_28)
- `pxp` — PXP blit test on the EVK framebuffer
- `tone [hz] [ms]` — sine on the mxs-builtin DAC
- `mac` — ENC28J60 link status (EVK)
- `info`, `systime`, `mem`, `echo` — built-in
- Log lines are prefixed with `[sec.msec]` timestamps.

## NAND / filesystem recommendation

Use **LittleFS** (`os/various/littlefs_bindings`) on top of the contrib
`hal_nand` block API — but insert a thin MTD-style **bad-block-skipping block
device** between hal_nand (GPMI+BCH via APBH DMA) and LittleFS: LittleFS only
wear-levels good blocks, bad blocks must be skipped one level below (that is
not a "transcribe" method — it is standard MTD bad-block management reading
the manufacturer's bad-block marks in the spare area).
