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
 * @file    devices_lib/nand/nand_chips.c
 * @brief   Generic NAND chip database (JEDEC READID decode).
 *
 * @addtogroup NAND
 * @{
 */

#include "nand_chips.h"

#include <string.h>

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/* Known chips: keyed by manufacturer (byte 0) and chip ID (byte 1). */
typedef struct {
  uint8_t  maker;               /* manufacturer ID byte                   */
  uint8_t  id2;                 /* chip ID byte                           */
  const char *name;
  uint32_t page_size;
  uint32_t oob_size;
  uint32_t pages_per_block;
  uint32_t blocks;              /* physical erase blocks                  */
  bool     mlc;                 /* MLC: BBM in the LAST page of a block   */
} nand_chip_entry_t;

static const nand_chip_entry_t nand_chip_table[] = {
  /* Samsung (maker 0xEC), K9* series. */
  { 0xECU, 0xF1U, "K9F1G08 1Gb SLC",  2048U,  64U,  64U, 1024U, false },
  { 0xECU, 0xDAU, "K9F2G08 2Gb SLC",  2048U,  64U,  64U, 2048U, false },
  { 0xECU, 0xDCU, "K9F4G08 4Gb SLC",  4096U, 128U,  64U, 2048U, false },
  { 0xECU, 0xD3U, "K9GBG08 8Gb MLC",  4096U, 224U, 128U, 4096U, true  },
  { 0xECU, 0xD7U, "K9LBG08 32Gb MLC", 4096U, 218U, 128U, 8192U, true  },
};

/* Samsung 3rd ID byte -> density in Gbit (for the generic fallback). */
static const struct { uint8_t id2; uint32_t gbit; } nand_density_table[] = {
  { 0xF1U, 1U }, { 0xDAU, 2U }, { 0xDCU, 4U }, { 0xD3U, 8U },
  { 0xD5U, 16U }, { 0xD7U, 32U },
};

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

static void geo_set_name(nand_chip_geo_t *g, const char *s) {
  size_t n = strlen(s);

  if (n >= sizeof(g->name)) {
    n = sizeof(g->name) - 1U;
  }
  memcpy(g->name, s, n);
  g->name[n] = '\0';
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Decode the chip geometry from JEDEC READID bytes.
 *
 * @param[in]  id   raw ID bytes (up to 8, at least 5 valid)
 * @param[out] g    geometry structure to fill
 *
 * @return  NAND_CHIP_OK on success, NAND_CHIP_ERR_UNKNOWN if the ID is not
 *          recognized or the geometry exceeds the supported maxima.
 */
int nand_chip_decode(const uint8_t *id, nand_chip_geo_t *g) {
  unsigned i;
  uint32_t gbit = 0U;
  bool mlc = false;

  memset(g, 0, sizeof(*g));
  memcpy(g->id, id, 8U);
  g->id_len = 5U;

  /* Table lookup on manufacturer + chip ID. */
  for (i = 0U; i < sizeof(nand_chip_table) / sizeof(nand_chip_table[0]); i++) {
    if (nand_chip_table[i].maker == id[0] && nand_chip_table[i].id2 == id[1]) {
      geo_set_name(g, nand_chip_table[i].name);
      g->page_size       = nand_chip_table[i].page_size;
      g->oob_size        = nand_chip_table[i].oob_size;
      g->pages_per_block = nand_chip_table[i].pages_per_block;
      g->blocks          = nand_chip_table[i].blocks;
      mlc                = nand_chip_table[i].mlc;
      goto have_geo;
    }
  }

  /* Generic JEDEC fallback (page/oob/block from byte 3, density from the
     byte 2 lookup table; currently only Samsung density encodings are
     known). */
  if (id[0] != 0xECU) {
    return NAND_CHIP_ERR_UNKNOWN;
  }
  g->page_size = 1024U << (id[3] & 0x03U);
  g->oob_size  = (8U << ((id[3] >> 2) & 0x01U)) * (g->page_size >> 9);
  g->block_size = (64U * 1024U) << ((id[3] >> 4) & 0x03U);
  g->pages_per_block = g->block_size / g->page_size;
  for (i = 0U; i < sizeof(nand_density_table) / sizeof(nand_density_table[0]);
       i++) {
    if (nand_density_table[i].id2 == id[2]) {
      gbit = nand_density_table[i].gbit;
      break;
    }
  }
  if (gbit == 0U || g->page_size > NAND_CHIP_MAX_PAGE_SIZE ||
      g->oob_size > NAND_CHIP_MAX_OOB_SIZE || g->pages_per_block == 0U) {
    return NAND_CHIP_ERR_UNKNOWN;
  }
  g->blocks = (gbit * 1024U * 1024U * 1024U / 8U) / g->block_size;
  mlc = ((id[2] & 0x0CU) >> 2) > 0U;
  {
    static const char hex[] = "0123456789ABCDEF";
    char nm[24];
    nm[0] = 'J'; nm[1] = 'E'; nm[2] = 'D'; nm[3] = 'E'; nm[4] = 'C'; nm[5] = ' ';
    nm[6] = hex[(id[0] >> 4) & 0xFU]; nm[7] = hex[id[0] & 0xFU];
    nm[8] = hex[(id[1] >> 4) & 0xFU]; nm[9] = hex[id[1] & 0xFU];
    nm[10] = hex[(id[2] >> 4) & 0xFU]; nm[11] = hex[id[2] & 0xFU];
    nm[12] = '\0';
    geo_set_name(g, nm);
  }

have_geo:
  g->pages       = g->blocks * g->pages_per_block;
  g->block_size  = g->pages_per_block * g->page_size;
  g->row_bytes   = (g->pages > 0xFFFFU) ? 3U : 2U;
  g->ecc_chunks  = (uint8_t)(g->page_size / 512U);
  /* ECC capacity of the spare area: all spare bytes except the metadata
     reservation, 13 bits per bit of strength per 512-byte chunk (GF(13)
     BCH), rounded down to an even strength, max 40 bits. */
  if (g->oob_size > NAND_CHIP_ECC_METADATA_SIZE) {
    g->ecc_strength = (uint8_t)(((g->oob_size - NAND_CHIP_ECC_METADATA_SIZE) *
                                 8U) / (13U * g->ecc_chunks));
  }
  else {
    g->ecc_strength = 0U;
  }
  g->ecc_strength &= (uint8_t)~1U;
  if (g->ecc_strength > 40U) {
    g->ecc_strength = 40U;
  }
  g->good_blocks = 0U;
  g->mlc         = mlc;
  return NAND_CHIP_OK;
}

/** @} */
