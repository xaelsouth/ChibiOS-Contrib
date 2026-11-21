/*
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
*/

/**
 * @file    devices_lib/nand/nand_lfs.c
 * @brief   LittleFS-over-GPMI-NAND binding code.
 *
 * @details The lfs_config callbacks translate littlefs block/offset accesses
 *          into the MTD-style logical block device of the NAND driver:
 *          read  -> imx23_nand_lread()  (ECC-corrected page reads)
 *          prog  -> imx23_nand_lwrite_page() per page (BCH encode)
 *          erase -> imx23_nand_lerase()  (NAND erase block, bad-block aware)
 *
 *          read_size = prog_size = NAND page size, block_size = NAND erase
 *          block size, so every littlefs prog is exactly one full NAND page.
 *
 * @addtogroup NAND
 * @{
 */

#include "hal.h"
#include "nand_lfs.h"

#include <string.h>

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/* littlefs hooks, declared by nand_lfs_config.h (LFS_CONFIG). Memory comes
   from the constant-time TLSF heap when USE_TLSF_HEAP is defined (see
   os/various/tlsf_bindings), otherwise from the ChibiOS first-fit heap.
   CRC is the standard littlefs table. */
#if defined(USE_TLSF_HEAP)
#include "tlsf_bindings.h"
#endif

void *lfs_malloc(size_t size) {

#if defined(USE_TLSF_HEAP)
  return tlsfHeapAlloc(size);
#else
  return chHeapAlloc(NULL, size);
#endif
}

void lfs_free(void *p) {

  if (p != NULL) {
#if defined(USE_TLSF_HEAP)
    tlsfHeapFree(p);
#else
    chHeapFree(p);
#endif
  }
}

uint32_t lfs_crc(uint32_t crc, const void *buffer, size_t size) {
  static const uint32_t rtable[16] = {0x00000000, 0x1db71064, 0x3b6e20c8,
                                      0x26d930ac, 0x76dc4190, 0x6b6b51f4,
                                      0x4db26158, 0x5005713c, 0xedb88320,
                                      0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
                                      0x9b64c2b0, 0x86d3d2d4, 0xa00ae278,
                                      0xbdbdf21c, };
  const uint8_t *data = (const uint8_t *)buffer;

  for (size_t i = 0; i < size; i++) {
    crc = (crc >> 4) ^ rtable[(crc ^ (data[i] >> 0)) & 0xf];
    crc = (crc >> 4) ^ rtable[(crc ^ (data[i] >> 4)) & 0xf];
  }

  return crc;
}

static int lfs_nand_read(const struct lfs_config *c, lfs_block_t block,
                         lfs_off_t off, void *buffer, lfs_size_t size) {
  nand_lfs_t *nl = (nand_lfs_t *)c->context;

  if (imx23_nand_lread(nl->np, nl->base_lblock + block, off, buffer, size) !=
      IMX23_NAND_OK) {
    return LFS_ERR_IO;
  }
  return 0;
}

static int lfs_nand_prog(const struct lfs_config *c, lfs_block_t block,
                         lfs_off_t off, const void *buffer, lfs_size_t size) {
  nand_lfs_t *nl = (nand_lfs_t *)c->context;
  const imx23_nand_geo_t *g = imx23_nand_geo(nl->np);
  uint32_t lblock = nl->base_lblock + block;
  lfs_off_t o = off;
  lfs_size_t s = size;
  const uint8_t *p = (const uint8_t *)buffer;

  /* prog_size == page size, so prog calls are whole pages. */
  while (s > 0U) {
    uint32_t page = (uint32_t)(o / g->page_size);
    uint32_t len  = g->page_size;

    if (len > s) {
      len = s;
    }
    if (imx23_nand_lwrite_page(nl->np, lblock, page, p, len) !=
        IMX23_NAND_OK) {
      return LFS_ERR_IO;
    }
    p += len;
    o += len;
    s -= len;
  }
  return 0;
}

static int lfs_nand_erase(const struct lfs_config *c, lfs_block_t block) {
  nand_lfs_t *nl = (nand_lfs_t *)c->context;

  if (imx23_nand_lerase(nl->np, nl->base_lblock + block) != IMX23_NAND_OK) {
    return LFS_ERR_IO;
  }
  return 0;
}

static int lfs_nand_sync(const struct lfs_config *c) {

  (void)c;
  return 0;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Mounts a LittleFS volume on the NAND logical block device.
 *
 * @param[in] nl              binding object
 * @param[in] np              initialized NAND driver
 * @param[in] base_lblock     first logical block reserved for the volume
 * @param[in] format_on_fail  if true, format the volume when mount fails
 *
 * @return  0 on success, littlefs error code otherwise.
 */
int nand_lfs_mount(nand_lfs_t *nl, IMX23NANDDriver *np,
                   uint32_t base_lblock, bool format_on_fail) {
  const imx23_nand_geo_t *g = imx23_nand_geo(np);
  uint32_t lblocks = imx23_nand_lblocks(np);
  int rc;

  osalDbgCheck(nl != NULL && np != NULL);
  osalDbgCheck(np->ready);
  osalDbgCheck(base_lblock < lblocks);

  memset(nl, 0, sizeof(*nl));
  nl->np = np;
  nl->base_lblock = base_lblock;

  nl->cfg.context         = nl;
  nl->cfg.read_size       = g->page_size;
  nl->cfg.prog_size       = g->page_size;
  nl->cfg.block_size      = g->block_size;
  nl->cfg.block_count     = lblocks - base_lblock;
  nl->cfg.cache_size      = g->page_size;
  nl->cfg.lookahead_size  = sizeof(nl->lookahead);
  nl->cfg.read_buffer     = nl->read_buf;
  nl->cfg.prog_buffer     = nl->prog_buf;
  nl->cfg.lookahead_buffer = nl->lookahead;
  nl->cfg.read            = lfs_nand_read;
  nl->cfg.prog            = lfs_nand_prog;
  nl->cfg.erase           = lfs_nand_erase;
  nl->cfg.sync            = lfs_nand_sync;

  rc = lfs_mount(&nl->lfs, &nl->cfg);
  if (rc != LFS_ERR_OK && format_on_fail) {
    rc = lfs_format(&nl->lfs, &nl->cfg);
    if (rc == LFS_ERR_OK) {
      rc = lfs_mount(&nl->lfs, &nl->cfg);
    }
  }
  if (rc == LFS_ERR_OK) {
    nl->mounted = true;
  }
  return rc;
}

int nand_lfs_unmount(nand_lfs_t *nl) {
  int rc = lfs_unmount(&nl->lfs);

  if (rc == LFS_ERR_OK) {
    nl->mounted = false;
  }
  return rc;
}

lfs_t *nand_lfs_handle(nand_lfs_t *nl) {

  return &nl->lfs;
}

/** @} */
