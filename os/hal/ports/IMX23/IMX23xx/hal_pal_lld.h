/*
    ChibiOS - Copyright (C) 2006-2026 Xael South.

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
 * @file    IMX23/IMX23xx/hal_pal_lld.h
 * @brief   i.MX23 PAL subsystem low level driver header (PINCTRL).
 *
 * @details Ports are the three pinctrl banks: GPIOA = bank 0, GPIOB = bank 1,
 *          GPIOC = bank 2; pads are the pin numbers 0..31.
 *
 * @addtogroup IMX23
 * @{
 */

#ifndef HAL_PAL_LLD_H
#define HAL_PAL_LLD_H

#if (HAL_USE_PAL == TRUE) || defined(__DOXYGEN__)

#include "imx23.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/* Port identifiers (pinctrl banks). */
#define GPIOA                           0U
#define GPIOB                           1U
#define GPIOC                           2U
#define GPIO_NULL                       0xFFU

/* Pads per port (one 32-bit register set per bank). */
#define PAL_IOPORTS_WIDTH               32U

/* Alternate function mode (mux select value 0..3). */
#if !defined(PAL_MODE_ALTERNATE) || defined(__DOXYGEN__)
#define PAL_MODE_ALTERNATE(n)           (0x40U | ((n) & 0x3FU))
#endif
#define PAL_MODE_ALTERNATE_MASK         0x3FU
#define PAL_IS_ALTERNATE(mode)          (((mode) & 0xC0U) == 0x40U)

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !defined(PAL_MODE_INPUT) || defined(__DOXYGEN__)
/* Generic modes from hal_pal.h are used. */
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a PAL configuration structure.
 */
typedef struct {
  /* End of the mandatory fields.*/
} PALConfig;

/**
 * @brief   Type of a port identifier (pinctrl bank number).
 */
typedef uint32_t        ioportid_t;

/* A port/pad pair encoded into an ioline_t (used by the MAC driver config).
   Ports are bank numbers 0..2, so encode as (port << 8) | pad. */
typedef uint32_t ioline_t;
#define PAL_LINE(port, pad)   ((ioline_t)(((uint32_t)(port) << 8) | ((uint32_t)(pad) & 0xFFU)))
#define PAL_PORT(line)        ((ioportid_t)((line) >> 8))
#define PAL_PAD(line)         ((ioportmask_t)((line) & 0xFFU))

/**
 * @brief   Type of a pad mask.
 */
typedef uint32_t        ioportmask_t;

/**
 * @brief   Type of a pad mode.
 */
typedef uint32_t        iomode_t;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

extern const PALConfig pal_default_config;

/* Drive strength encoding for imx23_iomux_set_pad() (4-bit DRIVE field). */
#define IMX23_DRIVE_4MA     0x0U
#define IMX23_DRIVE_8MA     0x1U
#define IMX23_DRIVE_12MA    0x2U
#define IMX23_DRIVE_3V3     (1U << 2)
#define IMX23_DRIVE_PULLUP  (1U << 3)

#ifdef __cplusplus
extern "C" {
#endif
  void _pal_lld_init(const PALConfig *config);
  void pal_lld_setpadmode(uint32_t port, uint32_t pad, uint32_t mode);
  void imx23_iomux_set_pad(uint32_t bank, uint32_t pin, uint32_t func,
                           uint32_t drive);
#ifdef __cplusplus
}
#endif

#define pal_lld_init(config)    _pal_lld_init(config)

/*===========================================================================*/
/* Driver inline functions.                                                  */
/*===========================================================================*/

/**
 * @brief   Reads a pad state.
 */
static inline uint32_t pal_lld_readpad(uint32_t port, uint32_t pad) {

  return (IMX23_REG(IMX23_PINCTRL_DIN(port)) >> pad) & 1U;
}

/**
 * @brief   Sets a pad to a logic high state.
 */
static inline void pal_lld_setpad(uint32_t port, uint32_t pad) {

  IMX23_REG(IMX23_PINCTRL_DOUT(port) + 4U) = 1U << pad;
}

/**
 * @brief   Clears a pad to a logic low state.
 */
static inline void pal_lld_clearpad(uint32_t port, uint32_t pad) {

  IMX23_REG(IMX23_PINCTRL_DOUT(port) + 8U) = 1U << pad;
}

/**
 * @brief   Toggles a pad state.
 */
static inline void pal_lld_togglepad(uint32_t port, uint32_t pad) {

  IMX23_REG(IMX23_PINCTRL_DOUT(port) + 12U) = 1U << pad;
}

/**
 * @brief   Reads a group of bits.
 */
static inline uint32_t pal_lld_readgroup(uint32_t port, uint32_t mask, uint32_t offset) {

  return (IMX23_REG(IMX23_PINCTRL_DIN(port)) & mask) >> offset;
}

/**
 * @brief   Writes a group of bits.
 */
static inline void pal_lld_writegroup(uint32_t port, uint32_t mask, uint32_t offset, uint32_t bits) {
  uint32_t value = (bits << offset) & mask;

  IMX23_REG(IMX23_PINCTRL_DOUT(port) + 4U) = value;         /* set   */
  IMX23_REG(IMX23_PINCTRL_DOUT(port) + 8U) = mask & ~value;  /* clear */
}

/**
 * @brief   Pads group mode setting.
 */
static inline void pal_lld_setgroupmode(uint32_t port, uint32_t mask, uint32_t offset, uint32_t mode) {
  uint32_t p = offset;

  while (mask != 0U) {
    if (mask & 1U) {
      pal_lld_setpadmode(port, p, mode);
    }
    mask >>= 1U;
    p++;
  }
}

#define pal_lld_writepad(port, pad, bit)                                    \
  do {                                                                      \
    if (bit) pal_lld_setpad(port, pad); else pal_lld_clearpad(port, pad);   \
  } while (0)

#define pal_lld_readpadlatch(port, pad)    pal_lld_readpad(port, pad)

#endif /* HAL_USE_PAL == TRUE */

#endif /* HAL_PAL_LLD_H */

/** @} */
