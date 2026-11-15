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
 * @file    w5500_lld.c
 * @brief   W5500 low level MAC driver code.
 *
 * @addtogroup MAC
 * @{
 */

#include "hal.h"

#if HAL_USE_MAC || defined(__DOXYGEN__)

#include "w5500_lld.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Fallback MAC address used when the configuration does not
 *          provide one.
 */
static const uint8_t default_mac_address[] = {0xAA, 0x55, 0x13,
                                              0x37, 0x01, 0x10};

static void w5500_lld_enable_isr(MACDriver *macp);
static void w5500_lld_disable_isr(MACDriver *macp);

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Reads a block of bytes from the chip.
 *
 * @param[in] spi       pointer to the @p SPIDriver object
 * @param[in] address   register/buffer address
 * @param[in] block     block select bits
 * @param[in] buflen    number of bytes to read
 * @param[out] buf      pointer to the read buffer
 *
 * @notapi
 */
static void w5500_read(SPIDriver *spi, io_address_t address, block_select_t block,
                       size_t buflen, void *buf) {

  const uint8_t cmd[3] = {
    W5500_MSB(address),
    W5500_LSB(address),
    block | W5500_RWB_READ
  };

#if ETH_USE_MUTUAL_EXCLUSION == TRUE
  spiAcquireBus(spi);
#endif

  spiSelect(spi);
  spiSend(spi, sizeof(cmd), cmd);
  spiReceive(spi, buflen, buf);
  spiUnselect(spi);

#if ETH_USE_MUTUAL_EXCLUSION == TRUE
  spiReleaseBus(spi);
#endif
}

/**
 * @brief   Writes a block of bytes to the chip.
 *
 * @param[in] spi       pointer to the @p SPIDriver object
 * @param[in] address   register/buffer address
 * @param[in] block     block select bits
 * @param[in] buflen    number of bytes to write
 * @param[in] buf       pointer to the write buffer
 *
 * @notapi
 */
static void w5500_write(SPIDriver *spi, io_address_t address, block_select_t block,
                        size_t buflen, const void *buf) {

  const uint8_t cmd[3] = {
    W5500_MSB(address),
    W5500_LSB(address),
    block | W5500_RWB_WRITE
  };

#if ETH_USE_MUTUAL_EXCLUSION == TRUE
  spiAcquireBus(spi);
#endif

  spiSelect(spi);
  spiSend(spi, sizeof(cmd), cmd);

  if ((buf != NULL) && (buflen > 0)) {
    spiSend(spi, buflen, buf);
  }

  spiUnselect(spi);

#if ETH_USE_MUTUAL_EXCLUSION == TRUE
  spiReleaseBus(spi);
#endif
}

/**
 * @brief   Reads an 8 bits register.
 *
 * @param[in] spi       pointer to the @p SPIDriver object
 * @param[in] address   register address
 * @param[in] block     block select bits
 * @return              The register content.
 *
 * @notapi
 */
static uint8_t w5500_read8(SPIDriver *spi, io_address_t address, block_select_t block) {
  uint8_t ret;

  w5500_read(spi, address, block, sizeof(ret), &ret);
  return ret;
}

/**
 * @brief   Reads a 16 bits register.
 * @details The register is read as a single two bytes SPI frame and the read
 *          is repeated until two consecutive reads return the same value,
 *          as required by the datasheet for the pointer/size registers.
 *
 * @param[in] spi       pointer to the @p SPIDriver object
 * @param[in] address   register address
 * @param[in] block     block select bits
 * @return              The register content, big endian.
 *
 * @notapi
 */
static uint16_t w5500_read16(SPIDriver *spi, io_address_t address, block_select_t block) {
  uint8_t buf[2];
  uint16_t prev = 0, last = 0;

  do {
    prev = last;
    w5500_read(spi, address, block, sizeof(buf), buf);
    last = ((uint16_t)buf[0] << 8) | (uint16_t)buf[1];
  } while (prev != last);

  return last;
}

/**
 * @brief   Writes an 8 bits register.
 *
 * @param[in] spi       pointer to the @p SPIDriver object
 * @param[in] address   register address
 * @param[in] block     block select bits
 * @param[in] val       value to be written
 *
 * @notapi
 */
static void w5500_write8(SPIDriver *spi, io_address_t address, block_select_t block,
                         uint8_t val) {
  w5500_write(spi, address, block, sizeof(val), &val);
}

/**
 * @brief   Writes a 16 bits register.
 * @details The register is written as a single two bytes SPI frame, big
 *          endian.
 *
 * @param[in] spi       pointer to the @p SPIDriver object
 * @param[in] address   register address
 * @param[in] block     block select bits
 * @param[in] val       value to be written
 *
 * @notapi
 */
static void w5500_write16(SPIDriver *spi, io_address_t address, block_select_t block,
                          uint16_t val) {
  const uint8_t buf[2] = {W5500_MSB(val), W5500_LSB(val)};

  w5500_write(spi, address, block, sizeof(buf), buf);
}

/**
 * @brief   Performs a hardware reset of the chip.
 * @note    Called in locked context, the S-class sleep variant is used.
 *
 * @param[in] spicfg    pointer to the @p ETHSPIConfig object
 *
 * @notapi
 */
static void w5500_lld_hw_reset(const ETHSPIConfig *spicfg) {

  palClearLine(spicfg->reset_line);
  chThdSleepS(TIME_MS2I(spicfg->reset_trc_delay_ms));

  palSetLine(spicfg->reset_line);
  chThdSleepS(TIME_MS2I(spicfg->reset_tpl_delay_ms));
}

/**
 * @brief   Verifies that a W5500 chip is present on the SPI bus.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 * @return              The probe result.
 * @retval true         a W5500 chip has been detected.
 * @retval false        no chip or wrong chip detected.
 *
 * @notapi
 */
static bool w5500_lld_probe(MACDriver *macp) {
  SPIDriver *spi = macp->spi_config->driver;

  return w5500_read8(spi, W5500_IO_VERSIONR, W5500P_BSB_COMMON) == W5500_CHIP_ID;
}

/**
 * @brief   Clears all the pending socket interrupts.
 *
 * @param[in] spi       pointer to the @p SPIDriver object
 *
 * @notapi
 */
static void w5500_lld_clear_pending_irqs(SPIDriver *spi) {

  w5500_write8(spi, W5500_IO_SN_IR, W5500P_BSB_SOCKET, W5500_SN_IR_CLEAR_ALL);
}

/**
 * @brief   Sets the source MAC address in the chip.
 *
 * @param[in] spi       pointer to the @p SPIDriver object
 * @param[in] mac       pointer to a six bytes buffer containing the MAC
 *                      address
 *
 * @notapi
 */
static void w5500_lld_set_mac_address(SPIDriver *spi, const uint8_t mac[6]) {

  w5500_write(spi, W5500_IO_SHAR, W5500P_BSB_COMMON, 6, mac);
}

/**
 * @brief   Puts the PHY in power down mode.
 *
 * @param[in] spi       pointer to the @p SPIDriver object
 *
 * @notapi
 */
static void w5500_lld_pwr_down(SPIDriver *spi) {
  uint8_t phycfg;

  phycfg = w5500_read8(spi, W5500_IO_PHYCFGR, W5500P_BSB_COMMON);
  phycfg |= W5500_PHYCFGR_PWR_DOWN;
  w5500_write8(spi, W5500_IO_PHYCFGR, W5500P_BSB_COMMON, phycfg);
}

/**
 * @brief   Sets the socket 0 TX buffer size.
 *
 * @param[in] spi       pointer to the @p SPIDriver object
 * @param[in] buf_size_kb buffer size in Kbytes
 *
 * @notapi
 */
static void w5500_lld_set_tx_buffer_size(SPIDriver *spi, size_t buf_size_kb) {
  w5500_write8(spi, W5500_IO_SN_TXBUF_SIZE, W5500P_BSB_SOCKET, buf_size_kb);
}

/**
 * @brief   Sets the socket 0 RX buffer size.
 *
 * @param[in] spi       pointer to the @p SPIDriver object
 * @param[in] buf_size_kb buffer size in Kbytes
 *
 * @notapi
 */
static void w5500_lld_set_rx_buffer_size(SPIDriver *spi, size_t buf_size_kb) {
  w5500_write8(spi, W5500_IO_SN_RXBUF_SIZE, W5500P_BSB_SOCKET, buf_size_kb);
}

/**
 * @brief   Configures socket 0 in MACRAW mode.
 *
 * @param[in] spi       pointer to the @p SPIDriver object
 *
 * @notapi
 */
static void w5500_lld_set_socket_mode_raw(SPIDriver *spi) {
  uint8_t mode = w5500_read8(spi, W5500_IO_SN_MR, W5500P_BSB_SOCKET);

  mode = (uint8_t)((mode & ~W5500_SN_MR_PROTO_MASK) | W5500_SN_MR_PROTO_MACRAW);
  w5500_write8(spi, W5500_IO_SN_MR, W5500P_BSB_SOCKET, mode);
}

/**
 * @brief   Opens socket 0.
 *
 * @param[in] spi       pointer to the @p SPIDriver object
 *
 * @notapi
 */
static void w5500_lld_open_socket(SPIDriver *spi) {

  w5500_write8(spi, W5500_IO_SN_CR, W5500P_BSB_SOCKET, W5500_SN_CR_OPEN);
}

/**
 * @brief   Closes socket 0.
 *
 * @param[in] spi       pointer to the @p SPIDriver object
 *
 * @notapi
 */
static void w5500_lld_close_socket(SPIDriver *spi) {

  w5500_write8(spi, W5500_IO_SN_CR, W5500P_BSB_SOCKET, W5500_SN_CR_CLOSE);
}

/**
 * @brief   Applies a PHY configuration to the chip.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 * @param[in] chipcfg   pointer to the @p ETHPhyConfig object, if @p NULL the
 *                      configuration in the driver object is used
 *
 * @notapi
 */
static void w5500_lld_set_phy_mode(MACDriver *macp, const ETHPhyConfig *chipcfg) {
  SPIDriver *spi = macp->spi_config->driver;

  uint8_t phycfg = w5500_read8(spi, W5500_IO_PHYCFGR, W5500P_BSB_COMMON);

  if (chipcfg == NULL)
    chipcfg = macp->phy_config;

  if (chipcfg->conf_by == ETH_PHY_CONF_BY_SW) {
    phycfg |= W5500_PHYCFGR_OPMD;
  } else {
    phycfg &= (uint8_t)~W5500_PHYCFGR_OPMD;
  }

  if (chipcfg->duplex == ETH_PHY_DUPLEX_FULL) {
    phycfg |= W5500_PHYCFGR_OPMDC_FULL_DUPLEX;
  } else {
    phycfg &= (uint8_t)~W5500_PHYCFGR_OPMDC_FULL_DUPLEX;
  }

  if (chipcfg->speed == ETH_PHY_SPEED_100) {
    phycfg |= W5500_PHYCFGR_OPMDC_SPEED_100;
  } else {
    phycfg &= (uint8_t)~W5500_PHYCFGR_OPMDC_SPEED_100;
  }

  if (chipcfg->mode == ETH_PHY_MODE_AUTO) {
    phycfg |= W5500_PHYCFGR_OPMDC_AUTONEGOTIATION;
  } else {
    phycfg &= (uint8_t)~W5500_PHYCFGR_OPMDC_AUTONEGOTIATION;
  }

  /* Reset PHY by setting the RST bit to '0' after the user configures this bit as '1'. */
  phycfg &= (uint8_t)~W5500_PHYCFGR_RST;
  w5500_write8(spi, W5500_IO_PHYCFGR, W5500P_BSB_COMMON, phycfg);
  /* And enable again. */
  phycfg |= W5500_PHYCFGR_RST;
  w5500_write8(spi, W5500_IO_PHYCFGR, W5500P_BSB_COMMON, phycfg);
}

/**
 * @brief   Returns the free space in the socket 0 TX buffer.
 *
 * @param[in] spi       pointer to the @p SPIDriver object
 * @return              The number of free bytes, zero if the socket is not
 *                      in MACRAW mode.
 *
 * @notapi
 */
static size_t w5500_lld_get_free_bytes(SPIDriver *spi) {

  if (W5500_SN_SR_SOCK_MACRAW != w5500_read8(spi, W5500_IO_SN_SR, W5500P_BSB_SOCKET))
    return 0;

  return w5500_read16(spi, W5500_IO_SN_TX_FSR, W5500P_BSB_SOCKET);
}

/**
 * @brief   Returns the socket 0 TX write pointer.
 *
 * @param[in] spi       pointer to the @p SPIDriver object
 * @return              The TX write pointer.
 *
 * @notapi
 */
static uint16_t w5500_lld_get_tx_ptr(SPIDriver *spi) {
  return w5500_read16(spi, W5500_IO_SN_TX_WR, W5500P_BSB_SOCKET);
}

/**
 * @brief   Returns the number of bytes received in the socket 0 RX buffer.
 *
 * @param[in] spi       pointer to the @p SPIDriver object
 * @return              The number of received bytes, including the MACRAW
 *                      frame headers.
 *
 * @notapi
 */
static size_t w5500_lld_get_bytes_received(SPIDriver *spi) {
  return w5500_read16(spi, W5500_IO_SN_RX_RSR, W5500P_BSB_SOCKET);
}

/**
 * @brief   Returns the socket 0 RX read pointer.
 *
 * @param[in] spi       pointer to the @p SPIDriver object
 * @return              The RX read pointer.
 *
 * @notapi
 */
static uint16_t w5500_lld_get_rx_ptr(SPIDriver *spi) {
  return w5500_read16(spi, W5500_IO_SN_RX_RD, W5500P_BSB_SOCKET);
}

/**
 * @brief   Advances the socket 0 RX read pointer and issues the RECV command.
 *
 * @param[in] spi       pointer to the @p SPIDriver object
 * @param[in] ptr       current RX read pointer
 * @param[in] len       number of bytes to be skipped
 * @return              The new RX read pointer.
 *
 * @notapi
 */
static uint16_t w5500_lld_skip_rx_bytes(SPIDriver *spi, uint16_t ptr, size_t len) {

  ptr = (uint16_t)(ptr + len);
  w5500_write16(spi, W5500_IO_SN_RX_RD, W5500P_BSB_SOCKET, ptr);
  w5500_write8(spi, W5500_IO_SN_CR, W5500P_BSB_SOCKET, W5500_SN_CR_RECV);

  return ptr;
}

/**
 * @brief   Writes data into the socket 0 TX buffer.
 *
 * @param[in] tdp       pointer to a @p MACTransmitDescriptor structure
 * @param[in] txbuf     pointer to the data to be written
 * @param[in] size      number of bytes to be written
 * @return              The number of bytes written.
 *
 * @notapi
 */
static size_t w5500_lld_write_transmit(MACTransmitDescriptor *tdp, const void *txbuf,
                                       size_t size) {
  SPIDriver *spi = tdp->macp->spi_config->driver;
  uint16_t ptr;

  if (size > tdp->size)
    size = tdp->size;

  if (size > 0) {
    ptr = (uint16_t)tdp->offset;
    w5500_write(spi, ptr, W5500P_BSB_TX_BUF, size, txbuf);

    ptr = (uint16_t)(ptr + size);
    w5500_write16(spi, W5500_IO_SN_TX_WR, W5500P_BSB_SOCKET, ptr);

    tdp->offset = ptr;
    tdp->size = tdp->size - size;
  }

  return size;
}

/**
 * @brief   Reads data from the socket 0 RX buffer.
 *
 * @param[in] rdp       pointer to a @p MACReceiveDescriptor structure
 * @param[out] rxbuf    pointer to the read buffer
 * @param[in] size      number of bytes to be read
 * @return              The number of bytes read.
 *
 * @notapi
 */
static size_t w5500_lld_read_receive(MACReceiveDescriptor *rdp, void *rxbuf, size_t size) {
  SPIDriver *spi = rdp->macp->spi_config->driver;
  uint16_t ptr;

  if (size > rdp->size)
    size = rdp->size;

  if (size > 0) {
    ptr = (uint16_t)rdp->offset;
    w5500_read(spi, ptr, W5500P_BSB_RX_BUF, size, rxbuf);

    rdp->offset = (uint16_t)(ptr + size);
    rdp->size = rdp->size - size;

    w5500_write16(spi, W5500_IO_SN_RX_RD, W5500P_BSB_SOCKET, (uint16_t)rdp->offset);
    w5500_write8(spi, W5500_IO_SN_CR, W5500P_BSB_SOCKET, W5500_SN_CR_RECV);
  }

  return size;
}

/**
 * @brief   Chip initialization and configuration.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 *
 * @notapi
 */
static void w5500_lld_start(MACDriver *macp) {
  SPIDriver *spi = macp->spi_config->driver;
  const uint8_t *mac_address = macp->config->mac_address;

  w5500_lld_hw_reset(macp->spi_config);

  if (!w5500_lld_probe(macp))
    osalSysHalt("W5500 chip not detected");

  w5500_lld_set_phy_mode(macp, NULL);

  if (mac_address == NULL)
    mac_address = default_mac_address;
  w5500_lld_set_mac_address(spi, mac_address);

  w5500_lld_set_tx_buffer_size(spi, W5500_MAC_TX_BUFFER_SIZE);

  w5500_lld_set_rx_buffer_size(spi, W5500_MAC_RX_BUFFER_SIZE);

  w5500_lld_set_socket_mode_raw(spi);

  w5500_lld_open_socket(spi);

  macp->link_up = false;

  w5500_lld_enable_isr(macp);
}

/**
 * @brief   Chip de-initialization.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 *
 * @notapi
 */
static void w5500_lld_stop(MACDriver *macp) {
  SPIDriver *spi = macp->spi_config->driver;

  w5500_lld_disable_isr(macp);

  w5500_lld_close_socket(spi);

  w5500_lld_pwr_down(spi);
}

/**
 * @brief   Updates and returns the link status.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 * @return              The link status.
 * @retval true         if the link is active.
 * @retval false        if the link is down.
 *
 * @notapi
 */
static bool w5500_lld_poll_link_status(MACDriver *macp) {
  SPIDriver *spi = macp->spi_config->driver;

  uint8_t phycfg = w5500_read8(spi, W5500_IO_PHYCFGR, W5500P_BSB_COMMON);

  return macp->link_up = ((phycfg & W5500_PHYCFGR_STATUS_LNK_UP) != 0);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if ETH_USE_CALLBACKS == TRUE
/**
 * @brief   Interrupt handler.
 * @note    The SPI driver has no I-class transfers so the chip interrupt
 *          flags are not read/cleared here, this is done in thread context
 *          by the receive path which re-arms the INT line.
 *
 * @isr
 */
static void w5500_lld_isr(void *arg) {
  MACDriver *macp = (MACDriver *)arg;

  __mac_rx_wakeup(macp);
  __mac_callback(macp);
}

/**
 * @brief   Enables the chip interrupt line.
 * @note    Called in locked context from @p mac_lld_start(), the I-class
 *          PAL API is used.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 *
 * @notapi
 */
static void w5500_lld_enable_isr(MACDriver *macp) {
  SPIDriver *spi = macp->spi_config->driver;

  palSetLineCallbackI(macp->spi_config->irq_line, w5500_lld_isr, macp);
  palEnableLineEventI(macp->spi_config->irq_line, PAL_EVENT_MODE_FALLING_EDGE);

  /* Unmask RECV interrupt. */
  w5500_write8(spi, W5500_IO_SN_IMR, W5500P_BSB_SOCKET, W5500_SN_IMR_RECV);

  /* Clear all pending interrupts. */
  w5500_write8(spi, W5500_IO_SN_IR, W5500P_BSB_SOCKET, W5500_SN_IR_CLEAR_ALL);

  /* Enable RECV interrupt. */
  w5500_write8(spi, W5500_IO_SIMR, W5500P_BSB_COMMON, W5500_SIMR_S0_INT);
}

/**
 * @brief   Disables the chip interrupt line.
 * @note    Called in locked context from @p mac_lld_stop(), the I-class
 *          PAL API is used.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 *
 * @notapi
 */
static void w5500_lld_disable_isr(MACDriver *macp) {
  SPIDriver *spi = macp->spi_config->driver;

  /* Mask interrupts. */
  w5500_write8(spi, W5500_IO_SN_IMR, W5500P_BSB_SOCKET, 0);

  /* Clear all pending interrupts. */
  w5500_write8(spi, W5500_IO_SN_IR, W5500P_BSB_SOCKET, W5500_SN_IR_CLEAR_ALL);

  /* Disable interrupts. */
  w5500_write8(spi, W5500_IO_SIMR, W5500P_BSB_COMMON, 0);

  palDisableLineEventI(macp->spi_config->irq_line);
  palSetLineCallbackI(macp->spi_config->irq_line, NULL, NULL);
}
#else
static void w5500_lld_enable_isr(MACDriver *macp) {
  (void)macp;
}

static void w5500_lld_disable_isr(MACDriver *macp) {
  (void)macp;
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level MAC initialization.
 *
 * @notapi
 */
void mac_lld_init(void) {

#if PLATFORM_MAC_USE_MAC1 == TRUE
  macObjectInit(&ETHD1);
  ETHD1.link_up = false;
#endif
}

/**
 * @brief   Configures and activates the MAC peripheral.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 *
 * @notapi
 */
void mac_lld_start(MACDriver *macp) {

  osalDbgCheck(macp != NULL);

  if (macp->state == MAC_STOP) {
    spiStart(macp->spi_config->driver, macp->spi_config->config);
    w5500_lld_start(macp);
  }
}

/**
 * @brief   Deactivates the MAC peripheral.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 *
 * @notapi
 */
void mac_lld_stop(MACDriver *macp) {

  osalDbgCheck(macp != NULL);

  if (macp->state != MAC_STOP) {
    w5500_lld_stop(macp);
    spiStop(macp->spi_config->driver);
  }
}

/**
 * @brief   Returns a transmission descriptor.
 * @details One of the available transmission descriptors is locked and
 *          returned.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 * @param[out] tdp      pointer to a @p MACTransmitDescriptor structure
 * @return              The operation status.
 * @retval MSG_OK       the descriptor has been obtained.
 * @retval MSG_TIMEOUT  descriptor not available.
 *
 * @notapi
 */
msg_t mac_lld_get_transmit_descriptor(MACDriver *macp,
                                      MACTransmitDescriptor *tdp) {
  SPIDriver *spi;
  size_t bytes_free;

  osalDbgCheck((macp != NULL) && (tdp != NULL));

  if (!macp->link_up)
    return MSG_TIMEOUT;

  spi = macp->spi_config->driver;

  bytes_free = w5500_lld_get_free_bytes(spi);

  if (bytes_free == 0)
    return MSG_TIMEOUT;

  /* A single frame cannot be larger than the maximum frame size. */
  if (bytes_free > W5500_ETH_MAX_FRAME_SIZE)
    bytes_free = W5500_ETH_MAX_FRAME_SIZE;

  tdp->offset = w5500_lld_get_tx_ptr(spi);
  tdp->size   = bytes_free;
  tdp->macp   = macp;

  return MSG_OK;
}

/**
 * @brief   Releases a transmit descriptor and starts the transmission of the
 *          enqueued data as a single frame.
 * @details Waits, with a bounded timeout, for the SEND_OK/TIMEOUT socket
 *          interrupt of the chip.
 *
 * @param[in] tdp       the pointer to the @p MACTransmitDescriptor structure
 *
 * @notapi
 */
void mac_lld_release_transmit_descriptor(MACTransmitDescriptor *tdp) {
  MACDriver *macp;
  SPIDriver *spi;
  systime_t start;

  osalDbgCheck(tdp != NULL);

  macp = tdp->macp;
  spi  = macp->spi_config->driver;

  tdp->offset = 0;
  tdp->size   = 0;
  tdp->macp   = NULL;

  /* Starts the transmission of the frame. */
  w5500_write8(spi, W5500_IO_SN_CR, W5500P_BSB_SOCKET, W5500_SN_CR_SEND);

  /* Bounded wait for the transmission to complete, no sleeping is done
     because this API can be invoked with the system lock held. */
  start = chVTGetSystemTimeX();
  while (osalTimeDiffX(start, chVTGetSystemTimeX()) <
         TIME_MS2I(W5500_MAC_SEND_TIMEOUT_MS)) {
    uint8_t ir = w5500_read8(spi, W5500_IO_SN_IR, W5500P_BSB_SOCKET);

    if (ir & (W5500_SN_IR_SEND_OK | W5500_SN_IR_TIMEOUT)) {
      w5500_write8(spi, W5500_IO_SN_IR, W5500P_BSB_SOCKET,
                   ir & (W5500_SN_IR_SEND_OK | W5500_SN_IR_TIMEOUT));
      return;
    }
  }
}

/**
 * @brief   Returns a receive descriptor.
 * @details The MACRAW per-frame header is read in order to determine the
 *          actual frame length.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 * @param[out] rdp      pointer to a @p MACReceiveDescriptor structure
 * @return              The operation status.
 * @retval MSG_OK       the descriptor has been obtained.
 * @retval MSG_TIMEOUT  descriptor not available.
 *
 * @notapi
 */
msg_t mac_lld_get_receive_descriptor(MACDriver *macp,
                                     MACReceiveDescriptor *rdp) {
  SPIDriver *spi;
  size_t bytes_received;
  uint16_t rx_rd;
  uint8_t header[W5500_RX_FRAME_HEADER_SIZE];
  uint16_t frame_length;

  osalDbgCheck((macp != NULL) && (rdp != NULL));

  spi = macp->spi_config->driver;

  if (W5500_SN_SR_SOCK_MACRAW != w5500_read8(spi, W5500_IO_SN_SR, W5500P_BSB_SOCKET))
    return MSG_TIMEOUT;

  bytes_received = w5500_lld_get_bytes_received(spi);

  if (bytes_received < W5500_RX_FRAME_HEADER_SIZE) {
    /* Spurious interrupt, clear it so that the INT line can re-arm. */
    w5500_lld_clear_pending_irqs(spi);
    return MSG_TIMEOUT;
  }

  /* The MACRAW header carries the real length of the frame. */
  rx_rd = w5500_lld_get_rx_ptr(spi);
  w5500_read(spi, rx_rd, W5500P_BSB_RX_BUF, sizeof(header), header);
  frame_length = (uint16_t)(((uint16_t)header[0] << 8) | (uint16_t)header[1]);

  if ((frame_length == 0) ||
      (frame_length > W5500_ETH_MAX_FRAME_SIZE) ||
      (frame_length > bytes_received - W5500_RX_FRAME_HEADER_SIZE)) {
    /* Corrupt or oversize frame, discard everything queued. */
    w5500_lld_skip_rx_bytes(spi, rx_rd, bytes_received);
    w5500_lld_clear_pending_irqs(spi);
    return MSG_TIMEOUT;
  }

  /* Consume the header, the frame data follows it in the RX buffer. */
  rx_rd = w5500_lld_skip_rx_bytes(spi, rx_rd, W5500_RX_FRAME_HEADER_SIZE);

  rdp->offset = rx_rd;
  rdp->size   = frame_length;
  rdp->macp   = macp;

  return MSG_OK;
}

/**
 * @brief   Releases a receive descriptor.
 * @details The descriptor and its buffer are made available for more incoming
 *          frames. Any unread remainder of the frame is discarded so that
 *          the receive stream stays frame aligned, then the chip interrupt
 *          is cleared re-arming the INT line.
 *
 * @param[in] rdp       the pointer to the @p MACReceiveDescriptor structure
 *
 * @notapi
 */
void mac_lld_release_receive_descriptor(MACReceiveDescriptor *rdp) {
  MACDriver *macp;
  SPIDriver *spi;

  osalDbgCheck(rdp != NULL);

  macp = rdp->macp;
  spi  = macp->spi_config->driver;

  /* Discard the unread remainder of the frame, if any. */
  if (rdp->size > 0) {
    w5500_lld_skip_rx_bytes(spi, (uint16_t)rdp->offset, rdp->size);
  }

  /* Clear the interrupt so that the INT line can re-arm. */
  w5500_lld_clear_pending_irqs(spi);

  rdp->offset = 0;
  rdp->size   = 0;
  rdp->macp   = NULL;
}

/**
 * @brief   Updates and returns the link status.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 * @return              The link status.
 * @retval true         if the link is active.
 * @retval false        if the link is down.
 *
 * @notapi
 */
bool mac_lld_poll_link_status(MACDriver *macp) {

  osalDbgCheck(macp != NULL);

  return w5500_lld_poll_link_status(macp);
}

/**
 * @brief   Writes to a transmit descriptor's stream.
 *
 * @param[in] tdp       pointer to a @p MACTransmitDescriptor structure
 * @param[in] buf       pointer to the buffer containing the data to be
 *                      written
 * @param[in] size      number of bytes to be written
 * @return              The number of bytes written into the descriptor's
 *                      stream, this value can be less than the amount
 *                      specified in the parameter @p size if the maximum
 *                      frame size is reached.
 *
 * @notapi
 */
size_t mac_lld_write_transmit_descriptor(MACTransmitDescriptor *tdp,
                                         uint8_t *buf,
                                         size_t size) {

  osalDbgCheck((tdp != NULL) && (buf != NULL));

  return w5500_lld_write_transmit(tdp, buf, size);
}

/**
 * @brief   Reads from a receive descriptor's stream.
 *
 * @param[in] rdp       pointer to a @p MACReceiveDescriptor structure
 * @param[in] buf       pointer to the buffer that will receive the read data
 * @param[in] size      number of bytes to be read
 * @return              The number of bytes read from the descriptor's
 *                      stream, this value can be less than the amount
 *                      specified in the parameter @p size if there are
 *                      no more bytes to read.
 *
 * @notapi
 */
size_t mac_lld_read_receive_descriptor(MACReceiveDescriptor *rdp,
                                       uint8_t *buf,
                                       size_t size) {

  osalDbgCheck((rdp != NULL) && (buf != NULL));

  return w5500_lld_read_receive(rdp, buf, size);
}

#endif /* HAL_USE_MAC */

/** @} */
