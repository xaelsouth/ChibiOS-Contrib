# Bootlets / .sb bootstream for the i.MX23 ROM USB boot

`make elftosb BOARD=IMX23_EVK` builds `build/IMX23_EVK/ch.sb` from the ELF
and prints the command to push it to a board in USB HID recovery mode:

    make elftosb BOARD=IMX23_EVK
    # then, with the EVK in USB recovery mode (no SD card / boot pins set):
    sudo tools/sb_loader -d -p hid build/IMX23_EVK/ch.sb

Layout:

    bootlets/<BOARD>/bootlets.bd   elftosb command template (%CHIBIOS_ELF%
                                   is replaced at build time with the
                                   absolute path of build/<BOARD>/ch.elf)
    bootlets/<BOARD>/power_prep    prebuilt bootlet: power supply init
    bootlets/<BOARD>/boot_prep     prebuilt bootlet: SDRAM init

The template loads the bootlets, then loads the ChibiOS ELF at its link
address (IMX23_RAM_ORIGIN = 0x40000000) and jumps to 0x40000000 — the
startup section there is the vector table, whose reset slot forwards to
the reset code, so this enters the image correctly.

The bootlets themselves are prebuilt from the mxs-bootlets 10.12.01
sources (same as buildroot's imx-kobs target uses); rebuild them with
kobs-ng if needed.
