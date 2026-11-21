/*
    ChibiOS - Copyright (C) 2023 Xael South

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
 * @file    hal_mac_lld.h
 * @brief   SPI attached MAC (W5500) low level driver header.
 *
 * @addtogroup MAC
 * @{
 */

#ifndef HAL_MAC_LLD_H
#define HAL_MAC_LLD_H

#if HAL_USE_MAC || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   This implementation does not support the zero-copy mode API.
 * @note    The frame data of an SPI attached chip lives in the chip memory
 *          and must always be copied over the SPI bus, zero-copy is not
 *          possible.
 */
#define MAC_SUPPORTS_ZERO_COPY      FALSE

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    PLATFORM configuration options
 * @{
 */
/**
 * @brief   MAC driver enable switch.
 * @details If set to @p TRUE the support for MAC1 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(PLATFORM_MAC_USE_MAC1) || defined(__DOXYGEN__)
  #define PLATFORM_MAC_USE_MAC1               TRUE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if HAL_USE_SPI == FALSE
  #error "MAC driver requires HAL_USE_SPI"
#endif

#if SPI_USE_WAIT == FALSE
  #error "MAC driver requires SPI_USE_WAIT"
#endif

#if HAL_USE_PAL == FALSE
  #error "MAC driver requires HAL_USE_PAL"
#endif

#define ETH_USE_MUTUAL_EXCLUSION SPI_USE_MUTUAL_EXCLUSION

#define ETH_USE_CALLBACKS        PAL_USE_CALLBACKS

#if (MAC_USE_EVENTS == TRUE) && (ETH_USE_CALLBACKS == FALSE)
  #error "MAC driver requires PAL callbacks (PAL_USE_CALLBACKS) in order to signal MAC events"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a PHY configuration source.
 */
typedef enum {
  ETH_PHY_CONF_BY_HW = 0,     /**< PHY configured by hardware (straps).      */
  ETH_PHY_CONF_BY_SW = 1      /**< PHY configured by software.                */
} eth_phy_conf_t;

/**
 * @brief   Type of a PHY duplex mode.
 */
typedef enum {
  ETH_PHY_DUPLEX_HALF = 0,    /**< Half duplex.                               */
  ETH_PHY_DUPLEX_FULL = 1     /**< Full duplex.                               */
} eth_phy_duplex_t;

/**
 * @brief   Type of a PHY operation mode.
 */
typedef enum {
  ETH_PHY_MODE_MANUAL = 0,    /**< Manual configuration.                      */
  ETH_PHY_MODE_AUTO = 1       /**< Auto-negotiation.                          */
} eth_phy_mode_t;

/**
 * @brief   Type of a PHY link speed.
 */
typedef enum {
  ETH_PHY_SPEED_10 = 0,       /**< 10 Mbit/s.                                 */
  ETH_PHY_SPEED_100 = 1,      /**< 100 Mbit/s.                                */
  ETH_PHY_SPEED_1000 = 2      /**< 1000 Mbit/s.                               */
} eth_phy_speed_t;

/**
 * @brief   Type of a PHY link state.
 */
typedef enum {
  ETH_PHY_LINK_DOWN = 0,      /**< Link down.                                 */
  ETH_PHY_LINK_UP = 1,        /**< Link up.                                   */
  ETH_PHY_LINK_UNKNOWN = -1   /**< Link state unknown.                        */
} eth_phy_link_state_t;

/**
 * @brief   SPI attached MAC driver configuration structure.
 */
typedef struct {
  void *driver;                 /**< @brief SPIDriver used to access the chip. */

  const void *config;           /**< @brief SPIConfig for the chip.            */

  uint16_t reset_trc_delay_ms;  /**< @brief Reset line low time, in ms.        */

  uint16_t reset_tpl_delay_ms;  /**< @brief Reset line release wait, in ms.    */

  ioline_t reset_line;          /**< @brief Hardware reset line.               */

#if ETH_USE_CALLBACKS == TRUE
  ioline_t irq_line;            /**< @brief Interrupt line, if used.           */
#endif
} ETHSPIConfig;

/**
 * @brief   PHY configuration structure.
 */
typedef struct {
  eth_phy_conf_t    conf_by;    /**< @brief Configuration source.              */

  eth_phy_mode_t    mode;       /**< @brief Operation mode.                    */

  eth_phy_speed_t   speed;      /**< @brief Link speed.                        */

  eth_phy_duplex_t  duplex;     /**< @brief Duplex mode.                       */
} ETHPhyConfig;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Low level fields of the MAC driver structure.
 */
/**
 * @brief   Low level fields of the MAC driver structure.
 * @note    The chip-specific fields (@p enc_bank, @p enc_irq_line, used by
 *          the ENC28J60 driver only) are always present so that any mix of
 *          SPI-attached MAC drivers can be compiled into one build and
 *          every MACDriver instance carries its own state (no module
 *          static variables anywhere in the LLDs).
 *          @p enc_bank caches the currently selected register bank,
 *          @p enc_irq_line is the optional INT pin for the RX poll fast
 *          path (0 = none); with ETH_USE_CALLBACKS the framework
 *          irq_line from ETHSPIConfig is used instead.
 */
#define mac_lld_driver_fields                                               \
  const ETHSPIConfig            *spi_config;                                \
  const ETHPhyConfig            *phy_config;                                \
  /* Link status flag.*/                                                    \
  bool                          link_up;                                    \
  uint8_t                       enc_bank;                                   \
  ioline_t                      enc_irq_line;

/**
 * @brief   Low level fields of the MAC configuration structure.
 */
#define mac_lld_config_fields                                               \
  /* MAC address.*/                                                         \
  const uint8_t                 *mac_address;

/**
 * @brief   Low level fields of the MAC transmit descriptor structure.
 */
#define mac_lld_transmit_descriptor_fields                                  \
  MACDriver                     *macp;

/**
 * @brief   Low level fields of the MAC receive descriptor structure.
 */
/**
 * @brief   Low level fields of the MAC receive descriptor structure.
 * @note    @p enc_rx_pkt is the chip buffer address of the packet being
 *          received, @p enc_rx_next its next-packet pointer (ENC28J60
 *          only); both are valid between mac_lld_get_receive_descriptor()
 *          and mac_lld_release_receive_descriptor(). Always present so any
 *          mix of SPI-attached MAC drivers can be compiled into one build.
 */
#define mac_lld_receive_descriptor_fields                                   \
  MACDriver                     *macp;                                      \
  uint16_t                      enc_rx_pkt;                                 \
  uint16_t                      enc_rx_next;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

/**
 * @brief   Ethernet driver 1.
 * @note    Unlike the MCU integrated MAC implementations, this driver does
 *          not instantiate the @p ETHD1 object itself: the SPI and PHY
 *          configuration is part of the driver structure, so the application
 *          must define it, for example:
 *          @code
 *            MACDriver ETHD1 = {
 *              .spi_config = &eth_spi_config,
 *              .phy_config = &eth_phy_config
 *            };
 *          @endcode
 */
#if (PLATFORM_MAC_USE_MAC1 == TRUE) && !defined(__DOXYGEN__)
extern MACDriver ETHD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
void mac_lld_init(void);
void mac_lld_start(MACDriver *macp);
void mac_lld_stop(MACDriver *macp);
msg_t mac_lld_get_transmit_descriptor(MACDriver *macp,
                                      MACTransmitDescriptor *tdp);
void mac_lld_release_transmit_descriptor(MACTransmitDescriptor *tdp);
msg_t mac_lld_get_receive_descriptor(MACDriver *macp,
                                     MACReceiveDescriptor *rdp);
void mac_lld_release_receive_descriptor(MACReceiveDescriptor *rdp);
bool mac_lld_poll_link_status(MACDriver *macp);
size_t mac_lld_write_transmit_descriptor(MACTransmitDescriptor *tdp,
                                         uint8_t *buf,
                                         size_t size);
size_t mac_lld_read_receive_descriptor(MACReceiveDescriptor *rdp,
                                       uint8_t *buf,
                                       size_t size);
#if MAC_SUPPORTS_ZERO_COPY == TRUE || defined(__DOXYGEN__)
uint8_t *mac_lld_get_next_transmit_buffer(MACTransmitDescriptor *tdp,
                                          size_t size,
                                          size_t *sizep);
const uint8_t *mac_lld_get_next_receive_buffer(MACReceiveDescriptor *rdp,
                                               size_t *sizep);
#endif
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_MAC */

#endif /* HAL_MAC_LLD_H */

/** @} */
