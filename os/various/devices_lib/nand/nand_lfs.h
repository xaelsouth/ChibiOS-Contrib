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
 * @brief   LittleFS-over-GPMI-NAND binding header.
 *
 * @details Mounts a LittleFS volume on the MTD-style logical block device of
 *          an IMX23NANDDriver (bad blocks skipped, hardware BCH ECC, block
 *          erase = NAND erase block). Build littlefs with
 *          -DLFS_THREADSAFE=0 and add ext/littlefs (lfs.c, lfs_util.c) plus
 *          this directory to the build.
 *
 * @addtogroup NAND
 * @{
 */

#ifndef NAND_LFS_H
#define NAND_LFS_H

#include "hal.h"
#include "imx23_gpmi_nand.h"
#include "lfs.h"

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   LittleFS-over-NAND binding object.
 */
typedef struct {
  IMX23NANDDriver   *np;            /**< backing NAND driver               */
  uint32_t          base_lblock;   /**< first logical block of the volume */
  lfs_t             lfs;           /**< littlefs state                    */
  struct lfs_config cfg;           /**< littlefs config                   */
  bool              mounted;
  /* lfs buffers (must be >= read/prog size). */
  uint8_t           read_buf[IMX23_NAND_MAX_PAGE_SIZE] __attribute__((aligned(32)));
  uint8_t           prog_buf[IMX23_NAND_MAX_PAGE_SIZE] __attribute__((aligned(32)));
  uint8_t           lookahead[1024U];
} nand_lfs_t;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  int  nand_lfs_mount(nand_lfs_t *nl, IMX23NANDDriver *np,
                      uint32_t base_lblock, bool format_on_fail);
  int  nand_lfs_unmount(nand_lfs_t *nl);
  lfs_t *nand_lfs_handle(nand_lfs_t *nl);
#ifdef __cplusplus
}
#endif

#endif /* NAND_LFS_H */

/** @} */
