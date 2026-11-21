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
 * @file    devices_lib/nand/imx23_gpmi_nand.h
 * @brief   i.MX23 GPMI + BCH NAND flash driver header.
 *
 * @details Ported from U-Boot drivers/mtd/nand/raw/mxs_nand.c and Linux
 *          drivers/mtd/nand/raw/gpmi-nand. All page transfers are done by
 *          the APBH DMA engine on the GPMI channel; the BCH hardware
 *          computes/verifies ECC (one status byte per 512-byte chunk lands
 *          in the auxiliary buffer right after the 10-byte metadata).
 *
 *          The driver exposes three layers:
 *          - physical page/block operations with hardware ECC
 *            (imx23_nand_read_page/write_page/erase_block) plus raw
 *            (no-ECC) variants used by the U-Boot-style shell commands;
 *          - an in-RAM bad block table (bitmap_t, one bit per erase block)
 *            built at init by scanning the factory bad block marks in the
 *            OOB of the first two pages of every erase block (no on-flash
 *            BBT, matching the EVK devicetree which has nand-on-flash-bbt
 *            disabled);
 *          - an MTD-style logical block device (imx23_nand_lread/lwrite_page/
 *            lerase) that transparently skips bad blocks, intended as the
 *            backing store for LittleFS (see nand_lfs.h).
 *
 *          All mutable state lives in the IMX23NANDDriver object (no module
 *          static variables), so the code is instance-safe. One object
 *          (IMX23NAND1) is provided because there is a single GPMI block.
 *
 * @addtogroup NAND
 * @{
 */

#ifndef IMX23_GPMI_NAND_H
#define IMX23_GPMI_NAND_H

#include "hal.h"
#include "imx23.h"
#include "imx23_dma.h"
#include "bitmap.h"

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/* NAND flash command opcodes. */
#define IMX23_NAND_CMD_READ0        0x00U
#define IMX23_NAND_CMD_READOOB      0x50U
#define IMX23_NAND_CMD_PAGEPROG     0x10U
#define IMX23_NAND_CMD_READ0_CONF   0x30U
#define IMX23_NAND_CMD_ERASE0       0x60U
#define IMX23_NAND_CMD_ERASE_CONF   0xD0U
#define IMX23_NAND_CMD_STATUS       0x70U
#define IMX23_NAND_CMD_READID       0x90U
#define IMX23_NAND_CMD_RESET        0xFFU

/* Status register bit 0: 1 = operation failed. */
#define IMX23_NAND_STATUS_FAIL      0x01U

/* BCH metadata size prepended to the auxiliary (spare) area. */
#define IMX23_NAND_METADATA_SIZE    10U

/* Maximum supported geometry (drives the in-object buffer sizes). */
#define IMX23_NAND_MAX_PAGE_SIZE    4096U
#define IMX23_NAND_MAX_OOB_SIZE     256U
#define IMX23_NAND_MAX_PAGE_TOTAL   (IMX23_NAND_MAX_PAGE_SIZE + IMX23_NAND_MAX_OOB_SIZE)
#define IMX23_NAND_MAX_AUX          (IMX23_NAND_MAX_OOB_SIZE + 32U)
#define IMX23_NAND_MAX_BLOCKS       8192U
#define IMX23_NAND_BBT_WORDS        ((IMX23_NAND_MAX_BLOCKS + 31U) / 32U)

/* Return codes. */
#define IMX23_NAND_OK               0
#define IMX23_NAND_ERR_IO           (-1)    /* DMA / chip timeout          */
#define IMX23_NAND_ERR_ECC          (-2)    /* uncorrectable ECC           */
#define IMX23_NAND_ERR_BAD          (-3)    /* bad block                   */
#define IMX23_NAND_ERR_TIMEOUT      (-4)    /* device busy timeout         */
#define IMX23_NAND_ERR_NOT_READY    (-5)    /* driver not initialized      */
#define IMX23_NAND_ERR_ERASE        (-6)    /* erase failed (status FAIL)  */
#define IMX23_NAND_ERR_PROG         (-7)    /* program failed              */
#define IMX23_NAND_ERR_UNKNOWN_CHIP (-8)    /* JEDEC ID not recognized     */
#define IMX23_NAND_ERR_PARAM        (-9)

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   NAND chip geometry (decoded from the JEDEC ID at probe time).
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
  uint8_t  ecc_strength;      /**< BCH bits per 512-byte chunk          */
  uint8_t  ecc_chunks;        /**< 512-byte chunks per page             */
  uint8_t  row_bytes;         /**< row address bytes (2 or 3)           */
  uint32_t layout0;           /**< BCH FLASH0LAYOUT0 value              */
  uint32_t layout1;           /**< BCH FLASH0LAYOUT1 value              */
  uint32_t good_blocks;       /**< erase blocks not marked bad in BBT   */
} imx23_nand_geo_t;

/**
 * @brief   IMX23 GPMI NAND driver object.
 * @note    DMA buffers must reside in SDRAM and be word aligned; they are
 *          placed in the object which is a global, so that holds. The port
 *          runs with caches disabled, so no cache maintenance is needed.
 */
typedef struct {
  imx23_nand_geo_t  geo;                        /**< probed geometry       */
  uint8_t           cs;                         /**< GPMI chip select      */
  uint8_t           dma_ch;                     /**< APBH channel          */
  bool              ready;                      /**< init done, BBT valid  */
  bitmap_t          bbt;                        /**< 1 bit per phys block  */
  bitmap_word_t     bbt_array[IMX23_NAND_BBT_WORDS];
  uint16_t          l2p[IMX23_NAND_MAX_BLOCKS]; /**< logical->physical map */
  bool              mlc;                        /**< MLC: BBM in last page  */
  /* DMA resources (word aligned, in SDRAM via the global object). */
  imx23_dma_ccw_t   ccw[8]  __attribute__((aligned(32)));
  uint8_t           cmd_buf[8] __attribute__((aligned(32)));
  uint8_t           data_buf[IMX23_NAND_MAX_PAGE_TOTAL] __attribute__((aligned(32)));
  uint8_t           aux_buf[IMX23_NAND_MAX_AUX] __attribute__((aligned(32)));
} IMX23NANDDriver;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

extern IMX23NANDDriver IMX23NAND1;

  /* Init: pinctrl + GPMI clock + hardware reset + JEDEC probe + BCH layout
     + in-RAM BBT scan + logical block map. cs = GPMI chip select (0..3). */
  int  imx23_nand_init(IMX23NANDDriver *np, uint8_t cs);
  const imx23_nand_geo_t *imx23_nand_geo(IMX23NANDDriver *np);

  /* Physical page operations (page = absolute page number). data/oob may
     be NULL to skip that part. corrected (optional) gets the number of
     corrected bit errors (or -1 on uncorrectable). */
  int  imx23_nand_read_page(IMX23NANDDriver *np, uint32_t page,
                            uint8_t *data, uint8_t *oob, int *corrected);
  int  imx23_nand_write_page(IMX23NANDDriver *np, uint32_t page,
                             const uint8_t *data, const uint8_t *oob);
  /* Raw variants: no ECC is computed/checked, data and oob are transferred
     as-is (oob must be oob_size bytes). Used by the 'nand ... raw' shell
     commands to mimic U-Boot. */
  int  imx23_nand_read_page_raw(IMX23NANDDriver *np, uint32_t page,
                                uint8_t *data, uint8_t *oob);
  int  imx23_nand_write_page_raw(IMX23NANDDriver *np, uint32_t page,
                                 const uint8_t *data, const uint8_t *oob);
  /* Raw OOB-only read (NAND READ OOB command), no ECC. */
  int  imx23_nand_read_oob_raw(IMX23NANDDriver *np, uint32_t page,
                               uint8_t *oob);

  /* Erase one physical block; checks the status register. */
  int  imx23_nand_erase_block(IMX23NANDDriver *np, uint32_t block);
  /* Bad block test (factory mark from the BBT scan, or marked at runtime). */
  bool imx23_nand_is_bad(IMX23NANDDriver *np, uint32_t block);
  /* Mark a block bad in the RAM BBT (no on-flash BBT is maintained). */
  void imx23_nand_mark_bad(IMX23NANDDriver *np, uint32_t block);

  /* MTD-style logical block device over the good blocks (bad blocks are
     skipped; logical block n maps to the n-th good physical block). */
  uint32_t imx23_nand_lblocks(IMX23NANDDriver *np);   /* logical blocks   */
  int  imx23_nand_lread(IMX23NANDDriver *np, uint32_t lblock, uint32_t off,
                        uint8_t *buf, uint32_t len);
  int  imx23_nand_lwrite_page(IMX23NANDDriver *np, uint32_t lblock,
                              uint32_t page, const uint8_t *data,
                              uint32_t len);
  int  imx23_nand_lerase(IMX23NANDDriver *np, uint32_t lblock);

#ifdef __cplusplus
}
#endif

#endif /* IMX23_GPMI_NAND_H */

/** @} */
