# RT-IMX23-BOOTLOADER — failsafe NAND bootloader demo

Two-level failsafe boot for the i.MX23 EVK:

1. **ROM level** — the BootROM loads the bootloader's `.sb` bootstream from
   the NAND boot area (NCB/LDLB/DBBT + SB image, written by the
   `RT-IMX23-HELLO` demo's `nand bootwrite /bootloader.sb` command; build
   the `.sb` in `RT-IMX23-HELLO` with `make elftosb BOARD=IMX23_EVK`, it has
   `tools/` + `bootlets/`). Dual bootstream slots (blocks 24..39 / 40..47)
   are handled there via `nand bootslot`.
2. **Bootloader level (this demo)** — runs at 0x40000000, mounts the NAND
   LittleFS volume, reads the active slot from `/boot/slot`, loads
   `/boot/app<slot>.bin` to `0x41000000`, verifies its SHA-256 with the
   DCP (`mxs_dcp_sha256`) against `/boot/app<slot>.sha256`, and jumps.
   On failure it tries the other slot; if both fail it stays in the shell.

App images are flat binaries linked at `0x41000000`
(`arm-none-eabi-objcopy -O binary app.elf app.bin`).

## Shell commands (DUART, 115200 8N1)

    info                                  NAND/LFS/slot status
    mount [format]                        mount (or format+mount) LFS
    ls [path]                             list LFS
    slot [0|1]                            show / set active slot
    install <lfs-image.bin> [lfs-sha.txt] copy image into the INACTIVE slot
                                          and verify the written copy
    boot                                  boot active slot now (fallback)

## Usage

    # on the HELLO demo (has tools/bootlets):
    make elftosb BOARD=IMX23_EVK          # -> build/IMX23_EVK/ch.sb (rename bootloader.sb)
    nand bootwrite /bootloader.sb         # write ROM boot area
    # upload an app image + its sha256 (hex, 64 chars) to LFS (TFTP/HTTPS),
    # then in the bootloader shell:
    install /update.bin /update.sha256    # writes /boot/app<other>.bin{,.sha256}
    slot 1                                # activate it
    boot                                  # or just reset

Build: `make -j8 BOARD=IMX23_EVK` (or `BOARD=IMX233_OLINUXINO_MINI`).
