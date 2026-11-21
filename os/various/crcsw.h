/*
    ChibiOS - Copyright (C) 2015 Michael D. Spradling
    ChibiOS - Copyright (C) 2026 Xael South

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
 * @file    crcsw.h
 * @brief   CRC software driver + standalone software CRC algorithms.
 *
 * @details The first part of this file is always available and provides a
 *          generic table-driven CRC engine (crcsw_calc/crcsw_update) with
 *          ready-made configurations for common algorithms (CRC-8, CRC-16
 *          CCITT, CRC-16 IEC 870 FT3 / M-Bus, CRC-16 MODBUS, CRC-32).
 *          The second part is the HAL CRC low level driver (CRCDriver),
 *          compiled only when HAL_USE_CRC is enabled.
 *
 * @addtogroup CRC
 * @{
 */

#ifndef CRCSW_H
#define CRCSW_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Standalone software CRC algorithms (always available).                    */
/*===========================================================================*/

/**
 * @brief   CRC algorithm configuration (Rocksoft^tm model).
 */
typedef struct {
  uint8_t           width;    /**< CRC width in bits: 8, 16 or 32.          */
  uint32_t          poly;     /**< Polynomial, normal (non-reflected) form.  */
  uint32_t          init;     /**< Initial remainder value.                  */
  uint32_t          xorout;   /**< Value XORed with the final remainder.     */
  bool              refin;    /**< Reflect each input byte.                  */
  bool              refout;   /**< Reflect the final remainder.              */
  const uint32_t   *table;    /**< Lookup table (256 entries), or NULL to
                                   use the bitwise fallback.                */
} crcsw_config_t;

/* Ready-made algorithm configurations. */
extern const crcsw_config_t crcsw_crc8_cfg;           /**< CRC-8/SMBUS (0x07).        */
extern const crcsw_config_t crcsw_crc16ccitt_cfg;     /**< CRC-16/CCITT-FALSE (0x1021). */
extern const crcsw_config_t crcsw_crc16iec870ft3_cfg; /**< CRC-16 IEC 870 FT3, M-Bus (0x3D65). */
extern const crcsw_config_t crcsw_crc16modbus_cfg;    /**< CRC-16/MODBUS (0x8005, reflected). */
extern const crcsw_config_t crcsw_crc32_cfg;          /**< CRC-32/ISO-HDLC (0x04C11DB7, reflected). */

/**
 * @brief   Feed a data blob into a running CRC.
 * @note    The @p crc parameter and the return value are the raw remainder
 *          (before final reflection/XOR); start with @p cfg->init.
 *
 * @param[in] cfg       pointer to a @p crcsw_config_t
 * @param[in] crc       current raw remainder
 * @param[in] blob      pointer to the data
 * @param[in] bloblen   length of the data in bytes
 *
 * @return              The updated raw remainder.
 */
uint32_t crcsw_update(const crcsw_config_t *cfg, uint32_t crc,
                      const void *blob, size_t bloblen);

/**
 * @brief   Calculate a CRC over a data blob in one shot.
 *
 * @param[in] cfg       pointer to a @p crcsw_config_t
 * @param[in] blob      pointer to the data
 * @param[in] bloblen   length of the data in bytes
 *
 * @return              The computed CRC value (finalized: reflected and
 *                      XORed as configured, masked to cfg->width bits).
 */
uint32_t crcsw_calc(const crcsw_config_t *cfg, const void *blob, size_t bloblen);

#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* HAL CRC low level driver (software fallback for CRCDriver).               */
/*===========================================================================*/

#include "hal.h"

#if (defined(HAL_USE_CRC) && (HAL_USE_CRC == TRUE)) || defined(__DOXYGEN__)

#if CRCSW_USE_CRC1 || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   CRC1 software driver enable switch.
 * @details If set to @p TRUE the support for CRC1 is included.
 * @note    The default is @p FALSE
 */
#if !defined(CRCSW_USE_CRC1) || defined(__DOXYGEN__)
#define CRCSW_USE_CRC1                  FALSE
#endif

/**
 * @brief Enables software CRC32
 */
#if !defined(CRCSW_CRC32_TABLE) || defined(__DOXYGEN__)
#define CRCSW_CRC32_TABLE               FALSE
#endif

/**
 * @brief Enables software CRC16
 */
#if !defined(CRCSW_CRC16_TABLE) || defined(__DOXYGEN__)
#define CRCSW_CRC16_TABLE               FALSE
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if CRCSW_USE_CRC1 && CRC_USE_DMA
#error "Software CRC does not support DMA(CRC_USE_DMA)"
#endif

#if CRCSW_CRC32_TABLE == FALSE && CRCSW_CRC16_TABLE == FALSE &&                         \
    CRCSW_PROGRAMMABLE == FALSE
#error "At least one of CRCSW_PROGRAMMABLE, CRCSW_CRC32_TABLE, or CRCSW_CRC16_TABLE must be defined"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/
/**
 * @brief   Type of a structure representing an CRC driver.
 */
typedef struct CRCDriver CRCDriver;

/**
 * @brief   Driver configuration structure.
 */
typedef struct {
  /**
   * @brief The size of polynomial to be used for CRC.
   */
  uint32_t                 poly_size;
  /**
   * @brief The coefficients of the polynomial to be used for CRC.
   */
  uint32_t                 poly;
  /**
   * @brief The inital value
   */
  uint32_t                 initial_val;
  /**
   * @brief The final XOR value
   */
  uint32_t                 final_val;
  /**
   * @brief Reflect bit order data going into CRC
   */
  bool                     reflect_data;
  /**
   * @brief Reflect bit order of final remainder
   */
  bool                     reflect_remainder;
  /* End of the mandatory fields.*/
  /**
   * @brief The crc lookup table to use when calculating CRC.
   */
  const uint32_t           *table;
} CRCConfig;


/**
 * @brief   Structure representing an CRC driver.
 */
struct CRCDriver {
  /**
   * @brief Driver state.
   */
  crcstate_t                state;
  /**
   * @brief Current configuration data.
   */
  const CRCConfig           *config;
#if CRC_USE_MUTUAL_EXCLUSION || defined(__DOXYGEN__)
  /**
   * @brief   Mutex protecting the peripheral.
   */
  mutex_t                   mutex;
#endif /* CRC_USE_MUTUAL_EXCLUSION */
  /* End of the mandatory fields.*/
  /**
   * @brief Current value of calculated CRC.
   */
  uint32_t                  crc;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/
#if CRCSW_CRC32_TABLE || defined(__DOXYGEN__)
/**
 * @brief Configuration that represents CRC32
 */
#define CRCSW_CRC32_TABLE_CONFIG (&crcsw_crc32_config)
#endif

#if CRCSW_CRC16_TABLE || defined(__DOXYGEN__)
/**
 * @brief Configuration that represents CRC16
 */
#define CRCSW_CRC16_TABLE_CONFIG (&crcsw_crc16_config)
#endif

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

extern CRCDriver CRCD1;

#if CRCSW_CRC32_TABLE
extern const CRCConfig crcsw_crc32_config;
#endif

#if CRCSW_CRC16_TABLE
extern const CRCConfig crcsw_crc16_config;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void crc_lld_init(void);
  void crc_lld_start(CRCDriver *crcp);
  void crc_lld_stop(CRCDriver *crcp);
  void crc_lld_reset(CRCDriver *crcp);
  uint32_t crc_lld_calc(CRCDriver *crcp, size_t n, const void *buf);
#ifdef __cplusplus
}
#endif

#endif /* CRCSW_USE_CRC1 */

#endif /* HAL_USE_CRC */

#endif /* CRCSW_H */

/** @} */
