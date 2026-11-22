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
 * @file    devices_lib/nand/nand_lfs.h
 * @brief   LittleFS over a generic MTD-style NAND logical block device.
 *
 * @details Mounts a LittleFS volume on any NAND driver that exposes an
 *          MTD-style logical block interface (bad blocks skipped, ECC handled
 *          by the controller) through the @p nand_lfs_bd_t operations table:
 *          - lread:       ECC-corrected read of @p len bytes at offset @p off
 *                         inside logical block @p lblock
 *          - lwrite_page: program page @p page of logical block @p lblock
 *          - lerase:      erase logical block @p lblock
 *          - lblocks:     total number of logical blocks
 *          All return 0 on success.
 *
 *          read_size = prog_size = NAND page size, block_size = NAND erase
 *          block size, so every littlefs prog is exactly one full NAND page.
 *
 *          Build littlefs with -DLFS_THREADSAFE=0 and add ext/littlefs
 *          (lfs.c, lfs_util.c) plus this directory to the build.
 *
 * @addtogroup NAND
 * @{
 */

#ifndef NAND_LFS_H
#define NAND_LFS_H

#include "hal.h"
#include "nand_chips.h"
#include "lfs.h"

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   MTD-style NAND logical block device operations for LittleFS.
 * @note    All callbacks receive @p ctx (the first field) as their first
 *          argument and return 0 on success, negative on error.
 */
typedef struct {
  void     *ctx;          /**< backing device context (e.g. driver object)  */
  uint32_t  page_size;    /**< NAND page (main area) size in bytes          */
  uint32_t  block_size;   /**< NAND erase block size in bytes               */
  uint32_t (*lblocks)(void *ctx);
  int      (*lread)(void *ctx, uint32_t lblock, uint32_t off,
                    uint8_t *buf, uint32_t len);
  int      (*lwrite_page)(void *ctx, uint32_t lblock, uint32_t page,
                          const uint8_t *data, uint32_t len);
  int      (*lerase)(void *ctx, uint32_t lblock);
} nand_lfs_bd_t;

/**
 * @brief   LittleFS-over-NAND binding object.
 */
typedef struct {
  const nand_lfs_bd_t *bd;          /**< backing block device ops      */
  uint32_t          base_lblock;   /**< first logical block of volume */
  lfs_t             lfs;           /**< littlefs state                */
  struct lfs_config cfg;           /**< littlefs config               */
  bool              mounted;
  /* lfs buffers (must be >= read/prog size). */
  uint8_t           read_buf[NAND_CHIP_MAX_PAGE_SIZE] __attribute__((aligned(32)));
  uint8_t           prog_buf[NAND_CHIP_MAX_PAGE_SIZE] __attribute__((aligned(32)));
  uint8_t           lookahead[1024U];
} nand_lfs_t;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  int   nand_lfs_mount(nand_lfs_t *nl, const nand_lfs_bd_t *bd,
                       uint32_t base_lblock, bool format_on_fail);
  int   nand_lfs_unmount(nand_lfs_t *nl);
  lfs_t *nand_lfs_handle(nand_lfs_t *nl);
#ifdef __cplusplus
}
#endif

#endif /* NAND_LFS_H */

/** @} */
