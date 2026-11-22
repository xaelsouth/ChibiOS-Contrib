/*
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
 * @file    CRCv1/hal_crc_lld.h
 * @brief   Hardware CRC (GPCRC) low-level driver header.
 *
 * @addtogroup CRC
 * @{
 */

#ifndef HAL_CRC_LLD_H
#define HAL_CRC_LLD_H

#if (HAL_USE_CRC == TRUE) || (defined(EFR32_CRC_USE_GPCRC) && (EFR32_CRC_USE_GPCRC == TRUE)) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/* Polynomial types. */
#define EFR32_CRC_POLY_CRC32    GPCRC_CTRL_POLYSEL_CRC32   /**< CRC-32 (IEEE 802.3) */
#define EFR32_CRC_POLY_CRC16    GPCRC_CTRL_POLYSEL_CRC16   /**< CRC-16-CCITT        */

/* Default polynomials. */
#define EFR32_CRC32_POLY        0x04C11DB7UL
#define EFR32_CRC16_POLY        0x1021UL

/* Default init values. */
#define EFR32_CRC32_INIT        0xFFFFFFFFUL
#define EFR32_CRC16_INIT        0xFFFFUL

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

#if !defined(EFR32_CRC_USE_GPCRC) || defined(__DOXYGEN__)
#define EFR32_CRC_USE_GPCRC     FALSE
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

typedef struct {
  uint32_t  poly_sel;   /**< EFR32_CRC_POLY_*. */
  uint32_t  poly;       /**< Polynomial value. */
  uint32_t  init;       /**< Initial CRC value. */
  bool      bitrev;     /**< Reverse bit order of input/output. */
  bool      byterev;    /**< Reverse byte order. */
} CRCLLDConfig;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void crc_lld_init(void);
  void crc_lld_start(const CRCLLDConfig *cfg);
  void crc_lld_stop(void);
  void crc_lld_write32(uint32_t data);
  void crc_lld_write16(uint16_t data);
  void crc_lld_write8(uint8_t data);
  uint32_t crc_lld_read(void);
  uint32_t crc_lld_compute32(const void *data, size_t len, uint32_t poly_sel,
                             uint32_t poly, uint32_t init);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_CRC or EFR32_CRC_USE_GPCRC */

#endif /* HAL_CRC_LLD_H */

/** @} */
