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
 * @file    devices_lib/nand/imx23_gpmi_nand.c
 * @brief   i.MX23 GPMI + BCH NAND flash driver (ported from U-Boot
 *          drivers/mtd/nand/raw/mxs_nand.c and Linux
 *          drivers/mtd/nand/raw/gpmi-nand).
 *
 * @details Hardware notes (i.MX23 RM):
 *          - PIO words of a DMA command word are written to the GPMI
 *            register file starting at offset 0 with a 0x10 stride, i.e.
 *            pio[0]=CTRL0, pio[1]=COMPARE, pio[2]=ECCCTRL, pio[3]=ECCCOUNT,
 *            pio[4]=PAYLOAD, pio[5]=AUXILIARY.
 *          - With ADDRESS=CLE + ADDRESS_INCREMENT the first byte of a burst
 *            goes out with CLE (command) and the following bytes with ALE
 *            (addresses).
 *          - With BCH ECC enabled (BCH_PAGE) the data movement between
 *            memory and the NAND is done through the PAYLOAD/AUXILIARY
 *            pointers, the DMA command word is NO_DMA_XFER with the byte
 *            count in CTRL0/ECCCOUNT.
 *          - The auxiliary (spare) buffer in memory gets the flash spare
 *            area; the BCH then stores one status byte per 512-byte chunk
 *            at the word-aligned offset right after the 10-byte metadata
 *            (offset 12), i.e. aux[0] is the bad block mark on i.MX23
 *            (no bit swap needed, unlike i.MX6+).
 *
 * @addtogroup NAND
 * @{
 */

#include "hal.h"
#include "imx23_gpmi_nand.h"

#include <string.h>

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/* Timeouts. */
#define NAND_DMA_TIMEOUT_MS     2000U
#define NAND_BCH_TIMEOUT_MS     500U

/* GPMI clock: 24MHz XTAL, divider 1 -> 24MHz GPMI clock.
   TIMING0 AS=2, DS=1, DH=1 -> pin clock = 24/(1+1) = 12MHz, tRP = 83ns,
   safely above tREA/tALS/tCLS of any NAND. */
#define NAND_TIMING0            (IMX23_GPMI_TIMING0_ADDR_SETUP(2U) |        \
                                 IMX23_GPMI_TIMING0_DATA_HOLD(1U) |        \
                                 IMX23_GPMI_TIMING0_DATA_SETUP(1U))
#define NAND_TIMING1            IMX23_GPMI_TIMING1_BUSY_TIMEOUT(0x3FFFU)

/* Aux buffer offset of the first per-chunk ECC status byte
   (metadata padded to the next word boundary). */
#define NAND_AUX_STATUS_OFF     ((IMX23_NAND_METADATA_SIZE + 3U) & ~3U)

/* Number of CCW descriptors in the longest chain (ECC page write). */
#define NAND_CCW_COUNT          8U

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

IMX23NANDDriver IMX23NAND1;

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/* Known Samsung chips: maker 0xEC, keyed by the 2nd JEDEC ID byte. */
typedef struct {
  uint8_t  id2;                 /* chip id byte                           */
  const char *name;
  uint32_t page_size;
  uint32_t oob_size;
  uint32_t pages_per_block;
  uint32_t blocks;              /* physical erase blocks                  */
  bool     mlc;                 /* MLC: BBM in the LAST page of a block   */
} nand_chip_entry_t;

static const nand_chip_entry_t nand_chip_table[] = {
  { 0xF1U, "K9F1G08 1Gb SLC",   2048U,  64U,  64U, 1024U, false },
  { 0xDAU, "K9F2G08 2Gb SLC",   2048U,  64U,  64U, 2048U, false },
  { 0xDCU, "K9F4G08 4Gb SLC",   4096U, 128U,  64U, 2048U, false },
  { 0xD3U, "K9GBG08 8Gb MLC",   4096U, 224U, 128U, 4096U, true  },
  { 0xD7U, "K9LBG08 32Gb MLC",  4096U, 218U, 128U, 8192U, true  }, /* EVK */
};

/* Samsung 3rd ID byte -> density in Gbit (for the generic fallback). */
static const struct { uint8_t id2; uint32_t gbit; } nand_density_table[] = {
  { 0xF1U, 1U }, { 0xDAU, 2U }, { 0xDCU, 4U }, { 0xD3U, 8U },
  { 0xD5U, 16U }, { 0xD7U, 32U },
};

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

#define NAND_MS2TICKS(ms)                                                   \
  ((systime_t)(((uint64_t)(ms) * CH_CFG_ST_FREQUENCY) / 1000U))

static bool wait_sema(IMX23NANDDriver *np, uint32_t timeout_ms) {
  systime_t t0 = chVTGetSystemTimeX();
  systime_t dt = NAND_MS2TICKS(timeout_ms);

  while (!imx23_dma_done(IMX23_DMA_APBH, np->dma_ch)) {
    if ((chVTGetSystemTimeX() - t0) > dt) {
      return false;
    }
  }
  return true;
}

static bool bch_wait_complete(void) {
  systime_t t0 = chVTGetSystemTimeX();
  systime_t dt = NAND_MS2TICKS(NAND_BCH_TIMEOUT_MS);

  while ((IMX23_REG(IMX23_BCH_CTRL) & IMX23_BCH_CTRL_COMPLETE_IRQ) == 0U) {
    if ((chVTGetSystemTimeX() - t0) > dt) {
      return false;
    }
  }
  IMX23_REG_CLR(IMX23_BCH_CTRL, IMX23_BCH_CTRL_COMPLETE_IRQ);
  return true;
}

/* CTRL0 base value for this driver's chip select. */
static uint32_t ctrl0_base(IMX23NANDDriver *np) {

  return IMX23_GPMI_CTRL0_WORD_LENGTH | IMX23_GPMI_CTRL0_CS(np->cs);
}

/* CCW builders. Each sets all fields; chaining is fixed up in nand_run(). */

/* PIO-only descriptor (optionally with a mem->periph byte phase). */
static void ccw_cmd(IMX23NANDDriver *np, unsigned i, uint32_t ctrl0,
                    const uint8_t *buf, unsigned n) {
  imx23_dma_ccw_t *c = &np->ccw[i];

  c->bits    = IMX23_CCW_COMMAND_READ | IMX23_CCW_CHAIN |
               IMX23_CCW_WAIT4END | IMX23_CCW_SEMAPHORE |
               IMX23_CCW_PIO_NUM(1U);
  c->xfer    = (uint16_t)n;
  c->bufaddr = (uint32_t)(uintptr_t)buf;
  c->pio[0]  = ctrl0;
}

/* Wait-for-ready descriptor. */
static void ccw_wait_ready(IMX23NANDDriver *np, unsigned i, uint32_t ctrl0) {
  imx23_dma_ccw_t *c = &np->ccw[i];

  c->bits    = IMX23_CCW_COMMAND_NO_XFER | IMX23_CCW_CHAIN |
               IMX23_CCW_WAIT4READY | IMX23_CCW_WAIT4END |
               IMX23_CCW_SEMAPHORE | IMX23_CCW_PIO_NUM(1U);
  c->xfer    = 0U;
  c->bufaddr = 0U;
  c->pio[0]  = ctrl0;
}

/* Raw data read (peripheral -> memory, no ECC). */
static void ccw_read_raw(IMX23NANDDriver *np, unsigned i, uint32_t ctrl0,
                         uint8_t *buf, unsigned n) {
  imx23_dma_ccw_t *c = &np->ccw[i];

  c->bits    = IMX23_CCW_COMMAND_WRITE | IMX23_CCW_CHAIN |
               IMX23_CCW_WAIT4END | IMX23_CCW_SEMAPHORE |
               IMX23_CCW_PIO_NUM(1U);
  c->xfer    = (uint16_t)n;
  c->bufaddr = (uint32_t)(uintptr_t)buf;
  c->pio[0]  = ctrl0;
}

/* Raw data write (memory -> peripheral, no ECC). */
static void ccw_write_raw(IMX23NANDDriver *np, unsigned i, uint32_t ctrl0,
                          const uint8_t *buf, unsigned n) {
  imx23_dma_ccw_t *c = &np->ccw[i];

  c->bits    = IMX23_CCW_COMMAND_READ | IMX23_CCW_CHAIN |
               IMX23_CCW_WAIT4END | IMX23_CCW_SEMAPHORE |
               IMX23_CCW_PIO_NUM(1U);
  c->xfer    = (uint16_t)n;
  c->bufaddr = (uint32_t)(uintptr_t)buf;
  c->pio[0]  = ctrl0;
}

/* BCH page read: 6 PIO words, data lands in data_buf/aux_buf. */
static void ccw_read_ecc(IMX23NANDDriver *np, unsigned i, uint32_t page_total) {
  imx23_dma_ccw_t *c = &np->ccw[i];

  c->bits    = IMX23_CCW_COMMAND_NO_XFER | IMX23_CCW_CHAIN |
               IMX23_CCW_WAIT4END | IMX23_CCW_SEMAPHORE |
               IMX23_CCW_PIO_NUM(6U);
  c->xfer    = 0U;
  c->bufaddr = 0U;
  c->pio[0]  = ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_READ |
               IMX23_GPMI_CTRL0_ADDR_DATA |
               IMX23_GPMI_CTRL0_XFER_COUNT(page_total);
  c->pio[1]  = 0U;
  c->pio[2]  = IMX23_GPMI_ECCCTRL_ENABLE_ECC |
               IMX23_GPMI_ECCCTRL_ECC_CMD_DECODE |
               IMX23_GPMI_ECCCTRL_BCH_PAGE;
  c->pio[3]  = page_total;                          /* ECCCOUNT            */
  c->pio[4]  = (uint32_t)(uintptr_t)np->data_buf;   /* PAYLOAD             */
  c->pio[5]  = (uint32_t)(uintptr_t)np->aux_buf;    /* AUXILIARY           */
}

/* BCH page write: 6 PIO words, data taken from data_buf/aux_buf. */
static void ccw_write_ecc(IMX23NANDDriver *np, unsigned i, uint32_t page_total) {
  imx23_dma_ccw_t *c = &np->ccw[i];

  c->bits    = IMX23_CCW_COMMAND_NO_XFER | IMX23_CCW_CHAIN |
               IMX23_CCW_WAIT4END | IMX23_CCW_SEMAPHORE |
               IMX23_CCW_PIO_NUM(6U);
  c->xfer    = 0U;
  c->bufaddr = 0U;
  c->pio[0]  = ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WRITE |
               IMX23_GPMI_CTRL0_ADDR_DATA |
               IMX23_GPMI_CTRL0_XFER_COUNT(page_total);
  c->pio[1]  = 0U;
  c->pio[2]  = IMX23_GPMI_ECCCTRL_ENABLE_ECC |
               IMX23_GPMI_ECCCTRL_ECC_CMD_ENCODE |
               IMX23_GPMI_ECCCTRL_BCH_PAGE;
  c->pio[3]  = page_total;                          /* ECCCOUNT            */
  c->pio[4]  = (uint32_t)(uintptr_t)np->data_buf;   /* PAYLOAD             */
  c->pio[5]  = (uint32_t)(uintptr_t)np->aux_buf;    /* AUXILIARY           */
}

/* BCH off + wait for ready (3 PIO words). */
static void ccw_bch_off_wait(IMX23NANDDriver *np, unsigned i,
                             uint32_t page_total) {
  imx23_dma_ccw_t *c = &np->ccw[i];

  c->bits    = IMX23_CCW_COMMAND_NO_XFER | IMX23_CCW_CHAIN |
               IMX23_CCW_WAIT4READY | IMX23_CCW_WAIT4END |
               IMX23_CCW_SEMAPHORE | IMX23_CCW_PIO_NUM(3U);
  c->xfer    = 0U;
  c->bufaddr = 0U;
  c->pio[0]  = ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WAITRDY |
               IMX23_GPMI_CTRL0_ADDR_DATA |
               IMX23_GPMI_CTRL0_XFER_COUNT(page_total);
  c->pio[1]  = 0U;
  c->pio[2]  = 0U;                                  /* ECCCTRL: BCH off    */
}

/* Chain terminator: IRQ + decrement semaphore, no chain. */
static void ccw_end(IMX23NANDDriver *np, unsigned i) {
  imx23_dma_ccw_t *c = &np->ccw[i];

  c->next    = 0U;
  c->bits    = IMX23_CCW_COMMAND_NO_XFER | IMX23_CCW_IRQ |
               IMX23_CCW_SEMAPHORE;
  c->xfer    = 0U;
  c->bufaddr = 0U;
  c->pio[0]  = 0U;
}

/* Fix up chaining and run n descriptors on the GPMI DMA channel. */
static int nand_run(IMX23NANDDriver *np, unsigned n) {
  unsigned i;

  for (i = 0U; i < n; i++) {
    np->ccw[i].next = (i + 1U < n) ? (uint32_t)(uintptr_t)&np->ccw[i + 1U]
                                   : 0U;
  }
  imx23_dma_reset(IMX23_DMA_APBH, np->dma_ch);
  imx23_dma_irq_clear(IMX23_DMA_APBH, np->dma_ch);
  imx23_dma_start_chain(IMX23_DMA_APBH, np->dma_ch, &np->ccw[0], n);
  if (!wait_sema(np, NAND_DMA_TIMEOUT_MS)) {
    imx23_dma_stop(IMX23_DMA_APBH, np->dma_ch);
    imx23_dma_reset(IMX23_DMA_APBH, np->dma_ch);
    return IMX23_NAND_ERR_IO;
  }
  return IMX23_NAND_OK;
}

/* Send a command + address phase: first byte with CLE, the rest with ALE
   (ADDRESS_INCREMENT does the CLE->ALE switch after the first byte). */
static int nand_send_cmd(IMX23NANDDriver *np, const uint8_t *bytes,
                         unsigned n) {
  uint32_t ctrl0 = ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WRITE |
                   IMX23_GPMI_CTRL0_ADDR_CLE |
                   IMX23_GPMI_CTRL0_ADDR_INCREMENT |
                   IMX23_GPMI_CTRL0_XFER_COUNT(n);

  ccw_cmd(np, 0U, ctrl0, bytes, n);
  ccw_wait_ready(np, 1U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WAITRDY |
                            IMX23_GPMI_CTRL0_ADDR_DATA);
  ccw_end(np, 2U);

  return nand_run(np, 3U);
}

/* Build the row/col address bytes for a page. */
static unsigned nand_page_addr(IMX23NANDDriver *np, uint32_t page,
                               uint8_t *out /* >= 5 bytes */) {
  const imx23_nand_geo_t *g = &np->geo;
  uint32_t row = page / g->pages_per_block;
  uint32_t col = (page % g->pages_per_block) * g->page_size;
  unsigned n = 0U;

  out[n++] = (uint8_t)(col & 0xFFU);
  out[n++] = (uint8_t)((col >> 8) & 0xFFU);
  out[n++] = (uint8_t)(row & 0xFFU);
  if (g->row_bytes > 2U) {
    out[n++] = (uint8_t)((row >> 8) & 0xFFU);
    out[n++] = (uint8_t)((row >> 16) & 0xFFU);
  }
  else {
    out[n++] = (uint8_t)((row >> 8) & 0xFFU);
  }
  return n;
}

static unsigned nand_row_addr(IMX23NANDDriver *np, uint32_t block,
                              uint8_t *out /* >= 3 bytes */) {
  const imx23_nand_geo_t *g = &np->geo;
  unsigned n = 0U;

  out[n++] = (uint8_t)(block & 0xFFU);
  out[n++] = (uint8_t)((block >> 8) & 0xFFU);
  if (g->row_bytes > 2U) {
    out[n++] = (uint8_t)((block >> 16) & 0xFFU);
  }
  return n;
}

/* GPMI clock: 24MHz XTAL, divider 1, bypass PLL. */
static void gpmi_clock_setup(void) {

  IMX23_REG_CLR(IMX23_CLKCTRL_GPMI, IMX23_CLKCTRL_GPMI_CLKGATE);
  while ((IMX23_REG(IMX23_CLKCTRL_GPMI) & IMX23_CLKCTRL_GPMI_CLKGATE) != 0U) {
  }
  IMX23_REG(IMX23_CLKCTRL_GPMI) = 1U;      /* DIV = 1                     */
  while ((IMX23_REG(IMX23_CLKCTRL_GPMI) & IMX23_CLKCTRL_GPMI_BUSY) != 0U) {
  }
  IMX23_REG_SET(IMX23_CLKCTRL_CLKSEQ, IMX23_CLKCTRL_CLKSEQ_BYPASS_GPMI);
}

/* GPMI pads (EVK wiring): bank0 pins 0-7 (D0-7), 16 CLE, 17 ALE, 19 RDY0,
   23 WPN, 24 WRN, 25 RDN, bank2 pin 28 CE0N; all function 0. */
static void gpmi_pinctrl_setup(void) {
  static const uint8_t pins[] = { 0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U,
                                  16U, 17U, 19U, 23U, 24U, 25U };
  unsigned i;

  for (i = 0U; i < sizeof(pins); i++) {
    imx23_iomux_set_pad(0U, pins[i], 0U,
                        IMX23_DRIVE_3V3 | IMX23_DRIVE_4MA);
  }
  /* Write-protect and read/write strobes need 12mA. */
  imx23_iomux_set_pad(0U, 23U, 0U, IMX23_DRIVE_3V3 | IMX23_DRIVE_12MA);
  imx23_iomux_set_pad(0U, 24U, 0U, IMX23_DRIVE_3V3 | IMX23_DRIVE_12MA);
  imx23_iomux_set_pad(0U, 25U, 0U, IMX23_DRIVE_3V3 | IMX23_DRIVE_12MA);
  /* Chip enable. */
  imx23_iomux_set_pad(2U, 28U, 0U, IMX23_DRIVE_3V3 | IMX23_DRIVE_4MA);
}

/* mxs-style block reset. just_enable: only clear SFTRST/CLKGATE (MX23 BCH). */
static void gpmi_reset_block(uint32_t reg, bool just_enable) {

  IMX23_REG_CLR(reg, IMX23_GPMI_CTRL0_SFTRST);
  while ((IMX23_REG(reg) & IMX23_GPMI_CTRL0_SFTRST) != 0U) {
  }
  IMX23_REG_CLR(reg, IMX23_GPMI_CTRL0_CLKGATE);
  if (!just_enable) {
    IMX23_REG_SET(reg, IMX23_GPMI_CTRL0_SFTRST);
    while ((IMX23_REG(reg) & IMX23_GPMI_CTRL0_CLKGATE) == 0U) {
    }
    IMX23_REG_CLR(reg, IMX23_GPMI_CTRL0_SFTRST);
    while ((IMX23_REG(reg) & IMX23_GPMI_CTRL0_SFTRST) != 0U) {
    }
    IMX23_REG_CLR(reg, IMX23_GPMI_CTRL0_CLKGATE);
  }
}

static void gpmi_hw_init(IMX23NANDDriver *np) {

  gpmi_reset_block(IMX23_GPMI_CTRL0, false);
  /* MX23 BCH erratum: just ungate, no full reset. */
  gpmi_reset_block(IMX23_BCH_CTRL, true);

  /* NAND mode (GPMI_MODE=0), IRQRDY polarity, release DEV_RESET, BCH. */
  IMX23_REG(IMX23_GPMI_CTRL1) =
      (IMX23_REG(IMX23_GPMI_CTRL1) & ~IMX23_GPMI_CTRL1_GPMI_MODE) |
      IMX23_GPMI_CTRL1_ATA_IRQRDY_POL | IMX23_GPMI_CTRL1_DEV_RESET |
      IMX23_GPMI_CTRL1_BCH_MODE;

  IMX23_REG(IMX23_GPMI_TIMING0) = NAND_TIMING0;
  IMX23_REG(IMX23_GPMI_TIMING1) = NAND_TIMING1;

  /* All chip selects use BCH layout 0; enable BCH completion IRQ. */
  IMX23_REG(IMX23_BCH_LAYOUTSELECT) = 0U;
  IMX23_REG_SET(IMX23_BCH_CTRL, IMX23_BCH_CTRL_COMPLETE_IRQ_EN);

  imx23_dma_reset(IMX23_DMA_APBH, np->dma_ch);
}

/* Program the BCH flash layout for the probed geometry. */
static void bch_set_layout(IMX23NANDDriver *np) {
  imx23_nand_geo_t *g = &np->geo;
  uint32_t ecc0n = g->ecc_strength >> 1;

  g->layout0 = IMX23_BCH_LAYOUT0_NBLOCKS(g->ecc_chunks - 1U) |
               IMX23_BCH_LAYOUT0_META_SIZE(IMX23_NAND_METADATA_SIZE) |
               IMX23_BCH_LAYOUT0_ECC0(ecc0n) |
               IMX23_BCH_LAYOUT0_CHUNK0(512U);            /* GF13, MX23      */
  g->layout1 = IMX23_BCH_LAYOUT1_PAGE_SIZE(g->page_size + g->oob_size) |
               IMX23_BCH_LAYOUT1_ECCN(ecc0n) |
               IMX23_BCH_LAYOUT1_CHUNKN(512U);

  IMX23_REG(IMX23_BCH_FLASH0LAYOUT0) = g->layout0;
  IMX23_REG(IMX23_BCH_FLASH0LAYOUT1) = g->layout1;
}

/* READID: returns the number of ID bytes read (up to 8). */
static int nand_read_id(IMX23NANDDriver *np, uint8_t *id) {
  uint8_t cmd[2] = { IMX23_NAND_CMD_READID, 0x00U };
  uint32_t ctrl0 = ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_READ |
                   IMX23_GPMI_CTRL0_ADDR_DATA | IMX23_GPMI_CTRL0_XFER_COUNT(8U);
  int rc;

  ccw_cmd(np, 0U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WRITE |
                      IMX23_GPMI_CTRL0_ADDR_CLE |
                      IMX23_GPMI_CTRL0_ADDR_INCREMENT |
                      IMX23_GPMI_CTRL0_XFER_COUNT(2U), cmd, 2U);
  ccw_read_raw(np, 1U, ctrl0, np->data_buf, 8U);
  ccw_wait_ready(np, 2U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WAITRDY |
                            IMX23_GPMI_CTRL0_ADDR_DATA);
  ccw_end(np, 3U);
  rc = nand_run(np, 4U);
  if (rc == IMX23_NAND_OK) {
    memcpy(id, np->data_buf, 8U);
  }
  return rc;
}

static void geo_set_name(imx23_nand_geo_t *g, const char *s) {
  size_t n = strlen(s);

  if (n >= sizeof(g->name)) {
    n = sizeof(g->name) - 1U;
  }
  memcpy(g->name, s, n);
  g->name[n] = '\0';
}

/* Decode geometry from the JEDEC ID into np->geo. */
static int nand_decode_id(IMX23NANDDriver *np, const uint8_t *id) {
  imx23_nand_geo_t *g = &np->geo;
  unsigned i;
  uint32_t gbit = 0U;
  bool mlc = false;

  memcpy(g->id, id, 8U);
  g->id_len = 5U;

  /* Table lookup on the Samsung chip ID (2nd byte). */
  for (i = 0U; i < sizeof(nand_chip_table) / sizeof(nand_chip_table[0]); i++) {
    if (nand_chip_table[i].id2 == id[1]) {
      geo_set_name(g, nand_chip_table[i].name);
      g->page_size       = nand_chip_table[i].page_size;
      g->oob_size        = nand_chip_table[i].oob_size;
      g->pages_per_block = nand_chip_table[i].pages_per_block;
      g->blocks          = nand_chip_table[i].blocks;
      mlc                = nand_chip_table[i].mlc;
      goto have_geo;
    }
  }

  /* Generic JEDEC ext-id fallback (page/oob/block from byte 3, density
     from byte 2 lookup). */
  if (id[0] != 0xECU) {
    return IMX23_NAND_ERR_UNKNOWN_CHIP;
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
  if (gbit == 0U || g->page_size > IMX23_NAND_MAX_PAGE_SIZE ||
      g->oob_size > IMX23_NAND_MAX_OOB_SIZE ||
      g->pages_per_block == 0U) {
    return IMX23_NAND_ERR_UNKNOWN_CHIP;
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
  /* Legacy BCH layout: use all spare bytes except the 10 metadata bytes
     for ECC, GF(13), rounded down to an even strength, max 40 on MX23. */
  g->ecc_strength = (uint8_t)(((g->oob_size - IMX23_NAND_METADATA_SIZE) * 8U) /
                              (13U * g->ecc_chunks));
  g->ecc_strength &= (uint8_t)~1U;
  if (g->ecc_strength > 40U) {
    g->ecc_strength = 40U;
  }
  g->good_blocks = 0U;
  np->mlc        = mlc;
  return IMX23_NAND_OK;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Full driver initialization.
 * @details Ungates the GPMI clock, configures the pads, resets the GPMI/BCH
 *          blocks, reads the JEDEC ID, programs the BCH layout, then scans
 *          all erase blocks for factory bad block marks (in-RAM BBT) and
 *          builds the logical->physical block map.
 *
 * @return  IMX23_NAND_OK or a negative error code.
 */
int imx23_nand_init(IMX23NANDDriver *np, uint8_t cs) {
  int rc;
  uint32_t b, good;

  osalDbgCheck(np != NULL);
  osalDbgCheck(cs < 4U);

  memset(np, 0, sizeof(*np));
  np->cs     = cs;
  np->dma_ch = IMX23_DMA_CH_GPMI0 + cs;

  gpmi_clock_setup();
  gpmi_pinctrl_setup();
  gpmi_hw_init(np);

  /* NAND reset command. */
  {
    uint8_t rst = IMX23_NAND_CMD_RESET;
    rc = nand_send_cmd(np, &rst, 1U);
    if (rc != IMX23_NAND_OK) {
      return rc;
    }
  }

  rc = nand_read_id(np, np->geo.id);
  if (rc != IMX23_NAND_OK) {
    return rc;
  }
  rc = nand_decode_id(np, np->geo.id);
  if (rc != IMX23_NAND_OK) {
    return rc;
  }

  bch_set_layout(np);

  /* In-RAM BBT: scan the bad block mark in the OOB of the BBM page(s).
     SLC Samsung: first and second page of each block; MLC: last page. */
  np->bbt.array = np->bbt_array;
  np->bbt.len   = (np->geo.blocks + 31U) / 32U;
  bitmapObjectInit(&np->bbt, 0U);    /* all good */

  for (b = 0U; b < np->geo.blocks; b++) {
    uint32_t p0 = b * np->geo.pages_per_block;
    uint32_t p1 = np->mlc ? (p0 + np->geo.pages_per_block - 1U) : (p0 + 1U);
    uint8_t oob[IMX23_NAND_MAX_OOB_SIZE];

    rc = imx23_nand_read_oob_raw(np, p0, oob);
    if (rc != IMX23_NAND_OK) {
      bitmapSet(&np->bbt, b);        /* unreadable -> treat as bad */
      continue;
    }
    if (oob[0] != 0xFFU) {
      bitmapSet(&np->bbt, b);
      continue;
    }
    if (!np->mlc) {
      rc = imx23_nand_read_oob_raw(np, p1, oob);
      if (rc != IMX23_NAND_OK || oob[0] != 0xFFU) {
        bitmapSet(&np->bbt, b);
      }
    }
  }

  /* Logical -> physical map over the good blocks. */
  good = 0U;
  for (b = 0U; b < np->geo.blocks; b++) {
    if (bitmapGet(&np->bbt, b) == 0U) {
      np->l2p[good++] = (uint16_t)b;
    }
  }
  np->geo.good_blocks = good;
  np->ready = true;

  return IMX23_NAND_OK;
}

const imx23_nand_geo_t *imx23_nand_geo(IMX23NANDDriver *np) {

  return &np->geo;
}

/**
 * @brief   Read one page with hardware ECC.
 */
int imx23_nand_read_page(IMX23NANDDriver *np, uint32_t page,
                         uint8_t *data, uint8_t *oob, int *corrected) {
  imx23_nand_geo_t *g = &np->geo;
  uint32_t page_total = g->page_size + g->oob_size;
  uint8_t addr[5];
  unsigned naddr;
  unsigned k;
  int rc, corr = 0, failed = 0;
  uint8_t blk0;
  const uint8_t *status;

  if (!np->ready || page >= g->pages) {
    return IMX23_NAND_ERR_PARAM;
  }

  naddr = nand_page_addr(np, page, addr);
  np->cmd_buf[0] = IMX23_NAND_CMD_READ0;
  memcpy(&np->cmd_buf[1], addr, naddr);
  k = 1U + naddr;
  np->cmd_buf[k] = IMX23_NAND_CMD_READ0_CONF;

  ccw_wait_ready(np, 0U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WAITRDY |
                             IMX23_GPMI_CTRL0_ADDR_DATA);
  ccw_cmd(np, 1U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WRITE |
                     IMX23_GPMI_CTRL0_ADDR_CLE |
                     IMX23_GPMI_CTRL0_ADDR_INCREMENT |
                     IMX23_GPMI_CTRL0_XFER_COUNT(k), np->cmd_buf, k);
  ccw_cmd(np, 2U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WRITE |
                     IMX23_GPMI_CTRL0_ADDR_CLE |
                     IMX23_GPMI_CTRL0_XFER_COUNT(1U), &np->cmd_buf[k], 1U);
  ccw_read_ecc(np, 3U, page_total);
  ccw_bch_off_wait(np, 4U, page_total);
  ccw_end(np, 5U);

  rc = nand_run(np, 6U);
  if (rc != IMX23_NAND_OK) {
    return rc;
  }
  if (!bch_wait_complete()) {
    return IMX23_NAND_ERR_TIMEOUT;
  }

  /* Per-chunk status bytes live right after the metadata in aux. */
  status = &np->aux_buf[NAND_AUX_STATUS_OFF];
  for (unsigned i = 0U; i < g->ecc_chunks; i++) {
    if (status[i] == IMX23_BCH_STATUS_BLK_OK) {
      continue;
    }
    if (status[i] == IMX23_BCH_STATUS_BLK_ERASED) {
      continue;
    }
    if (status[i] == IMX23_BCH_STATUS_BLK_UNCORR) {
      failed++;
      continue;
    }
    corr += status[i];
  }
  blk0 = IMX23_BCH_STATUS0_BLK0(IMX23_REG(IMX23_BCH_STATUS0));
  if (blk0 == IMX23_BCH_STATUS_BLK_UNCORR) {
    failed++;
  }
  else if (blk0 != IMX23_BCH_STATUS_BLK_OK &&
           blk0 != IMX23_BCH_STATUS_BLK_ERASED) {
    corr += blk0;
  }

  if (data != NULL) {
    memcpy(data, np->data_buf, g->page_size);
  }
  if (oob != NULL) {
    /* aux[0..oob): spare area; bytes at NAND_AUX_STATUS_OFF are the ECC
       status, not flash content. */
    memcpy(oob, np->aux_buf, g->oob_size);
  }
  if (corrected != NULL) {
    *corrected = failed ? (-1) : corr;
  }
  return failed ? IMX23_NAND_ERR_ECC : IMX23_NAND_OK;
}

/**
 * @brief   Write one page with hardware ECC (BCH encode).
 * @note    The page must be erased; oob (if given) is written to the spare
 *          area, with the first 10 bytes being the BCH metadata (byte 0 is
 *          the bad block mark location - pass 0xFF there to keep the block
 *          good). Pass NULL to write an all-0xFF spare area.
 */
int imx23_nand_write_page(IMX23NANDDriver *np, uint32_t page,
                          const uint8_t *data, const uint8_t *oob) {
  imx23_nand_geo_t *g = &np->geo;
  uint32_t page_total = g->page_size + g->oob_size;
  uint8_t addr[5];
  unsigned naddr, k;
  int rc;

  if (!np->ready || page >= g->pages) {
    return IMX23_NAND_ERR_PARAM;
  }

  if (data != NULL) {
    memcpy(np->data_buf, data, g->page_size);
  }
  else {
    memset(np->data_buf, 0xFFU, g->page_size);
  }
  memset(np->aux_buf, 0xFFU, g->oob_size);
  if (oob != NULL) {
    memcpy(np->aux_buf, oob, g->oob_size);
  }

  naddr = nand_page_addr(np, page, addr);
  np->cmd_buf[0] = IMX23_NAND_CMD_PAGEPROG;         /* 0x80: serial input  */
  memcpy(&np->cmd_buf[1], addr, naddr);
  k = 1U + naddr;
  np->cmd_buf[k] = IMX23_NAND_CMD_PAGEPROG + 0x70U; /* 0x10: prog confirm  */

  ccw_cmd(np, 0U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WRITE |
                     IMX23_GPMI_CTRL0_ADDR_CLE |
                     IMX23_GPMI_CTRL0_ADDR_INCREMENT |
                     IMX23_GPMI_CTRL0_XFER_COUNT(k), np->cmd_buf, k);
  ccw_write_ecc(np, 1U, page_total);
  ccw_cmd(np, 2U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WRITE |
                     IMX23_GPMI_CTRL0_ADDR_CLE |
                     IMX23_GPMI_CTRL0_XFER_COUNT(1U), &np->cmd_buf[k], 1U);
  ccw_bch_off_wait(np, 3U, page_total);
  ccw_end(np, 4U);

  rc = nand_run(np, 5U);
  if (rc != IMX23_NAND_OK) {
    return rc;
  }
  if (!bch_wait_complete()) {
    return IMX23_NAND_ERR_TIMEOUT;
  }

  /* Program status: 0x70 then read one byte; bit0 set = fail. */
  {
    uint8_t st_cmd = IMX23_NAND_CMD_STATUS;
    uint32_t ctrl0 = ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_READ |
                     IMX23_GPMI_CTRL0_ADDR_DATA |
                     IMX23_GPMI_CTRL0_XFER_COUNT(1U);

    ccw_cmd(np, 0U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WRITE |
                       IMX23_GPMI_CTRL0_ADDR_CLE |
                       IMX23_GPMI_CTRL0_XFER_COUNT(1U), &st_cmd, 1U);
    ccw_read_raw(np, 1U, ctrl0, np->data_buf, 1U);
    ccw_end(np, 2U);
    rc = nand_run(np, 3U);
    if (rc != IMX23_NAND_OK) {
      return rc;
    }
    if ((np->data_buf[0] & IMX23_NAND_STATUS_FAIL) != 0U) {
      return IMX23_NAND_ERR_PROG;
    }
  }
  return IMX23_NAND_OK;
}

/**
 * @brief   Raw page read (no ECC): data (page_size) and oob (oob_size) are
 *          read as-is.
 */
int imx23_nand_read_page_raw(IMX23NANDDriver *np, uint32_t page,
                             uint8_t *data, uint8_t *oob) {
  imx23_nand_geo_t *g = &np->geo;
  uint32_t page_total = g->page_size + g->oob_size;
  uint8_t addr[5];
  unsigned naddr, k;
  int rc;

  if (!np->ready || page >= g->pages) {
    return IMX23_NAND_ERR_PARAM;
  }

  naddr = nand_page_addr(np, page, addr);
  np->cmd_buf[0] = IMX23_NAND_CMD_READ0;
  memcpy(&np->cmd_buf[1], addr, naddr);
  k = 1U + naddr;
  np->cmd_buf[k] = IMX23_NAND_CMD_READ0_CONF;

  ccw_cmd(np, 0U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WRITE |
                     IMX23_GPMI_CTRL0_ADDR_CLE |
                     IMX23_GPMI_CTRL0_ADDR_INCREMENT |
                     IMX23_GPMI_CTRL0_XFER_COUNT(k), np->cmd_buf, k);
  ccw_cmd(np, 1U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WRITE |
                     IMX23_GPMI_CTRL0_ADDR_CLE |
                     IMX23_GPMI_CTRL0_XFER_COUNT(1U), &np->cmd_buf[k], 1U);
  ccw_read_raw(np, 2U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_READ |
                       IMX23_GPMI_CTRL0_ADDR_DATA |
                       IMX23_GPMI_CTRL0_XFER_COUNT(page_total),
               np->data_buf, page_total);
  ccw_wait_ready(np, 3U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WAITRDY |
                             IMX23_GPMI_CTRL0_ADDR_DATA);
  ccw_end(np, 4U);

  rc = nand_run(np, 5U);
  if (rc != IMX23_NAND_OK) {
    return rc;
  }
  if (data != NULL) {
    memcpy(data, np->data_buf, g->page_size);
  }
  if (oob != NULL) {
    memcpy(oob, &np->data_buf[g->page_size], g->oob_size);
  }
  return IMX23_NAND_OK;
}

/**
 * @brief   Raw page write (no ECC): data and oob written as-is. The caller
 *          packs data_buf = data(page_size) + oob(oob_size) beforehand via
 *          the data/oob arguments.
 */
int imx23_nand_write_page_raw(IMX23NANDDriver *np, uint32_t page,
                              const uint8_t *data, const uint8_t *oob) {
  imx23_nand_geo_t *g = &np->geo;
  uint32_t page_total = g->page_size + g->oob_size;
  uint8_t addr[5];
  unsigned naddr, k;
  int rc;

  if (!np->ready || page >= g->pages) {
    return IMX23_NAND_ERR_PARAM;
  }

  if (data != NULL) {
    memcpy(np->data_buf, data, g->page_size);
  }
  if (oob != NULL) {
    memcpy(&np->data_buf[g->page_size], oob, g->oob_size);
  }

  naddr = nand_page_addr(np, page, addr);
  np->cmd_buf[0] = IMX23_NAND_CMD_PAGEPROG;
  memcpy(&np->cmd_buf[1], addr, naddr);
  k = 1U + naddr;
  np->cmd_buf[k] = IMX23_NAND_CMD_PAGEPROG + 0x70U;

  ccw_cmd(np, 0U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WRITE |
                     IMX23_GPMI_CTRL0_ADDR_CLE |
                     IMX23_GPMI_CTRL0_ADDR_INCREMENT |
                     IMX23_GPMI_CTRL0_XFER_COUNT(k), np->cmd_buf, k);
  ccw_write_raw(np, 1U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WRITE |
                        IMX23_GPMI_CTRL0_ADDR_DATA |
                        IMX23_GPMI_CTRL0_XFER_COUNT(page_total),
                np->data_buf, page_total);
  ccw_cmd(np, 2U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WRITE |
                     IMX23_GPMI_CTRL0_ADDR_CLE |
                     IMX23_GPMI_CTRL0_XFER_COUNT(1U), &np->cmd_buf[k], 1U);
  ccw_wait_ready(np, 3U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WAITRDY |
                             IMX23_GPMI_CTRL0_ADDR_DATA);
  ccw_end(np, 4U);

  rc = nand_run(np, 5U);
  if (rc != IMX23_NAND_OK) {
    return rc;
  }

  /* Program status. */
  {
    uint8_t st_cmd = IMX23_NAND_CMD_STATUS;
    uint32_t ctrl0 = ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_READ |
                     IMX23_GPMI_CTRL0_ADDR_DATA |
                     IMX23_GPMI_CTRL0_XFER_COUNT(1U);

    ccw_cmd(np, 0U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WRITE |
                       IMX23_GPMI_CTRL0_ADDR_CLE |
                       IMX23_GPMI_CTRL0_XFER_COUNT(1U), &st_cmd, 1U);
    ccw_read_raw(np, 1U, ctrl0, np->data_buf, 1U);
    ccw_end(np, 2U);
    rc = nand_run(np, 3U);
    if (rc != IMX23_NAND_OK) {
      return rc;
    }
    if ((np->data_buf[0] & IMX23_NAND_STATUS_FAIL) != 0U) {
      return IMX23_NAND_ERR_PROG;
    }
  }
  return IMX23_NAND_OK;
}

/**
 * @brief   Raw OOB-only read (NAND READ OOB command 0x50, no ECC).
 */
int imx23_nand_read_oob_raw(IMX23NANDDriver *np, uint32_t page, uint8_t *oob) {
  imx23_nand_geo_t *g = &np->geo;
  uint8_t addr[5];
  unsigned naddr;
  int rc;

  if (!np->ready || page >= g->pages) {
    return IMX23_NAND_ERR_PARAM;
  }

  /* Column = page_size (start of the spare area). */
  {
    uint32_t row = page / g->pages_per_block;
    uint32_t col = g->page_size;

    addr[0] = (uint8_t)(col & 0xFFU);
    addr[1] = (uint8_t)((col >> 8) & 0xFFU);
    addr[2] = (uint8_t)(row & 0xFFU);
    addr[3] = (uint8_t)((row >> 8) & 0xFFU);
    if (g->row_bytes > 2U) {
      addr[4] = (uint8_t)((row >> 16) & 0xFFU);
      naddr = 5U;
    }
    else {
      naddr = 4U;
    }
  }
  np->cmd_buf[0] = IMX23_NAND_CMD_READOOB;
  memcpy(&np->cmd_buf[1], addr, naddr);

  ccw_cmd(np, 0U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WRITE |
                     IMX23_GPMI_CTRL0_ADDR_CLE |
                     IMX23_GPMI_CTRL0_ADDR_INCREMENT |
                     IMX23_GPMI_CTRL0_XFER_COUNT(1U + naddr),
          np->cmd_buf, 1U + naddr);
  ccw_read_raw(np, 1U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_READ |
                       IMX23_GPMI_CTRL0_ADDR_DATA |
                       IMX23_GPMI_CTRL0_XFER_COUNT(g->oob_size),
               np->data_buf, g->oob_size);
  ccw_wait_ready(np, 2U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WAITRDY |
                             IMX23_GPMI_CTRL0_ADDR_DATA);
  ccw_end(np, 3U);

  rc = nand_run(np, 4U);
  if (rc != IMX23_NAND_OK) {
    return rc;
  }
  memcpy(oob, np->data_buf, g->oob_size);
  return IMX23_NAND_OK;
}

/**
 * @brief   Erase one physical erase block.
 */
int imx23_nand_erase_block(IMX23NANDDriver *np, uint32_t block) {
  imx23_nand_geo_t *g = &np->geo;
  uint8_t addr[3];
  unsigned naddr, k;
  int rc;

  if (!np->ready || block >= g->blocks) {
    return IMX23_NAND_ERR_PARAM;
  }

  naddr = nand_row_addr(np, block, addr);
  np->cmd_buf[0] = IMX23_NAND_CMD_ERASE0;
  memcpy(&np->cmd_buf[1], addr, naddr);
  k = 1U + naddr;
  np->cmd_buf[k] = IMX23_NAND_CMD_ERASE_CONF;

  ccw_cmd(np, 0U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WRITE |
                     IMX23_GPMI_CTRL0_ADDR_CLE |
                     IMX23_GPMI_CTRL0_ADDR_INCREMENT |
                     IMX23_GPMI_CTRL0_XFER_COUNT(k), np->cmd_buf, k);
  ccw_cmd(np, 1U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WRITE |
                     IMX23_GPMI_CTRL0_ADDR_CLE |
                     IMX23_GPMI_CTRL0_XFER_COUNT(1U), &np->cmd_buf[k], 1U);
  ccw_wait_ready(np, 2U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WAITRDY |
                             IMX23_GPMI_CTRL0_ADDR_DATA);
  ccw_end(np, 3U);

  rc = nand_run(np, 4U);
  if (rc != IMX23_NAND_OK) {
    return rc;
  }

  /* Erase status: 0x70 + read one byte; bit0 set = fail. */
  {
    uint8_t st_cmd = IMX23_NAND_CMD_STATUS;
    uint32_t ctrl0 = ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_READ |
                     IMX23_GPMI_CTRL0_ADDR_DATA |
                     IMX23_GPMI_CTRL0_XFER_COUNT(1U);

    ccw_cmd(np, 0U, ctrl0_base(np) | IMX23_GPMI_CTRL0_CMD_MODE_WRITE |
                       IMX23_GPMI_CTRL0_ADDR_CLE |
                       IMX23_GPMI_CTRL0_XFER_COUNT(1U), &st_cmd, 1U);
    ccw_read_raw(np, 1U, ctrl0, np->data_buf, 1U);
    ccw_end(np, 2U);
    rc = nand_run(np, 3U);
    if (rc != IMX23_NAND_OK) {
      return rc;
    }
    if ((np->data_buf[0] & IMX23_NAND_STATUS_FAIL) != 0U) {
      return IMX23_NAND_ERR_ERASE;
    }
  }
  return IMX23_NAND_OK;
}

bool imx23_nand_is_bad(IMX23NANDDriver *np, uint32_t block) {

  if (!np->ready || block >= np->geo.blocks) {
    return true;
  }
  return bitmapGet(&np->bbt, block) != 0U;
}

void imx23_nand_mark_bad(IMX23NANDDriver *np, uint32_t block) {

  if (np->ready && block < np->geo.blocks) {
    bitmapSet(&np->bbt, block);
  }
}

/*===========================================================================*/
/* MTD-style logical block device (bad blocks skipped).                      */
/*===========================================================================*/

uint32_t imx23_nand_lblocks(IMX23NANDDriver *np) {

  return np->ready ? np->geo.good_blocks : 0U;
}

static int l2p(IMX23NANDDriver *np, uint32_t lblock, uint32_t *pblock) {

  if (!np->ready || lblock >= np->geo.good_blocks) {
    return IMX23_NAND_ERR_PARAM;
  }
  *pblock = np->l2p[lblock];
  return IMX23_NAND_OK;
}

int imx23_nand_lread(IMX23NANDDriver *np, uint32_t lblock, uint32_t off,
                     uint8_t *buf, uint32_t len) {
  imx23_nand_geo_t *g = &np->geo;
  uint32_t pblock;
  int rc;

  rc = l2p(np, lblock, &pblock);
  if (rc != IMX23_NAND_OK) {
    return rc;
  }
  if (off + len > g->block_size) {
    return IMX23_NAND_ERR_PARAM;
  }

  /* Read the covering pages (ECC-corrected) and copy the requested range. */
  {
    uint32_t first_page = off / g->page_size;
    uint32_t last_page  = (off + len - 1U) / g->page_size;
    uint32_t done = 0U;
    uint32_t p;

    for (p = first_page; p <= last_page && done < len; p++) {
      uint32_t poff, avail;

      rc = imx23_nand_read_page(np, pblock * g->pages_per_block + p,
                                np->data_buf, NULL, NULL);
      if (rc != IMX23_NAND_OK) {
        return rc;
      }
      poff  = (p == first_page) ? (off % g->page_size) : 0U;
      avail = g->page_size - poff;
      if (avail > len - done) {
        avail = len - done;
      }
      memcpy(buf + done, &np->data_buf[poff], avail);
      done += avail;
    }
  }
  return IMX23_NAND_OK;
}

int imx23_nand_lwrite_page(IMX23NANDDriver *np, uint32_t lblock,
                           uint32_t page, const uint8_t *data, uint32_t len) {
  imx23_nand_geo_t *g = &np->geo;
  uint32_t pblock;
  int rc;

  rc = l2p(np, lblock, &pblock);
  if (rc != IMX23_NAND_OK) {
    return rc;
  }
  if (page >= g->pages_per_block || len > g->page_size) {
    return IMX23_NAND_ERR_PARAM;
  }
  return imx23_nand_write_page(np, pblock * g->pages_per_block + page,
                               data, NULL);
}

int imx23_nand_lerase(IMX23NANDDriver *np, uint32_t lblock) {
  uint32_t pblock;
  int rc;

  rc = l2p(np, lblock, &pblock);
  if (rc != IMX23_NAND_OK) {
    return rc;
  }
  rc = imx23_nand_erase_block(np, pblock);
  if (rc == IMX23_NAND_ERR_ERASE) {
    /* Erase failed: mark the block bad in the RAM BBT. */
    imx23_nand_mark_bad(np, pblock);
  }
  return rc;
}

/** @} */
