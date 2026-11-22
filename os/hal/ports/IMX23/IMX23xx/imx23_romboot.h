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
 * @file    hal/ports/IMX23/IMX23xx/imx23_romboot.h
 * @brief   i.MX23 BootROM NAND boot structures writer (kobs-ng v0 port).
 *
 * @details Reimplements "kobs-ng init" on-device so the i.MX23 BootROM can
 *          boot from NAND: it writes the NCB (Hamming 13/8 encoded, raw
 *          pages), the LDLB/FCB and the DBBT into the first search areas,
 *          then the SB bootstream image (BCH-ECC pages, bad blocks skipped)
 *          starting at 6 search areas. Layout (search_exponent = 2, stride
 *          = one erase block):
 *
 *            blocks  0.. 7   NCB  (first page of each block, raw+Hamming)
 *            blocks  8..15   LDLB (first page of each block, BCH)
 *            blocks 16..23   DBBT header (+ bad block list on page 1, BCH)
 *            block 24+       SB bootstream image (BCH, erase per block)
 *
 *          The SB image itself is produced on the host by NXP elftosb from
 *          a .bd file; this module only writes it. Upload the .sb to the
 *          LittleFS volume (TFTP/HTTPS) and run "nand bootwrite /file.sb".
 *
 * @addtogroup NAND
 * @{
 */

#ifndef IMX23_ROMBOOT_H
#define IMX23_ROMBOOT_H

#include "hal.h"
#include "imx23_gpmi_nand.h"

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

#define IMX23_ROMBOOT_SEARCH_EXPONENT   2U      /* 2^2 strides per area     */
#define IMX23_ROMBOOT_STRIDES_PER_AREA  (1U << IMX23_ROMBOOT_SEARCH_EXPONENT)
/* Physical blocks reserved for ROM structures + bootstream image. LittleFS
   must start past this (see imx23_romboot_lfs_base()). */
#define IMX23_ROMBOOT_RESERVE_BLOCKS    48U
/* Two bootstream slots of 16 blocks (8 MiB) each after the BCB area. */
#define IMX23_ROMBOOT_SLOT_BLOCKS       16U
#define IMX23_ROMBOOT_SLOTS             2U

/* Fingerprints (little-endian ASCII). */
#define ROMBOOT_FP_STMP   0x504D5453U   /* 'STMP' */
#define ROMBOOT_FP_NCB    0x2042434EU   /* 'NCB ' */
#define ROMBOOT_FP_RBIN   0x4E494252U   /* 'RBIN' */
#define ROMBOOT_FP_LDLB   0x424C444CU   /* 'LDLB' */
#define ROMBOOT_FP_RBIL   0x4C494252U   /* 'RBIL' */
#define ROMBOOT_FP_DBBT   0x54424244U   /* 'DBBT' */
#define ROMBOOT_FP_RBID   0x44494252U   /* 'RBID' */

/* BCH ECC level enum values (nand_ecc_type_t in kobs BootControlBlocks.h). */
#define ROMBOOT_BCH_ECC_8BIT    6U
#define ROMBOOT_BCH_ECC_16BIT   10U

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   BootROM structures writer object.
 */
typedef struct {
  IMX23NANDDriver *np;                                /**< NAND driver     */
  uint8_t page_buf[IMX23_NAND_MAX_PAGE_TOTAL]         /**< page+oob stage  */
    __attribute__((aligned(32)));
} imx23_romboot_t;

/**
 * @brief   Bootstream source reader callback: read len bytes at sb offset.
 * @return  IMX23_NAND_OK on success.
 */
typedef int (*imx23_romboot_read_fn)(void *ctx, uint32_t offset,
                                     uint8_t *buf, uint32_t len);

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void     imx23_romboot_init(imx23_romboot_t *rb, IMX23NANDDriver *np);
  uint32_t imx23_romboot_bootstream_block(const imx23_nand_geo_t *g,
                                           uint32_t slot);
  uint32_t imx23_romboot_lfs_base(IMX23NANDDriver *np, uint32_t reserve_blocks);
  /* Write NCB/LDLB/DBBT + the SB image into boot slot (0 or 1); the LDLB
     is updated to point at that slot, so after reset the BootROM boots it. */
  int      imx23_romboot_write_sb(imx23_romboot_t *rb, imx23_romboot_read_fn rd,
                                  void *ctx, uint32_t sb_len, uint32_t slot);
  /* Re-point the LDLB at an already-written slot without rewriting the
     image (used to roll back / switch slots). */
  int      imx23_romboot_set_boot_slot(imx23_romboot_t *rb, uint32_t slot,
                                       uint32_t sb_sectors);
#ifdef __cplusplus
}
#endif

#endif /* IMX23_ROMBOOT_H */

/** @} */
