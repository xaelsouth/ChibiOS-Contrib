/*
    W5500 for ChibiOS/RT - Copyright (C) 2023 Xael South

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
 * @file    w5500_lld.h
 * @brief   W5500 low level driver header.
 *
 * @addtogroup WIZNET
 * @{
 */

#ifndef W5500_LLD_H
#define W5500_LLD_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief    W5500 chip ID, value of the VERSIONR register.
 */
#define W5500_CHIP_ID               0x04

/**
 * @brief    Size of the per-frame header prepended by the chip in MACRAW
 *           mode, big endian frame length.
 */
#define W5500_RX_FRAME_HEADER_SIZE  2

/**
 * @name    SPI frame helpers
 * @{
 */
#define W5500_MSB(n)                (((n) >> 8) & 0xFF)  /**< Address/value MSB. */
#define W5500_LSB(n)                (((n) >> 0) & 0xFF)  /**< Address/value LSB. */

#define W5500_RWB_READ              (0x00 << 2) /**< Read access mode bit.  */
#define W5500_RWB_WRITE             (0x01 << 2) /**< Write access mode bit. */
/** @} */

/**
 * @name    Common register block (BSB = 0)
 * @{
 */
#define W5500_IO_MR                 0x0000      /**< Mode register.           */
#define W5500_MR_RST                (1 << 7)    /**< Software reset bit.      */

#define W5500_IO_SNCR               0x0001      /**< Socket n command...      */

#define W5500_IO_SHAR               0x0009      /**< Source hardware address. */

#define W5500_IO_SIMR               0x0018      /**< Socket interrupt mask.   */
#define W5500_SIMR_S0_INT           (1 << 0)    /**< Socket 0 interrupt enable.*/

#define W5500_IO_PHYCFGR            0x002E      /**< PHY configuration.       */
#define W5500_PHYCFGR_RST           (1 << 7)    /**< PHY reset bit.           */
#define W5500_PHYCFGR_OPMD          (1 << 6)    /**< PHY operation mode bit.  */
#define W5500_PHYCFGR_OPMDC_AUTONEGOTIATION (1 << 5) /**< Auto-negotiation.    */
#define W5500_PHYCFGR_OPMDC_SPEED_100       (1 << 4) /**< 100 Mbit/s.          */
#define W5500_PHYCFGR_OPMDC_FULL_DUPLEX     (1 << 3) /**< Full duplex.         */
#define W5500_PHYCFGR_PWR_DOWN      ((1 << 5) | (1 << 4) | (0 << 3))
#define W5500_PHYCFGR_STATUS_DPX_FULL       (1 << 2) /**< Duplex status.       */
#define W5500_PHYCFGR_STATUS_SPD_100        (1 << 1) /**< Speed status.        */
#define W5500_PHYCFGR_STATUS_LNK_UP         (1 << 0) /**< Link status.         */

#define W5500_IO_VERSIONR           0x0039      /**< Chip version register.   */
/** @} */

/**
 * @name    Socket 0 register block (BSB = 1)
 * @{
 */
#define W5500_IO_SN_MR              0x0000      /**< Socket mode register.    */
#define W5500_SN_MR_PROTO_MASK      0x0F
#define W5500_SN_MR_PROTO_TCP       (1 << 0)
#define W5500_SN_MR_PROTO_UDP       (1 << 1)
#define W5500_SN_MR_PROTO_MACRAW    (1 << 2)

#define W5500_IO_SN_CR              0x0001      /**< Socket command register. */
#define W5500_SN_CR_OPEN            0x01
#define W5500_SN_CR_CLOSE           0x10
#define W5500_SN_CR_SEND            0x20
#define W5500_SN_CR_RECV            0x40

#define W5500_IO_SN_IR              0x0002      /**< Socket interrupt flags.  */
#define W5500_SN_IR_CON             (1 << 0)
#define W5500_SN_IR_DISCON          (1 << 1)
#define W5500_SN_IR_RECV            (1 << 2)
#define W5500_SN_IR_TIMEOUT         (1 << 3)
#define W5500_SN_IR_SEND_OK         (1 << 4)
#define W5500_SN_IR_CLEAR_ALL       (W5500_SN_IR_CON | W5500_SN_IR_DISCON | \
                                     W5500_SN_IR_RECV |                     \
                                     W5500_SN_IR_TIMEOUT |                  \
                                     W5500_SN_IR_SEND_OK)

#define W5500_IO_SN_SR              0x0003      /**< Socket status register.  */
#define W5500_SN_SR_SOCK_CLOSED     0x00
#define W5500_SN_SR_SOCK_MACRAW     0x42

#define W5500_IO_SN_RXBUF_SIZE      0x001E      /**< Socket RX buffer size.   */
#define W5500_IO_SN_TXBUF_SIZE      0x001F      /**< Socket TX buffer size.   */
#define W5500_IO_SN_TX_FSR          0x0020      /**< TX free size.            */
#define W5500_IO_SN_TX_WR           0x0024      /**< TX write pointer.        */
#define W5500_IO_SN_RX_RSR          0x0026      /**< RX received size.        */
#define W5500_IO_SN_RX_RD           0x0028      /**< RX read pointer.         */

#define W5500_IO_SN_IMR             0x002C      /**< Socket interrupt mask.   */
#define W5500_SN_IMR_CON            (1 << 0)
#define W5500_SN_IMR_DISCON         (1 << 1)
#define W5500_SN_IMR_RECV           (1 << 2)
#define W5500_SN_IMR_TIMEOUT        (1 << 3)
#define W5500_SN_IMR_SEND_OK        (1 << 4)
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief    Size of an ethernet frame without FCS: MTU + VLAN tag = 1500 + 4.
 */
#if !defined(W5500_ETH_MAX_FRAME_SIZE) || defined(__DOXYGEN__)
#define W5500_ETH_MAX_FRAME_SIZE    1504
#endif

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   Socket 0 TX buffer size in Kbytes.
 * @details Must be a power of two in the range 1..16. The W5500 has 16 Kbytes
 *          of TX memory shared between the 8 sockets, assigning the whole
 *          memory to socket 0 leaves nothing to the other sockets.
 */
#if !defined(W5500_MAC_TX_BUFFER_SIZE) || defined(__DOXYGEN__)
#define W5500_MAC_TX_BUFFER_SIZE    16
#endif

/**
 * @brief   Socket 0 RX buffer size in Kbytes.
 * @details Must be a power of two in the range 1..16. The W5500 has 16 Kbytes
 *          of RX memory shared between the 8 sockets.
 */
#if !defined(W5500_MAC_RX_BUFFER_SIZE) || defined(__DOXYGEN__)
#define W5500_MAC_RX_BUFFER_SIZE    16
#endif

/**
 * @brief   Maximum time to wait for the SEND_OK/TIMEOUT socket interrupt
 *          after issuing a SEND command, in milliseconds.
 */
#if !defined(W5500_MAC_SEND_TIMEOUT_MS) || defined(__DOXYGEN__)
#define W5500_MAC_SEND_TIMEOUT_MS   100
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if (W5500_MAC_TX_BUFFER_SIZE < 1) || (W5500_MAC_TX_BUFFER_SIZE > 16) || \
    ((W5500_MAC_TX_BUFFER_SIZE & (W5500_MAC_TX_BUFFER_SIZE - 1)) != 0)
  #error "W5500_MAC_TX_BUFFER_SIZE must be a power of two in the range 1..16"
#endif

#if (W5500_MAC_RX_BUFFER_SIZE < 1) || (W5500_MAC_RX_BUFFER_SIZE > 16) || \
    ((W5500_MAC_RX_BUFFER_SIZE & (W5500_MAC_RX_BUFFER_SIZE - 1)) != 0)
  #error "W5500_MAC_RX_BUFFER_SIZE must be a power of two in the range 1..16"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a W5500 register/buffer address.
 */
typedef uint16_t io_address_t;

/**
 * @brief   Type of a W5500 block select field.
 */
typedef uint8_t block_select_t;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @name    Block select bits of the SPI control byte
 * @{
 */
#define W5500P_BSB_COMMON           (0x00 << 3) /**< Common registers.   */
#define W5500P_BSB_SOCKET           (0x01 << 3) /**< Socket 0 registers. */
#define W5500P_BSB_TX_BUF           (0x02 << 3) /**< Socket 0 TX buffer. */
#define W5500P_BSB_RX_BUF           (0x03 << 3) /**< Socket 0 RX buffer. */
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#endif /* W5500_LLD_H */

/** @} */
