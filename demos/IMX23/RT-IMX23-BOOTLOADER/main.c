/*
    ChibiOS - Copyright (C) 2026 Xael South

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    RT-IMX23-BOOTLOADER/main.c
 * @brief   Failsafe NAND bootloader demo for the i.MX23 EVK.
 *
 * @details Boot flow:
 *          1. The BootROM loads this bootloader from the NAND boot area
 *             (NCB/LDLB/DBBT + bootstream .sb written by the RT-IMX23-HELLO
 *             demo's "nand bootwrite /bootloader.sb" command; build the .sb
 *             there with "make elftosb", it has tools/ + bootlets/).
 *          2. The bootloader mounts the NAND LittleFS volume and reads the
 *             active slot from /boot/slot (0 or 1).
 *          3. It loads /boot/app<slot>.bin to APP_LOAD_ADDR, verifies its
 *             SHA-256 (mxs_dcp) against /boot/app<slot>.sha256, and jumps.
 *          4. On any failure it tries the other slot; if both fail it
 *             stays in the shell.
 *
 *          App images are plain flat binaries linked at APP_LOAD_ADDR
 *          (0x41000000), e.g. produced from an ELF with objcopy -O binary.
 *
 *          Shell commands (DUART, 115200 8N1):
 *            info                 NAND/LFS/slot status
 *            mount [format]       mount (or format+mount) the LFS volume
 *            ls [path]            list LFS
 *            slot [0|1]           show/set active slot (/boot/slot)
 *            install <lfs.bin> [lfs.sha256]
 *                                 copy the image into the INACTIVE slot
 *                                 (/boot/app<other>.bin + .sha256), verify
 *                                 it by hashing the written copy
 *            boot                 boot the active slot now (with fallback)
 */

#include <string.h>
#include <stdlib.h>

#include "hal.h"
#include "chprintf.h"
#include "shell.h"

#include "imx23_gpmi_nand.h"
#include "imx23_romboot.h"
#include "nand_lfs.h"
#include "mxs_dcp.h"
#include "tlsf_bindings.h"

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/* Where app images are loaded and entered (must match the app's link addr). */
#define APP_LOAD_ADDR       0x41000000U
#define APP_MAX_SIZE        (4U * 1024U * 1024U)

#define BOOT_DIR            "/boot"
#define SLOT_FILE           "/boot/slot"

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

static const SIOConfig sio_console = {
  .baud     = 115200U,
  .framecfg = IMX23_SIO_LLD_8N1,
};

static bool nand_ready;
static nand_lfs_t nand_lfs;

/*===========================================================================*/
/* NAND + LittleFS plumbing (same binding as RT-IMX23-HELLO).                */
/*===========================================================================*/

static uint32_t lfs_base_lblock(void) {

  return imx23_romboot_lfs_base(&IMX23NAND1, IMX23_ROMBOOT_RESERVE_BLOCKS);
}

static uint32_t nand_bd_lblocks(void *ctx) {

  return imx23_nand_lblocks((IMX23NANDDriver *)ctx);
}

static int nand_bd_lread(void *ctx, uint32_t lblock, uint32_t off,
                         uint8_t *buf, uint32_t len) {

  return imx23_nand_lread((IMX23NANDDriver *)ctx, lblock, off, buf, len);
}

static int nand_bd_lwrite_page(void *ctx, uint32_t lblock, uint32_t page,
                               const uint8_t *data, uint32_t len) {

  return imx23_nand_lwrite_page((IMX23NANDDriver *)ctx, lblock, page, data,
                                len);
}

static int nand_bd_lerase(void *ctx, uint32_t lblock) {

  return imx23_nand_lerase((IMX23NANDDriver *)ctx, lblock);
}

static nand_lfs_bd_t nand_bd = {
  .ctx         = &IMX23NAND1,
  .page_size   = 0U,
  .block_size  = 0U,
  .lblocks     = nand_bd_lblocks,
  .lread       = nand_bd_lread,
  .lwrite_page = nand_bd_lwrite_page,
  .lerase      = nand_bd_lerase,
};

static int nand_ensure(void) {

  if (!nand_ready) {
    const imx23_nand_geo_t *g;
    int rc = imx23_nand_init(&IMX23NAND1, 0U);
    if (rc != IMX23_NAND_OK) {
      return rc;
    }
    g = imx23_nand_geo(&IMX23NAND1);
    nand_bd.page_size  = g->page_size;
    nand_bd.block_size = g->block_size;
    nand_ready = true;
  }
  return IMX23_NAND_OK;
}

static int lfs_ensure(BaseSequentialStream *chp, bool format) {
  int rc;

  if (nand_lfs.mounted) {
    return LFS_ERR_OK;
  }
  rc = nand_ensure();
  if (rc != IMX23_NAND_OK) {
    if (chp != NULL) {
      chprintf(chp, "nand init failed: %d\r\n", rc);
    }
    return rc;
  }
  rc = nand_lfs_mount(&nand_lfs, &nand_bd, lfs_base_lblock(), format);
  if (chp != NULL) {
    chprintf(chp, "lfs mount: %s (%d)\r\n",
             rc == LFS_ERR_OK ? "OK" : "FAILED", rc);
  }
  return rc;
}

/*===========================================================================*/
/* Boot logic.                                                               */
/*===========================================================================*/

static int read_slot(void) {
  lfs_file_t f;
  char buf[8U];
  lfs_ssize_t n;
  int slot = 0;

  if (lfs_file_open(&nand_lfs.lfs, &f, SLOT_FILE, LFS_O_RDONLY) ==
      LFS_ERR_OK) {
    n = lfs_file_read(&nand_lfs.lfs, &f, buf, sizeof(buf) - 1U);
    lfs_file_close(&nand_lfs.lfs, &f);
    if (n > 0U) {
      buf[n] = '\0';
      slot = (int)strtoul(buf, NULL, 10);
    }
  }
  return (slot == 1) ? 1 : 0;
}

static int write_slot(BaseSequentialStream *chp, int slot) {
  lfs_file_t f;
  char buf[8U];
  int rc;

  rc = lfs_file_open(&nand_lfs.lfs, &f, SLOT_FILE,
                     LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
  if (rc != LFS_ERR_OK) {
    return rc;
  }
  snprintf(buf, sizeof(buf), "%d\n", slot);
  rc = lfs_file_write(&nand_lfs.lfs, &f, buf, strlen(buf));
  lfs_file_close(&nand_lfs.lfs, &f);
  if (chp != NULL) {
    chprintf(chp, "active slot := %d\r\n", slot);
  }
  return (rc > 0) ? LFS_ERR_OK : LFS_ERR_IO;
}

/* Load /boot/app<slot>.bin to APP_LOAD_ADDR and verify its SHA-256 against
   /boot/app<slot>.sha256 (hex text). Returns LFS_ERR_OK on verified load. */
static int boot_app(BaseSequentialStream *chp, int slot) {
  char path[64U];
  char sha_path[64U];
  lfs_file_t fb, fs;
  uint8_t expect[32U];
  uint8_t got[MXS_DCP_SHA256_LEN];
  char hex[65U];
  uint8_t chunk[4096U];
  lfs_ssize_t n;
  uint32_t total = 0U;
  unsigned i;
  int rc;

  snprintf(path, sizeof(path), "%s/app%d.bin", BOOT_DIR, slot);
  snprintf(sha_path, sizeof(sha_path), "%s/app%d.sha256", BOOT_DIR, slot);

  if (lfs_file_open(&nand_lfs.lfs, &fs, sha_path, LFS_O_RDONLY) !=
      LFS_ERR_OK) {
    if (chp != NULL) {
      chprintf(chp, "slot %d: no %s\r\n", slot, sha_path);
    }
    return LFS_ERR_NOENT;
  }
  n = lfs_file_read(&nand_lfs.lfs, &fs, hex, 64U);
  lfs_file_close(&nand_lfs.lfs, &fs);
  if (n < 64U) {
    if (chp != NULL) {
      chprintf(chp, "slot %d: bad sha256 file\r\n", slot);
    }
    return LFS_ERR_IO;
  }
  hex[64U] = '\0';
  for (i = 0U; i < 32U; i++) {
    unsigned v;
    if (sscanf(hex + i * 2U, "%2x", &v) != 1) {
      return LFS_ERR_IO;
    }
    expect[i] = (uint8_t)v;
  }

  if (lfs_file_open(&nand_lfs.lfs, &fb, path, LFS_O_RDONLY) != LFS_ERR_OK) {
    if (chp != NULL) {
      chprintf(chp, "slot %d: no %s\r\n", slot, path);
    }
    return LFS_ERR_NOENT;
  }
  if (lfs_file_size(&nand_lfs.lfs, &fb) > APP_MAX_SIZE) {
    lfs_file_close(&nand_lfs.lfs, &fb);
    if (chp != NULL) {
      chprintf(chp, "slot %d: image too big\r\n", slot);
    }
    return LFS_ERR_IO;
  }
  while ((n = lfs_file_read(&nand_lfs.lfs, &fb, chunk, sizeof(chunk))) > 0) {
    memcpy((void *)(uintptr_t)(APP_LOAD_ADDR + total), chunk, (size_t)n);
    total += (uint32_t)n;
  }
  lfs_file_close(&nand_lfs.lfs, &fb);
  if (n < 0 || total == 0U) {
    return LFS_ERR_IO;
  }

  rc = mxs_dcp_sha256(&MXSDCP1, (const uint8_t *)(uintptr_t)APP_LOAD_ADDR,
                      total, got);
  if (rc != MXS_DCP_OK) {
    return LFS_ERR_IO;
  }
  if (memcmp(got, expect, 32U) != 0U) {
    if (chp != NULL) {
      chprintf(chp, "slot %d: SHA-256 MISMATCH\r\n", slot);
    }
    return LFS_ERR_IO;
  }
  if (chp != NULL) {
    chprintf(chp, "slot %d: %s verified (%lu bytes), jumping to 0x%08lx\r\n",
             slot, path, (unsigned long)total,
             (unsigned long)APP_LOAD_ADDR);
    sioSynchronizeTXEnd(&SIOD1, TIME_INFINITE);
  }
  ((void (*)(void))(uintptr_t)APP_LOAD_ADDR)();
  return LFS_ERR_IO;    /* app returned?! */
}

/* Try the active slot, then the other one. */
static void boot_with_fallback(BaseSequentialStream *chp) {
  int slot = read_slot();

  if (boot_app(chp, slot) == LFS_ERR_OK) {
    return;             /* does not return on success */
  }
  chprintf(chp, "active slot %d failed, trying other slot\r\n", slot);
  boot_app(chp, slot ^ 1);
  chprintf(chp, "both slots failed - staying in bootloader shell\r\n");
}

/*===========================================================================*/
/* Shell commands.                                                           */
/*===========================================================================*/

static void cmd_info(BaseSequentialStream *chp, int argc, char *argv[]) {
  const imx23_nand_geo_t *g;
  (void)argc; (void)argv;

  if (nand_ensure() == IMX23_NAND_OK) {
    g = imx23_nand_geo(&IMX23NAND1);
    chprintf(chp, "NAND: %s, page %lu + oob %lu, block %lu, %lu blocks (%lu good)\r\n",
             g->name, (unsigned long)g->page_size, (unsigned long)g->oob_size,
             (unsigned long)g->block_size, (unsigned long)g->blocks,
             (unsigned long)g->good_blocks);
  }
  chprintf(chp, "lfs: %s, base lblock %lu\r\n",
           nand_lfs.mounted ? "mounted" : "not mounted",
           (unsigned long)lfs_base_lblock());
  if (nand_lfs.mounted) {
    chprintf(chp, "active slot: %d\r\n", read_slot());
  }
}

static void cmd_mount(BaseSequentialStream *chp, int argc, char *argv[]) {

  lfs_ensure(chp, (argc >= 1U) && (strcmp(argv[0], "format") == 0));
}

static void cmd_ls(BaseSequentialStream *chp, int argc, char *argv[]) {
  lfs_dir_t dir;
  struct lfs_info info;
  const char *path = (argc >= 1U) ? argv[0] : "/";
  int rc;

  if (!nand_lfs.mounted) {
    chprintf(chp, "lfs not mounted\r\n");
    return;
  }
  rc = lfs_dir_open(&nand_lfs.lfs, &dir, path);
  if (rc != LFS_ERR_OK) {
    chprintf(chp, "ls %s: %d\r\n", path, rc);
    return;
  }
  while ((rc = lfs_dir_read(&nand_lfs.lfs, &dir, &info)) > 0) {
    if (info.name[0] == '.') {
      continue;
    }
    chprintf(chp, "%c %8lu %s\r\n",
             (info.type == LFS_TYPE_DIR) ? 'd' : '-',
             (unsigned long)info.size, info.name);
  }
  lfs_dir_close(&nand_lfs.lfs, &dir);
}

static void cmd_slot(BaseSequentialStream *chp, int argc, char *argv[]) {

  if (argc >= 1U) {
    int slot = (int)strtoul(argv[0], NULL, 10);
    if (slot != 0 && slot != 1) {
      chprintf(chp, "usage: slot [0|1]\r\n");
      return;
    }
    if (lfs_ensure(chp, false) != LFS_ERR_OK) {
      return;
    }
    write_slot(chp, slot);
    return;
  }
  if (!nand_lfs.mounted && lfs_ensure(chp, false) != LFS_ERR_OK) {
    return;
  }
  chprintf(chp, "active slot: %d\r\n", read_slot());
}

/* Copy an LFS file to another LFS file in 4 KiB chunks. */
static int lfs_copy(BaseSequentialStream *chp, const char *src,
                    const char *dst) {
  lfs_file_t fi, fo;
  uint8_t chunk[4096U];
  lfs_ssize_t n;
  uint32_t total = 0U;
  int rc;

  rc = lfs_file_open(&nand_lfs.lfs, &fi, src, LFS_O_RDONLY);
  if (rc != LFS_ERR_OK) {
    chprintf(chp, "install: cannot open %s (%d)\r\n", src, rc);
    return rc;
  }
  rc = lfs_file_open(&nand_lfs.lfs, &fo, dst,
                     LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
  if (rc != LFS_ERR_OK) {
    lfs_file_close(&nand_lfs.lfs, &fi);
    chprintf(chp, "install: cannot create %s (%d)\r\n", dst, rc);
    return rc;
  }
  while ((n = lfs_file_read(&nand_lfs.lfs, &fi, chunk, sizeof(chunk))) > 0) {
    if (lfs_file_write(&nand_lfs.lfs, &fo, chunk, (size_t)n) != n) {
      rc = LFS_ERR_IO;
      break;
    }
    total += (uint32_t)n;
  }
  lfs_file_close(&nand_lfs.lfs, &fo);
  lfs_file_close(&nand_lfs.lfs, &fi);
  if (rc == LFS_ERR_OK && n < 0) {
    rc = (int)n;
  }
  chprintf(chp, "install: %s -> %s, %lu bytes, rc=%d\r\n", src, dst,
           (unsigned long)total, rc);
  return rc;
}

static void cmd_install(BaseSequentialStream *chp, int argc, char *argv[]) {
  char bin_path[64U], sha_path[64U], hex[80U];
  int cur, dst;
  lfs_file_t f;
  lfs_ssize_t n;

  if (argc < 1U) {
    chprintf(chp, "usage: install <lfs-image.bin> [lfs-sha256.txt]\r\n");
    return;
  }
  if (lfs_ensure(chp, false) != LFS_ERR_OK) {
    return;
  }
  if (mxs_dcp_init(&MXSDCP1) != MXS_DCP_OK) {
    chprintf(chp, "dcp init failed\r\n");
    return;
  }
  cur = read_slot();
  dst = cur ^ 1;
  snprintf(bin_path, sizeof(bin_path), "%s/app%d.bin", BOOT_DIR, dst);
  snprintf(sha_path, sizeof(sha_path), "%s/app%d.sha256", BOOT_DIR, dst);

  if (lfs_copy(chp, argv[0], bin_path) != LFS_ERR_OK) {
    return;
  }
  /* Hash source: prefer the uploaded sha256 file, else compute from the
     freshly written copy (loaded at APP_LOAD_ADDR). */
  if (argc >= 2U &&
      lfs_file_open(&nand_lfs.lfs, &f, argv[1], LFS_O_RDONLY) == LFS_ERR_OK) {
    n = lfs_file_read(&nand_lfs.lfs, &f, hex, 64U);
    lfs_file_close(&nand_lfs.lfs, &f);
    if (n < 64U) {
      chprintf(chp, "install: bad sha256 file\r\n");
      return;
    }
    hex[64U] = '\0';
  }
  else {
    /* verify the written copy by loading and hashing it */
    if (boot_app(chp, dst) == LFS_ERR_OK) {
      return;               /* booted! */
    }
    chprintf(chp, "install: written copy failed verification\r\n");
    return;
  }
  if (lfs_file_open(&nand_lfs.lfs, &f, sha_path,
                    LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) != LFS_ERR_OK) {
    chprintf(chp, "install: cannot create %s\r\n", sha_path);
    return;
  }
  lfs_file_write(&nand_lfs.lfs, &f, hex, 64U);
  lfs_file_close(&nand_lfs.lfs, &f);
  chprintf(chp, "install: slot %d ready (%s + %s); 'slot %d' to activate\r\n",
           dst, bin_path, sha_path, dst);
}

static void cmd_boot(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argc; (void)argv;

  if (lfs_ensure(chp, false) != LFS_ERR_OK) {
    return;
  }
  if (mxs_dcp_init(&MXSDCP1) != MXS_DCP_OK) {
    chprintf(chp, "dcp init failed\r\n");
    return;
  }
  boot_with_fallback(chp);
}

static const ShellCommand commands[] = {
  { "info",    cmd_info    },
  { "mount",   cmd_mount   },
  { "ls",      cmd_ls      },
  { "slot",    cmd_slot    },
  { "install", cmd_install },
  { "boot",    cmd_boot    },
  { NULL, NULL }
};

static const ShellConfig shell_cfg = {
  .sc_channel  = (BaseSequentialStream *)&SIOD1,
  .sc_commands = commands,
};

static THD_WORKING_AREA(waShell, 2048U);

/*===========================================================================*/
/* Application entry point.                                                  */
/*===========================================================================*/

int main(void) {

  halInit();
  chSysInit();
  tlsfHeapInit();

  sioStart(&SIOD1, &sio_console);
  chprintf((BaseSequentialStream *)&SIOD1,
           "\r\n*** i.MX23 failsafe bootloader ***\r\n");

  shellInit();
  chThdCreateStatic(waShell, sizeof(waShell), NORMALPRIO + 1,
                    shellThread, (void *)&shell_cfg);

  /* Mount NAND LittleFS and try to boot the active app slot. If anything
     fails we stay here and the shell above is usable. */
  if (lfs_ensure((BaseSequentialStream *)&SIOD1, false) == LFS_ERR_OK) {
    if (mxs_dcp_init(&MXSDCP1) == MXS_DCP_OK) {
      boot_with_fallback((BaseSequentialStream *)&SIOD1);
    }
    else {
      chprintf((BaseSequentialStream *)&SIOD1, "dcp init failed\r\n");
    }
  }

  while (true) {
    chThdSleepMilliseconds(1000U);
  }
}
