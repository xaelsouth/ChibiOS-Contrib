/*
    ChibiOS - Copyright (C) 2026 Giovanni Di Sirio.

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
 * @file    devices_lib/ethernet/enc28j60_hw.h
 * @brief   ENC28J60 register definitions (from Linux enc28j60_hw.h).
 *
 * @addtogroup ETHERNET
 * @{
 */

#ifndef ENC28J60_HW_H
#define ENC28J60_HW_H

/* SPI opcodes. */
#define ENC28J60_READ_CTRL_REG    0x00U
#define ENC28J60_READ_BUF_MEM     0x3AU
#define ENC28J60_WRITE_CTRL_REG   0x40U
#define ENC28J60_WRITE_BUF_MEM    0x7AU
#define ENC28J60_BIT_FIELD_SET    0x80U
#define ENC28J60_BIT_FIELD_CLR    0xA0U
#define ENC28J60_SOFT_RESET       0xFFU

/* Register addresses (bank bits included). */
#define SPRD_MASK                 0xE0U
#define BANK_MASK                 0x60U

/* Bank 0 (ETH). */
#define ERDPTL                    0x00U
#define ERDPTH                    0x01U
#define EWRPTL                    0x02U
#define EWRPTH                    0x03U
#define ETXSTL                    0x04U
#define ETXSTH                    0x05U
#define ETXNDL                    0x06U
#define ETXNDH                    0x07U
#define ERXSTL                    0x08U
#define ERXSTH                    0x09U
#define ERXNDL                    0x0AU
#define ERXNDH                    0x0BU
#define ERXRDPTL                  0x0CU
#define ERXRDPTH                  0x0DU
#define ERXWRPTL                  0x0EU
#define ERXWRPTH                  0x0FU
#define EDMASTL                   0x10U
#define EDMASTH                   0x11U
#define EDMANDL                   0x12U
#define EDMANDH                   0x13U
#define EDMADSTL                  0x14U
#define EDMADSTH                  0x15U
#define EDMACSL                   0x16U
#define EDMACSH                   0x17U

/* Bank 1 (ETH). */
#define EHT0                      0x00U
#define ERXFCON                   0x18U
#define EPKTCNT                   0x19U

/* Bank 2 (MAC). */
#define MACON1                    (0x00U | 0x40U | SPRD_MASK)
#define MACON2                    (0x01U | 0x40U | SPRD_MASK)
#define MACON3                    (0x02U | 0x40U | SPRD_MASK)
#define MACON4                    (0x03U | 0x40U | SPRD_MASK)
#define MABBIPG                   (0x04U | 0x40U | SPRD_MASK)
#define MAIPGL                    (0x06U | 0x40U | SPRD_MASK)
#define MAIPGH                    (0x07U | 0x40U | SPRD_MASK)
#define MACLCON1                  (0x08U | 0x40U | SPRD_MASK)
#define MACLCON2                  (0x09U | 0x40U | SPRD_MASK)
#define MAMXFLL                   (0x0AU | 0x40U | SPRD_MASK)
#define MAMXFLH                   (0x0BU | 0x40U | SPRD_MASK)
#define MICMD                     (0x12U | 0x40U | SPRD_MASK)
#define MIREGADR                  (0x14U | 0x40U | SPRD_MASK)
#define MIWRL                     (0x16U | 0x40U | SPRD_MASK)
#define MIWRH                     (0x17U | 0x40U | SPRD_MASK)
#define MIRDL                     (0x18U | 0x40U | SPRD_MASK)
#define MIRDH                     (0x19U | 0x40U | SPRD_MASK)

/* Bank 3 (MAC). */
#define MAADR1                    (0x00U | 0x60U | SPRD_MASK)
#define MAADR0                    (0x01U | 0x60U | SPRD_MASK)
#define MAADR3                    (0x02U | 0x60U | SPRD_MASK)
#define MAADR2                    (0x03U | 0x60U | SPRD_MASK)
#define MAADR5                    (0x04U | 0x60U | SPRD_MASK)
#define MAADR4                    (0x05U | 0x60U | SPRD_MASK)
#define MISTAT                    (0x0AU | 0x60U | SPRD_MASK)
#define EREVID                    (0x12U | 0x60U)

/* Common (all banks). */
#define EIE                       0x1BU
#define EIR                       0x1CU
#define ESTAT                     0x1DU
#define ECON2                     0x1EU
#define ECON1                     0x1FU

/* PHY registers. */
#define PHCON1                    0x00U
#define PHSTAT1                   0x01U
#define PHSTAT2                   0x11U
#define PHCON2                    0x10U
#define PHIE                      0x12U
#define PHIR                      0x13U
#define PHLCON                    0x14U

/* EIE/EIR bits. */
#define EIE_INTIE                 0x80U
#define EIE_PKTIE                 0x40U
#define EIE_LINKIE                0x10U
#define EIE_TXIE                  0x08U
#define EIE_TXERIE                0x02U
#define EIE_RXERIE                0x01U
#define EIR_PKTIF                 0x40U
#define EIR_LINKIF                0x10U
#define EIR_TXIF                  0x08U
#define EIR_TXERIF                0x02U
#define EIR_RXERIF                0x01U

/* ESTAT bits. */
#define ESTAT_INT                 0x80U
#define ESTAT_LATECOL             0x10U
#define ESTAT_RXBUSY              0x04U
#define ESTAT_TXABRT              0x02U
#define ESTAT_CLKRDY              0x01U

/* ECON2 bits. */
#define ECON2_AUTOINC             0x80U
#define ECON2_PKTDEC              0x40U
#define ECON2_PWRSV               0x20U
#define ECON2_VRPS                0x08U

/* ECON1 bits. */
#define ECON1_TXRST               0x80U
#define ECON1_RXRST               0x40U
#define ECON1_DMAST               0x20U
#define ECON1_CSUMEN              0x10U
#define ECON1_TXRTS               0x08U
#define ECON1_RXEN                0x04U
#define ECON1_BSEL1               0x02U
#define ECON1_BSEL0               0x01U

/* MACON1/2/3 bits. */
#define MACON1_MARXEN             0x01U
#define MACON1_TXPAUS             0x08U
#define MACON1_RXPAUS             0x04U
#define MACON3_PADCFG0            0x20U
#define MACON3_TXCRCEN            0x10U
#define MACON3_FRMLNEN            0x02U
#define MACON3_FULDPX             0x01U

/* ERXFCON bits. */
#define ERXFCON_UCEN              0x80U
#define ERXFCON_CRCEN             0x20U
#define ERXFCON_BCEN              0x01U

/* MICMD / MISTAT bits. */
#define MICMD_MIIRD               0x01U
#define MICMD_MIISCAN             0x02U
#define MISTAT_BUSY               0x01U
#define MISTAT_SCAN               0x02U
#define MISTAT_NVALID             0x04U

/* PHY bits. */
#define PHCON1_PDPXMD             0x0100U
#define PHSTAT2_LSTAT             (1U << 10)
#define PHCON2_HDLDIS             0x0100U
#define PHIE_PGEIE                0x0200U
#define PHIE_PLNKIE               0x0010U

/* Buffer layout (Linux enc28j60). */
#define ENC28J60_TXSTART          0x1A00U
#define ENC28J60_TXEND            0x1FFFU
#define ENC28J60_RXSTART          0x0000U
#define ENC28J60_RXEND            0x19FFU
#define ENC28J60_MAX_FRAMELEN     1518U
#define ENC28J60_RSV_SIZE         6U      /* next(2) + len(2) + status(2) */
#define ENC28J60_LAMPS_MODE       0x3476U

#endif /* ENC28J60_HW_H */

/** @} */
