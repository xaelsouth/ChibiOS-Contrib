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
 * @file    devices_lib/ethernet/enc28j60_lld.c
 * @brief   ENC28J60 SPI-attached Ethernet MAC low level driver
 *          (ported from Linux drivers/net/ethernet/microchip/enc28j60.c).
 *
 * @addtogroup ETHERNET
 * @{
 */

#include "hal.h"
#include "hal_mac.h"
#include "enc28j60_hw.h"

#if (HAL_USE_MAC == TRUE) && (PLATFORM_MAC_USE_MAC1 == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

#define ENC_TIMEOUT_MS           100U

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

static uint8_t  enc_bank;        /* currently selected register bank      */
static uint16_t enc_rx_pkt;      /* address of the packet being received  */
static uint16_t enc_rx_next;     /* next-packet pointer of that packet    */
static ioline_t enc_irq_line;    /* optional INT pin (active low), 0=none */

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

static SPIDriver *enc_spi(MACDriver *macp) {

  return (SPIDriver *)macp->spi_config->driver;
}

/* Raw SPI transaction with CS handling. */
static void enc_xfer(MACDriver *macp, const uint8_t *tx, uint8_t *rx,
                     size_t n) {
  SPIDriver *spi = enc_spi(macp);
  const SPIConfig *cfg = (const SPIConfig *)macp->spi_config->config;

  spiSelect(spi);
  spiExchange(spi, n, tx, rx);
  spiUnselect(spi);
  (void)cfg;
}

/* Set the register bank if needed (EIE..ECON1 are common to all banks). */
static void enc_set_bank(MACDriver *macp, uint8_t addr) {
  uint8_t b = (addr & BANK_MASK) >> 5;
  uint8_t cmd[2];

  if ((addr >= EIE) && (addr <= ECON1)) {
    return;
  }
  if ((b & ECON1_BSEL0) != (enc_bank & ECON1_BSEL0)) {
    cmd[0] = (b & ECON1_BSEL0) ? (ENC28J60_BIT_FIELD_SET | ECON1)
                               : (ENC28J60_BIT_FIELD_CLR | ECON1);
    cmd[1] = ECON1_BSEL0;
    enc_xfer(macp, cmd, NULL, 2U);
  }
  if ((b & ECON1_BSEL1) != (enc_bank & ECON1_BSEL1)) {
    cmd[0] = (b & ECON1_BSEL1) ? (ENC28J60_BIT_FIELD_SET | ECON1)
                               : (ENC28J60_BIT_FIELD_CLR | ECON1);
    cmd[1] = ECON1_BSEL1;
    enc_xfer(macp, cmd, NULL, 2U);
  }
  enc_bank = b;
}

static uint8_t enc_read8(MACDriver *macp, uint8_t addr) {
  uint8_t tx[3] = { ENC28J60_READ_CTRL_REG | addr, 0xFFU, 0xFFU };
  uint8_t rx[3];

  enc_set_bank(macp, addr);
  enc_xfer(macp, tx, rx, 3U);
  /* ETH registers (>= 0x1B) have no dummy byte, MAC/MII have one. */
  return (addr >= EIE) ? rx[1] : rx[2];
}

static uint16_t enc_read16(MACDriver *macp, uint8_t addr) {
  uint8_t tx[4] = { ENC28J60_READ_CTRL_REG | addr, 0xFFU, 0xFFU, 0xFFU };
  uint8_t rx[4];
  unsigned off = (addr >= EIE) ? 1U : 2U;

  enc_set_bank(macp, addr);
  enc_xfer(macp, tx, rx, 4U);
  return (uint16_t)rx[off] | ((uint16_t)rx[off + 1U] << 8);
}

static void enc_write8(MACDriver *macp, uint8_t addr, uint8_t val) {
  uint8_t tx[2] = { ENC28J60_WRITE_CTRL_REG | addr, val };

  enc_set_bank(macp, addr);
  enc_xfer(macp, tx, NULL, 2U);
}

static void enc_write16(MACDriver *macp, uint8_t addr, uint16_t val) {
  uint8_t tx[3] = { ENC28J60_WRITE_CTRL_REG | addr, (uint8_t)val,
                    (uint8_t)(val >> 8) };

  enc_set_bank(macp, addr);
  enc_xfer(macp, tx, NULL, 3U);
}

static void enc_bfs(MACDriver *macp, uint8_t addr, uint8_t mask) {
  uint8_t tx[2] = { ENC28J60_BIT_FIELD_SET | addr, mask };

  enc_set_bank(macp, addr);
  enc_xfer(macp, tx, NULL, 2U);
}

static void enc_bfc(MACDriver *macp, uint8_t addr, uint8_t mask) {
  uint8_t tx[2] = { ENC28J60_BIT_FIELD_CLR | addr, mask };

  enc_set_bank(macp, addr);
  enc_xfer(macp, tx, NULL, 2U);
}

#if ETH_USE_CALLBACKS == TRUE
/**
 * @brief   INT pin interrupt handler (falling edge).
 * @isr
 */
static void enc28j60_isr(void *arg) {
  MACDriver *macp = (MACDriver *)arg;

  __mac_callback(macp);
}
#endif

/* PHY register access. */
static void enc_phy_write(MACDriver *macp, uint8_t addr, uint16_t val) {
  unsigned i;

  enc_write8(macp, MIREGADR, addr);
  enc_write16(macp, MIWRL, val);
  /* Wait for the MIIM to complete. */
  for (i = 0U; i < ENC_TIMEOUT_MS; i++) {
    if ((enc_read8(macp, MISTAT) & MISTAT_BUSY) == 0U) {
      return;
    }
    chThdSleepMilliseconds(1);
  }
}

static uint16_t enc_phy_read(MACDriver *macp, uint8_t addr) {
  unsigned i;

  enc_write8(macp, MIREGADR, addr);
  enc_bfs(macp, MICMD, MICMD_MIIRD);
  for (i = 0U; i < ENC_TIMEOUT_MS; i++) {
    if ((enc_read8(macp, MISTAT) & MISTAT_BUSY) == 0U) {
      break;
    }
    chThdSleepMilliseconds(1);
  }
  enc_bfc(macp, MICMD, MICMD_MIIRD);
  return enc_read16(macp, MIRDL);
}

/* Read n bytes from the chip buffer memory starting at addr. */
static void enc_read_buf(MACDriver *macp, uint16_t addr, uint8_t *buf,
                         size_t n) {
  uint8_t hdr = ENC28J60_READ_BUF_MEM;
  SPIDriver *spi = enc_spi(macp);

  enc_write16(macp, ERDPTL, addr);
  spiSelect(spi);
  spiSend(spi, 1U, &hdr);
  spiReceive(spi, n, buf);
  spiUnselect(spi);
}

/* Write n bytes to the chip buffer memory (EWRPT must be set). */
static void enc_write_buf(MACDriver *macp, const uint8_t *buf, size_t n) {
  uint8_t hdr = ENC28J60_WRITE_BUF_MEM;
  SPIDriver *spi = enc_spi(macp);

  spiSelect(spi);
  spiSend(spi, 1U, &hdr);
  spiSend(spi, n, buf);
  spiUnselect(spi);
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

void mac_lld_init(void) {

  enc_bank = 0U;
  enc_irq_line = 0U;
}

/**
 * @brief   Optional: route the ENC28J60 INT pin (active low) so the RX
 *          poll can short-circuit without SPI traffic. When the MAC
 *          events/PAL callbacks are enabled, the framework's
 *          @p irq_line in ETHSPIConfig is used instead.
 *
 * @notapi
 */
void enc28j60_set_irq_line(ioline_t line) {

  enc_irq_line = line;
}

void mac_lld_start(MACDriver *macp) {
  const uint8_t *mac = macp->config->mac_address;
  ioline_t rst = macp->spi_config->reset_line;
  uint8_t tx[2];
  unsigned i;

  /* Hardware reset pulse on the RESET line, if one is configured;
     otherwise fall back to the SPI soft-reset command. */
  if (rst != 0U) {
    palSetLineMode(rst, PAL_MODE_OUTPUT_PUSHPULL);
    palClearLine(rst);                              /* RESET low          */
    chThdSleepMilliseconds(macp->spi_config->reset_trc_delay_ms);
    palSetLine(rst);                                /* RESET high         */
    chThdSleepMilliseconds(macp->spi_config->reset_tpl_delay_ms);
  }
  else {
    /* Soft reset over SPI (no RESET pin wired). */
    tx[0] = ENC28J60_SOFT_RESET;
    tx[1] = ENC28J60_SOFT_RESET;
    enc_xfer(macp, tx, NULL, 2U);
    chThdSleepMilliseconds(2);
  }

  /* INT pin as input (active low, with pull-up) when provided. */
  if (enc_irq_line != 0U) {
    palSetLineMode(enc_irq_line, PAL_MODE_INPUT_PULLUP);
  }

#if ETH_USE_CALLBACKS == TRUE
  /* IRQ-driven mode: register the framework irq_line on the INT pin
     (falling edge), same as the w5500 driver does. */
  if (macp->spi_config->irq_line != 0U) {
    enc_irq_line = macp->spi_config->irq_line;
    palSetLineMode(enc_irq_line, PAL_MODE_INPUT_PULLUP);
    palSetLineCallbackI(enc_irq_line, enc28j60_isr, macp);
    palEnableLineEventI(enc_irq_line, PAL_EVENT_MODE_FALLING_EDGE);
  }
#endif

  /* Wait for the clock to become ready (errata: also just wait). */
  for (i = 0U; i < ENC_TIMEOUT_MS; i++) {
    if ((enc_read8(macp, ESTAT) & ESTAT_CLKRDY) != 0U) {
      break;
    }
    chThdSleepMilliseconds(1);
  }
  enc_bank = 0U;

  enc_write8(macp, ECON1, 0x00U);

  /* Enable address auto-increment and voltage regulator powersave. */
  enc_write8(macp, ECON2, ECON2_AUTOINC | ECON2_VRPS);

  /* RX/TX buffer layout. */
  enc_write16(macp, ERXSTL, ENC28J60_RXSTART);
  enc_write16(macp, ERXNDL, ENC28J60_RXEND);
  enc_write16(macp, ERXRDPTL, ENC28J60_RXEND);   /* read pointer = end (wrap) */
  enc_write16(macp, ETXSTL, ENC28J60_TXSTART);
  enc_write16(macp, ETXNDL, ENC28J60_TXEND);

  /* Receive filter: unicast + broadcast, CRC valid. */
  enc_write8(macp, ERXFCON, ERXFCON_UCEN | ERXFCON_CRCEN | ERXFCON_BCEN);

  /* Enable MAC receive, auto padding + CRC, frame length check, full duplex. */
  enc_write8(macp, MACON1, MACON1_MARXEN | MACON1_TXPAUS | MACON1_RXPAUS);
  enc_write8(macp, MACON3, MACON3_PADCFG0 | MACON3_TXCRCEN |
                            MACON3_FRMLNEN | MACON3_FULDPX);
  enc_write8(macp, MABBIPG, 0x15U);
  enc_write16(macp, MAIPGL, 0x0012U);
  enc_write16(macp, MAMXFLL, ENC28J60_MAX_FRAMELEN);

  /* MAC address (MAADR5..0, little-endian order in the chip). */
  enc_write8(macp, MAADR5, mac[0]);
  enc_write8(macp, MAADR4, mac[1]);
  enc_write8(macp, MAADR3, mac[2]);
  enc_write8(macp, MAADR2, mac[3]);
  enc_write8(macp, MAADR1, mac[4]);
  enc_write8(macp, MAADR0, mac[5]);

  /* LEDs. */
  enc_phy_write(macp, PHLCON, ENC28J60_LAMPS_MODE);
  /* Full duplex PHY. */
  enc_phy_write(macp, PHCON1, PHCON1_PDPXMD);
  enc_phy_write(macp, PHCON2, 0x0000U);

  /* Interrupts + RX enable. */
  enc_phy_write(macp, PHIE, PHIE_PGEIE | PHIE_PLNKIE);
  enc_bfc(macp, EIR, 0xFFU);
  enc_write8(macp, EIE, EIE_INTIE | EIE_PKTIE | EIE_LINKIE |
                         EIE_TXIE | EIE_TXERIE | EIE_RXERIE);
  enc_bfs(macp, ECON1, ECON1_RXEN);
}

void mac_lld_stop(MACDriver *macp) {

  enc_write8(macp, EIE, 0x00U);
  enc_bfc(macp, ECON1, ECON1_RXEN);

#if ETH_USE_CALLBACKS == TRUE
  if (macp->spi_config->irq_line != 0U) {
    palDisableLineEventI(macp->spi_config->irq_line);
    palSetLineCallbackI(macp->spi_config->irq_line, NULL, NULL);
  }
#endif
}

msg_t mac_lld_get_transmit_descriptor(MACDriver *macp,
                                      MACTransmitDescriptor *tdp) {
  unsigned i;

  /* Wait until the previous transmission completed. */
  for (i = 0U; i < ENC_TIMEOUT_MS; i++) {
    if ((enc_read8(macp, ECON1) & ECON1_TXRTS) == 0U) {
      break;
    }
    chThdSleepMilliseconds(1);
  }
  if (i == ENC_TIMEOUT_MS) {
    return MSG_TIMEOUT;
  }

  enc_bfc(macp, EIR, EIR_TXIF | EIR_TXERIF);
  tdp->offset = 0U;
  tdp->size   = ENC28J60_MAX_FRAMELEN;
  tdp->macp   = macp;

  return MSG_OK;
}

void mac_lld_release_transmit_descriptor(MACTransmitDescriptor *tdp) {
  MACDriver *macp = tdp->macp;

  tdp->offset = 0U;
  tdp->size   = 0U;
  tdp->macp   = NULL;

  /* Wait for the frame to be sent. */
  {
    unsigned i;
    for (i = 0U; i < ENC_TIMEOUT_MS; i++) {
      if ((enc_read8(macp, ECON1) & ECON1_TXRTS) == 0U) {
        break;
      }
      chThdSleepMilliseconds(1);
    }
  }
  enc_bfc(macp, EIR, EIR_TXIF | EIR_TXERIF);
}

msg_t mac_lld_get_receive_descriptor(MACDriver *macp,
                                     MACReceiveDescriptor *rdp) {
  uint8_t hdr[ENC28J60_RSV_SIZE];
  uint16_t len;

  /* INT pin fast path: high = no pending interrupt, skip the SPI read. */
  if ((enc_irq_line != 0U) && (palReadLine(enc_irq_line) != PAL_LOW)) {
    return MSG_TIMEOUT;
  }

  if (enc_read8(macp, EPKTCNT) == 0U) {
    return MSG_TIMEOUT;
  }

  enc_rx_pkt = enc_read16(macp, ERXRDPTL);
  enc_read_buf(macp, enc_rx_pkt, hdr, sizeof(hdr));
  enc_rx_next = (uint16_t)hdr[0] | ((uint16_t)hdr[1] << 8);
  len         = (uint16_t)hdr[2] | ((uint16_t)hdr[3] << 8);
  /* hdr[4..5] = receive status; len includes the 4-byte CRC. */
  if (len < 4U) {
    len = 4U;
  }

  rdp->offset = 0U;
  rdp->size   = len - 4U;
  rdp->macp   = macp;

  return MSG_OK;
}

void mac_lld_release_receive_descriptor(MACReceiveDescriptor *rdp) {
  MACDriver *macp = rdp->macp;
  uint16_t next = enc_rx_next;

  /* Wrap the read pointer as the datasheet requires. */
  if (next == ENC28J60_RXSTART) {
    next = ENC28J60_RXEND;
  }
  enc_write16(macp, ERXRDPTL, next);
  enc_bfs(macp, ECON2, ECON2_PKTDEC);

  rdp->offset = 0U;
  rdp->size   = 0U;
  rdp->macp   = NULL;
}

bool mac_lld_poll_link_status(MACDriver *macp) {
  uint16_t st = enc_phy_read(macp, PHSTAT2);

  return ((st & PHSTAT2_LSTAT) != 0U);
}

size_t mac_lld_write_transmit_descriptor(MACTransmitDescriptor *tdp,
                                         uint8_t *buf,
                                         size_t size) {
  MACDriver *macp = tdp->macp;
  uint8_t ctrl = 0x00U;

  if (size > tdp->size) {
    size = tdp->size;
  }

  /* Program the TX window and write control byte + frame. */
  enc_write16(macp, EWRPTL, ENC28J60_TXSTART);
  enc_write16(macp, ETXSTL, ENC28J60_TXSTART);
  enc_write16(macp, ETXNDL, ENC28J60_TXSTART + (uint16_t)size);
  enc_write_buf(macp, &ctrl, 1U);
  enc_write_buf(macp, buf, size);

  /* Kick the transmission. */
  enc_bfs(macp, ECON1, ECON1_TXRTS);

  return size;
}

size_t mac_lld_read_receive_descriptor(MACReceiveDescriptor *rdp,
                                       uint8_t *buf,
                                       size_t size) {

  if (size > rdp->size) {
    size = rdp->size;
  }
  /* Packet data starts right after the 6-byte receive header. */
  enc_read_buf(rdp->macp, enc_rx_pkt + ENC28J60_RSV_SIZE, buf, size);
  rdp->offset += size;
  rdp->size   -= size;

  return size;
}

#endif /* HAL_USE_MAC && PLATFORM_MAC_USE_MAC1 */

/** @} */
