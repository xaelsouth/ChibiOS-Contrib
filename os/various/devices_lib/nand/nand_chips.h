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
 * @file    devices_lib/nand/nand_chips.h
 * @brief   Generic NAND chip database (JEDEC READID decode).
 *
 * @details Vendor/chip database keyed by the JEDEC ID bytes read with the
 *          NAND READ ID command (0x90): byte 0 = manufacturer, byte 1 =
 *          chip, bytes 2..4 = geometry/density encodings. A table of known
 *          chips (currently Samsung K9* series) is consulted first; unknown
 *          Samsung chips fall back to the generic JEDEC decoding of bytes
 *          2..4 (page size, spare size, block size and density all have
 *          standardized bit fields there).
 *
 *          This module is controller-agnostic: it only fills in the chip
 *          geometry, the NAND controller driver derives its own ECC layout
 *          from it. Keeping it separate lets a board support any NAND chip
 *          without touching the controller code.
 *
 * @addtogroup NAND
 * @{
 */

#ifndef NAND_CHIPS_H
#define NAND_CHIPS_H

#include <stdint.h>
#include <stdbool.h>

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/* Maximum geometry the database understands (drives buffer sizing in the
   controller drivers). */
#define NAND_CHIP_MAX_PAGE_SIZE     4096U
#define NAND_CHIP_MAX_OOB_SIZE      256U

/* Bytes reserved at the start of the spare (OOB) area for controller
   metadata (e.g. the 10-byte BCH metadata on i.MX23/i.MX28). Used to derive
   how many ECC bits fit into the spare area. */
#define NAND_CHIP_ECC_METADATA_SIZE 10U

/* nand_chip_decode() return codes. */
#define NAND_CHIP_OK                0
#define NAND_CHIP_ERR_UNKNOWN       (-1)    /* ID not recognized           */

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   NAND chip geometry, decoded from the JEDEC ID at probe time.
 */
typedef struct {
  char     name[24];          /**< marketing name, if known             */
  uint8_t  id[8];             /**< raw JEDEC READID bytes               */
  uint8_t  id_len;            /**< number of valid id[] bytes           */
  uint32_t page_size;         /**< main area bytes per page             */
  uint32_t oob_size;          /**< spare bytes per page                 */
  uint32_t pages_per_block;   /**< pages per erase block                */
  uint32_t block_size;        /**< erase block size in bytes            */
  uint32_t blocks;            /**< total physical erase blocks          */
  uint32_t pages;             /**< total pages                          */
  uint8_t  row_bytes;         /**< row address bytes (2 or 3)           */
  bool     mlc;               /**< MLC: bad block mark in the LAST page
                                   of a block (SLC: first page)        */
  uint8_t  ecc_strength;      /**< ECC bits per 512-byte chunk that fit
                                   into the spare area (0 = none)       */
  uint8_t  ecc_chunks;        /**< 512-byte chunks per page             */
  uint32_t good_blocks;       /**< erase blocks not marked bad (filled
                                   by the controller's BBT scan)       */
} nand_chip_geo_t;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  int  nand_chip_decode(const uint8_t *id, nand_chip_geo_t *g);
#ifdef __cplusplus
}
#endif

#endif /* NAND_CHIPS_H */

/** @} */
