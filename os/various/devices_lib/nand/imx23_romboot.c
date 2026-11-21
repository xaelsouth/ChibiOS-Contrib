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
 * @file    devices_lib/nand/imx23_romboot.c
 * @brief   i.MX23 BootROM NAND boot structures writer (kobs-ng v0 port).
 *
 * @details Ported from imx-kobs (mtd.c v0_rom_mtd_init/v0_rom_mtd_commit_
 *          structures, ncb.c encode_hamming_code_13_8, BootControlBlocks.h)
 *          onto the imx23_gpmi_nand driver. All state lives in the
 *          imx23_romboot_t object (instance-safe).
 *
 * @addtogroup NAND
 * @{
 */

#include "hal.h"

#include <string.h>

#include "imx23_romboot.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/* Hamming 13/8 block layout (rom_nand_hamming_code_ecc.h). */
#define HC_DATA_BYTES       512U
#define HC_PARITY_BYTES     512U
#define HC_MAGIC_OFFSET     12U         /* BCB_MAGIC_OFFSET                 */

/* NAND timing defaults (kobs mtd.c mtd_config defaults). */
#define NCB_DATA_SETUP      80U
#define NCB_DATA_HOLD       60U
#define NCB_ADDRESS_SETUP   25U
#define NCB_DSAMPLE_TIME    6U

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/* NCB / LDLB / DBBT header - layout-compatible with kobs
   NCB_BootBlockStruct_t (all members naturally 4-byte aligned). */
typedef struct {
  uint32_t fp1;
  union {
    struct {
      uint8_t  timing[4];             /* data setup/hold, addr setup, sample */
      uint32_t data_page_size;
      uint32_t total_page_size;
      uint32_t sectors_per_block;
      uint32_t sector_in_page_mask;
      uint32_t sector_to_page_shift;
      uint32_t number_of_nands;
    } ncb1;
    struct {
      uint16_t major, minor, sub, rsv; /* LDLB version */
      uint32_t nand_bitmap;
    } ldlb1;
    struct {
      uint32_t number_bb_nand[4];
      uint32_t number_2k_pages_bb_nand[4];
    } dbbt1;
    uint32_t reserved1[10];
  } u1;
  uint32_t fp2;
  union {
    struct {
      uint32_t num_row_bytes;
      uint32_t num_column_bytes;
      uint32_t total_internal_die;
      uint32_t internal_planes_per_die;
      uint32_t cell_type;
      uint32_t ecc_type;
      uint32_t ecc_block0_size;
      uint32_t ecc_blockn_size;
      uint32_t ecc_block0_ecc_level;
      uint32_t num_ecc_blocks_per_page;
      uint32_t metadata_bytes;
      uint32_t erase_threshold;
      uint32_t read_1st_code;
      uint32_t read_2nd_code;
      uint32_t boot_patch;
      uint32_t patch_sectors;
      uint32_t firmware_starting_nand2;
    } ncb2;
    struct {
      uint32_t firmware_starting_nand;
      uint32_t firmware_starting_sector;
      uint32_t firmware_sector_stride;
      uint32_t sectors_in_firmware;
      uint32_t firmware_starting_nand2;
      uint32_t firmware_starting_sector2;
      uint32_t firmware_sector_stride2;
      uint32_t sectors_in_firmware2;
      struct { uint16_t major, minor, sub, rsv; } fw_version;
      uint32_t dbbt_sector;
      uint32_t dbbt_sector2;
    } ldlb2;
    uint32_t reserved2[20];
  } u2;
  uint32_t fp3;
  uint8_t  unknown[12];
} romboot_bcb_t;

/* DBBT bad block list page (BadBlockTableNand_t, 2K). */
typedef struct {
  uint32_t nand;
  uint32_t number_bb;
  uint32_t bad_block[(2048U / 4U) - 2U];
} romboot_bbtn_t;

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/* Hamming 13/8 parity of one data byte (kobs calculate_parity_13_8). */
static uint8_t hc_parity_13_8(uint8_t d) {
  uint8_t p = 0U;

  p |= (uint8_t)(((d >> 6) ^ (d >> 5) ^ (d >> 3) ^ (d >> 2)) & 1U) << 0;
  p |= (uint8_t)(((d >> 7) ^ (d >> 5) ^ (d >> 4) ^ (d >> 2) ^ (d >> 1)) & 1U) << 1;
  p |= (uint8_t)(((d >> 7) ^ (d >> 6) ^ (d >> 5) ^ (d >> 1) ^ (d >> 0)) & 1U) << 2;
  p |= (uint8_t)(((d >> 7) ^ (d >> 4) ^ (d >> 3) ^ (d >> 0)) & 1U) << 3;
  p |= (uint8_t)(((d >> 6) ^ (d >> 4) ^ (d >> 3) ^ (d >> 2) ^
                  (d >> 1) ^ (d >> 0)) & 1U) << 4;
  return p;
}

/* Hamming-encode a BCB struct into a zeroed page buffer (kobs
   ncb_encrypt + encode_hamming_code_13_8, ncb_version 3): data copy at
   offset 12, parity copy right after. */
static void hc_encode_13_8(const romboot_bcb_t *bcb, uint8_t *page) {
  uint8_t data[HC_DATA_BYTES];
  uint8_t ecc[HC_DATA_BYTES];
  uint32_t i;

  memset(data, 0, sizeof(data));
  memcpy(data, bcb, sizeof(*bcb));
  for (i = 0U; i < HC_DATA_BYTES; i++) {
    ecc[i] = hc_parity_13_8(data[i]);
  }
  memcpy(page + HC_MAGIC_OFFSET, data, HC_DATA_BYTES);
  memcpy(page + HC_MAGIC_OFFSET + HC_DATA_BYTES, ecc, HC_PARITY_BYTES);
}

/* Erase a physical block (tolerate failure: block may be bad). */
static int erase_block(IMX23NANDDriver *np, uint32_t block) {

  return imx23_nand_erase_block(np, block);
}

/* Write the same page image at page 0 of each block in
   [first_block, first_block + 4*areas). */
static int write_bcb_pages(imx23_romboot_t *rb, uint32_t first_block,
                           uint32_t areas, bool raw) {
  IMX23NANDDriver *np = rb->np;
  const imx23_nand_geo_t *g = imx23_nand_geo(np);
  uint32_t ppb = g->pages_per_block;
  uint32_t area, stride, blk;
  int rc;

  for (area = 0U; area < areas; area++) {
    for (stride = 0U; stride < IMX23_ROMBOOT_STRIDES_PER_AREA; stride++) {
      blk = first_block + area * IMX23_ROMBOOT_STRIDES_PER_AREA + stride;
      if (blk >= g->blocks) {
        return IMX23_NAND_ERR_PARAM;
      }
      (void)erase_block(np, blk);
      if (raw) {
        rc = imx23_nand_write_page_raw(np, blk * ppb, rb->page_buf,
                                       &rb->page_buf[g->page_size]);
      }
      else {
        rc = imx23_nand_write_page(np, blk * ppb, rb->page_buf, NULL);
      }
      if (rc != IMX23_NAND_OK) {
        return rc;
      }
    }
  }
  return IMX23_NAND_OK;
}

/* Fill the NCB fields from the probed geometry (kobs v0_rom_mtd_init). */
static void build_ncb(romboot_bcb_t *ncb, const imx23_nand_geo_t *g) {

  memset(ncb, 0, sizeof(*ncb));
  ncb->fp1 = ROMBOOT_FP_STMP;
  ncb->u1.ncb1.timing[0] = NCB_DATA_SETUP;
  ncb->u1.ncb1.timing[1] = NCB_DATA_HOLD;
  ncb->u1.ncb1.timing[2] = NCB_ADDRESS_SETUP;
  ncb->u1.ncb1.timing[3] = NCB_DSAMPLE_TIME;
  ncb->u1.ncb1.data_page_size  = g->page_size;
  ncb->u1.ncb1.total_page_size = g->page_size + g->oob_size;
  ncb->u1.ncb1.number_of_nands = 1U;
  ncb->fp2 = ROMBOOT_FP_NCB;
  ncb->u2.ncb2.num_row_bytes    = g->row_bytes;
  ncb->u2.ncb2.num_column_bytes = 2U;
  ncb->u2.ncb2.total_internal_die = 1U;
  ncb->u2.ncb2.internal_planes_per_die = 1U;
  ncb->u2.ncb2.cell_type = 0U;
  ncb->u2.ncb2.read_1st_code = 0x00U;
  ncb->u2.ncb2.read_2nd_code = 0x30U;
  ncb->u2.ncb2.ecc_block0_size = 512U;
  ncb->u2.ncb2.ecc_blockn_size = 512U;
  ncb->u2.ncb2.metadata_bytes = IMX23_NAND_METADATA_SIZE;

  if (g->page_size == 2048U) {
    ncb->u1.ncb1.sectors_per_block   = g->block_size / 2048U;
    ncb->u1.ncb1.sector_in_page_mask = 0U;
    ncb->u1.ncb1.sector_to_page_shift = 0U;
    ncb->u2.ncb2.ecc_type = ROMBOOT_BCH_ECC_8BIT;
    ncb->u2.ncb2.ecc_block0_ecc_level = ROMBOOT_BCH_ECC_8BIT;
    ncb->u2.ncb2.num_ecc_blocks_per_page = (2048U / 512U) - 1U;
  }
  else {  /* 4096 */
    ncb->u1.ncb1.sectors_per_block   = (g->block_size / g->page_size) * 2U;
    ncb->u1.ncb1.sector_in_page_mask = 1U;
    ncb->u1.ncb1.sector_to_page_shift = 1U;
    ncb->u2.ncb2.num_ecc_blocks_per_page = (g->page_size / 512U) - 1U;
    if (g->oob_size == 218U || g->oob_size == 224U) {
      ncb->u2.ncb2.ecc_type = ROMBOOT_BCH_ECC_16BIT;
      ncb->u2.ncb2.ecc_block0_ecc_level = ROMBOOT_BCH_ECC_16BIT;
    }
    else {
      ncb->u2.ncb2.ecc_type = ROMBOOT_BCH_ECC_8BIT;
      ncb->u2.ncb2.ecc_block0_ecc_level = ROMBOOT_BCH_ECC_8BIT;
    }
  }
  ncb->fp3 = ROMBOOT_FP_RBIN;
}

/* Fill the LDLB (FCB) pointing at the bootstream image. */
static void build_ldlb(romboot_bcb_t *ldlb, const imx23_nand_geo_t *g,
                       uint32_t bootstream_byte, uint32_t sb_sectors) {
  uint32_t search_area = IMX23_ROMBOOT_STRIDES_PER_AREA * g->block_size;

  memset(ldlb, 0, sizeof(*ldlb));
  ldlb->fp1 = ROMBOOT_FP_STMP;
  ldlb->u1.ldlb1.major = 1U;
  ldlb->u1.ldlb1.nand_bitmap = 0U;
  ldlb->fp2 = ROMBOOT_FP_LDLB;
  ldlb->u2.ldlb2.firmware_starting_nand   = 0U;
  ldlb->u2.ldlb2.firmware_starting_sector = bootstream_byte / 2048U;
  ldlb->u2.ldlb2.firmware_sector_stride   = 0U;
  ldlb->u2.ldlb2.sectors_in_firmware      = sb_sectors;
  ldlb->u2.ldlb2.firmware_starting_nand2  = 0U;
  ldlb->u2.ldlb2.firmware_starting_sector2 = bootstream_byte / 2048U;
  ldlb->u2.ldlb2.firmware_sector_stride2  = 0U;
  ldlb->u2.ldlb2.sectors_in_firmware2     = sb_sectors;
  ldlb->u2.ldlb2.fw_version.major = 1U;
  ldlb->u2.ldlb2.dbbt_sector  = (4U * search_area) / 2048U;
  ldlb->u2.ldlb2.dbbt_sector2 = (5U * search_area) / 2048U;
  ldlb->fp3 = ROMBOOT_FP_RBIL;
}

/* Fill the DBBT header + bad block list from the in-RAM BBT. */
static uint32_t build_dbbt(romboot_bcb_t *dbbt, romboot_bbtn_t *bbtn,
                           IMX23NANDDriver *np) {
  const imx23_nand_geo_t *g = imx23_nand_geo(np);
  uint32_t blk, nbad = 0U;

  memset(dbbt, 0, sizeof(*dbbt));
  memset(bbtn, 0, sizeof(*bbtn));
  dbbt->fp1 = ROMBOOT_FP_STMP;
  dbbt->fp2 = ROMBOOT_FP_DBBT;
  dbbt->fp3 = ROMBOOT_FP_RBID;

  bbtn->nand = 0U;
  for (blk = 0U; blk < g->blocks && nbad < 510U; blk++) {
    if (imx23_nand_is_bad(np, blk)) {
      bbtn->bad_block[nbad++] = blk;
    }
  }
  bbtn->number_bb = nbad;
  dbbt->u1.dbbt1.number_bb_nand[0] = nbad;
  dbbt->u1.dbbt1.number_2k_pages_bb_nand[0] = (nbad > 0U) ? 1U : 0U;
  return nbad;
}

/* Write the bootstream image page by page starting at the bootstream
   block, erasing each block on entry and skipping bad blocks. */
static int write_bootstream(imx23_romboot_t *rb, imx23_romboot_read_fn rd,
                            void *ctx, uint32_t sb_len, uint32_t start_block) {
  IMX23NANDDriver *np = rb->np;
  const imx23_nand_geo_t *g = imx23_nand_geo(np);
  uint32_t ppb = g->pages_per_block;
  uint32_t page_size = g->page_size;
  uint32_t blk = start_block;
  uint32_t page_in_blk = 0U;
  uint32_t off = 0U;
  int rc;

  while (off < sb_len) {
    uint32_t chunk = sb_len - off;

    if (imx23_nand_is_bad(np, blk)) {
      blk++;
      page_in_blk = 0U;
      continue;
    }
    if (page_in_blk == 0U) {
      rc = erase_block(np, blk);
      if (rc != IMX23_NAND_OK) {
        blk++;          /* treat as bad */
        imx23_nand_mark_bad(np, blk - 1U);
        page_in_blk = 0U;
        continue;
      }
    }
    if (chunk > page_size) {
      chunk = page_size;
    }
    memset(rb->page_buf, 0xFF, page_size);
    rc = rd(ctx, off, rb->page_buf, chunk);
    if (rc != IMX23_NAND_OK) {
      return rc;
    }
    rc = imx23_nand_write_page(np, blk * ppb + page_in_blk, rb->page_buf,
                               NULL);
    if (rc != IMX23_NAND_OK) {
      imx23_nand_mark_bad(np, blk);
      blk++;
      page_in_blk = 0U;
      continue;         /* retry the same chunk on the next block */
    }
    off += chunk;
    page_in_blk++;
    if (page_in_blk >= ppb) {
      blk++;
      page_in_blk = 0U;
    }
  }
  return IMX23_NAND_OK;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

void imx23_romboot_init(imx23_romboot_t *rb, IMX23NANDDriver *np) {

  memset(rb, 0, sizeof(*rb));
  rb->np = np;
}

uint32_t imx23_romboot_bootstream_block(const imx23_nand_geo_t *g,
                                        uint32_t slot) {

  /* 6 search areas (24 blocks) hold NCB/LDLB/DBBT, then the bootstream
     slots follow. */
  return 6U * IMX23_ROMBOOT_STRIDES_PER_AREA +
         slot * IMX23_ROMBOOT_SLOT_BLOCKS;
}

uint32_t imx23_romboot_lfs_base(IMX23NANDDriver *np, uint32_t reserve_blocks) {
  /* Map a physical block count to a logical block index (logical numbering
     skips bad blocks). */
  uint32_t lblocks = imx23_nand_lblocks(np);
  uint32_t l, base = 0U;

  for (l = 0U; l < lblocks; l++) {
    if (np->l2p[l] < reserve_blocks) {
      base = l + 1U;
    }
  }
  return base;
}

int imx23_romboot_write_sb(imx23_romboot_t *rb, imx23_romboot_read_fn rd,
                           void *ctx, uint32_t sb_len, uint32_t slot) {
  IMX23NANDDriver *np = rb->np;
  const imx23_nand_geo_t *g = imx23_nand_geo(np);
  romboot_bcb_t bcb;
  romboot_bbtn_t bbtn;
  uint32_t sb_sectors, boot_blk, nbad, ppb, area_strides;
  int rc;

  if (!np->ready || sb_len == 0U || slot >= IMX23_ROMBOOT_SLOTS) {
    return IMX23_NAND_ERR_PARAM;
  }
  if (g->page_size != 2048U && g->page_size != 4096U) {
    return IMX23_NAND_ERR_UNKNOWN_CHIP;
  }

  sb_sectors  = (sb_len + 2047U) / 2048U;
  area_strides = IMX23_ROMBOOT_STRIDES_PER_AREA;
  boot_blk    = imx23_romboot_bootstream_block(g, slot);

  /* 1) NCB: Hamming-encoded, raw page at block 0..7, page 0. */
  build_ncb(&bcb, g);
  memset(rb->page_buf, 0, g->page_size + g->oob_size);
  hc_encode_13_8(&bcb, rb->page_buf);
  rc = write_bcb_pages(rb, 0U, 2U, true);
  if (rc != IMX23_NAND_OK) {
    return rc;
  }

  /* 2) LDLB: BCH page at blocks 8..15, page 0. */
  build_ldlb(&bcb, g, boot_blk * g->block_size, sb_sectors);
  memset(rb->page_buf, 0, g->page_size + g->oob_size);
  memcpy(rb->page_buf, &bcb, sizeof(bcb));
  rc = write_bcb_pages(rb, 2U * area_strides, 2U, false);
  if (rc != IMX23_NAND_OK) {
    return rc;
  }

  /* 3) DBBT header at blocks 16..23 page 0 (+ bad block list on page 1). */
  nbad = build_dbbt(&bcb, &bbtn, np);
  memset(rb->page_buf, 0, g->page_size + g->oob_size);
  memcpy(rb->page_buf, &bcb, sizeof(bcb));
  rc = write_bcb_pages(rb, 4U * area_strides, 2U, false);
  if (rc != IMX23_NAND_OK) {
    return rc;
  }
  if (nbad > 0U) {
    uint32_t area, stride, blk;
    ppb = g->pages_per_block;
    for (area = 0U; area < 2U; area++) {
      for (stride = 0U; stride < area_strides; stride++) {
        blk = (4U + area) * area_strides + stride;
        memset(rb->page_buf, 0, g->page_size + g->oob_size);
        memcpy(rb->page_buf, &bbtn, sizeof(bbtn) > g->page_size ?
               g->page_size : sizeof(bbtn));
        rc = imx23_nand_write_page(np, blk * ppb + 1U, rb->page_buf, NULL);
        if (rc != IMX23_NAND_OK) {
          return rc;
        }
      }
    }
  }

  /* 4) Bootstream image into the requested slot, skipping bad blocks. */
  return write_bootstream(rb, rd, ctx, sb_len, boot_blk);
}

int imx23_romboot_set_boot_slot(imx23_romboot_t *rb, uint32_t slot,
                                uint32_t sb_sectors) {
  IMX23NANDDriver *np = rb->np;
  const imx23_nand_geo_t *g = imx23_nand_geo(np);
  romboot_bcb_t bcb;
  uint32_t boot_blk;
  int rc;

  if (!np->ready || slot >= IMX23_ROMBOOT_SLOTS) {
    return IMX23_NAND_ERR_PARAM;
  }
  boot_blk = imx23_romboot_bootstream_block(g, slot);
  build_ldlb(&bcb, g, boot_blk * g->block_size, sb_sectors);
  memset(rb->page_buf, 0, g->page_size + g->oob_size);
  memcpy(rb->page_buf, &bcb, sizeof(bcb));
  rc = write_bcb_pages(rb, 2U * IMX23_ROMBOOT_STRIDES_PER_AREA, 2U, false);
  return rc;
}

/** @} */
