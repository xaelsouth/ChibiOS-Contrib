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
 * @file    CRCv1/hal_crc_lld.c
 * @brief   Hardware CRC (GPCRC) low-level driver source.
 *
 * @addtogroup CRC
 * @{
 */

#include "hal.h"

#if (HAL_USE_CRC == TRUE) || defined(__DOXYGEN__)

#if EFR32_CRC_USE_GPCRC || defined(__DOXYGEN__)

void crc_lld_init(void) {
  /* Nothing to do at init; clock is enabled on start. */
}

void crc_lld_start(const CRCLLDConfig *cfg) {
  CMU->CLKEN0_SET = CMU_CLKEN0_GPCRC;

  GPCRC->EN_CLR = GPCRC_EN_EN;
  GPCRC->CTRL = 0;
  if (cfg->bitrev)  GPCRC->CTRL_SET = GPCRC_CTRL_BITREVERSE_REVERSED;
  if (cfg->byterev) GPCRC->CTRL_SET = GPCRC_CTRL_BYTEREVERSE_REVERSED;
  GPCRC->CTRL_SET = cfg->poly_sel;
  GPCRC->POLY = cfg->poly;
  GPCRC->INIT = cfg->init;
  GPCRC->EN_SET = GPCRC_EN_EN;
}

void crc_lld_stop(void) {
  GPCRC->EN_CLR = GPCRC_EN_EN;
  CMU->CLKEN0_CLR = CMU_CLKEN0_GPCRC;
}

void crc_lld_write32(uint32_t data) {
  GPCRC->INPUTDATA = data;
}

void crc_lld_write16(uint16_t data) {
  GPCRC->INPUTDATAHWORD = data;
}

void crc_lld_write8(uint8_t data) {
  GPCRC->INPUTDATABYTE = data;
}

uint32_t crc_lld_read(void) {
  return GPCRC->DATA;
}

uint32_t crc_lld_compute32(const void *data, size_t len, uint32_t poly_sel,
                           uint32_t poly, uint32_t init) {
  const CRCLLDConfig cfg = {
    .poly_sel = poly_sel,
    .poly     = poly,
    .init     = init,
    .bitrev   = false,
    .byterev  = false,
  };
  const uint8_t *p = (const uint8_t *)data;

  crc_lld_start(&cfg);
  while (len >= 4) {
    crc_lld_write32(((uint32_t)p[0]) | ((uint32_t)p[1] << 8) |
                    ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24));
    p += 4;
    len -= 4;
  }
  if (len >= 2) {
    crc_lld_write16(((uint16_t)p[0]) | ((uint16_t)p[1] << 8));
    p += 2;
    len -= 2;
  }
  if (len) {
    crc_lld_write8(*p);
  }
  uint32_t result = crc_lld_read();
  crc_lld_stop();
  return result;
}

#endif /* EFR32_CRC_USE_GPCRC */

#endif /* HAL_USE_CRC == TRUE */

/** @} */
