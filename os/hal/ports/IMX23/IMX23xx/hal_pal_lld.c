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
 * @file    IMX23/IMX23xx/hal_pal_lld.c
 * @brief   i.MX23 PAL subsystem low level driver code (PINCTRL).
 *
 * @addtogroup IMX23
 * @{
 */

#include "hal.h"

#if (HAL_USE_PAL == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   Default PAL configuration.
 */
const PALConfig pal_default_config = {
  /* End of the mandatory fields.*/
};

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   PAL subsystem initialization.
 * @note    The board code programs the pad mux/drive for the used
 *          peripherals; here only the pinctrl block clock is ungated.
 *
 * @param[in] config    architecture-dependent configuration
 *
 * @notapi
 */
void _pal_lld_init(const PALConfig *config) {

  (void)config;

  /* Ungate the pinctrl block clock (CLKGATE bit in PINCTRL_CTRL). */
  IMX23_REG_CLR(IMX23_PINCTRL_CTRL, 1U << 30);
}

/**
 * @brief   Programs a pad mode.
 *
 * @param[in] port      port (bank) identifier
 * @param[in] pad       pad number 0..31
 * @param[in] mode      pad mode (PAL_MODE_*), PAL_MODE_ALTERNATE(n) selects
 *                      the peripheral function n
 *
 * @notapi
 */
void pal_lld_setpadmode(uint32_t port, uint32_t pad, uint32_t mode) {
  volatile uint32_t *muxsel = (volatile uint32_t *)(uintptr_t)
      IMX23_PINCTRL_MUXSEL(port, pad);
  uint32_t shift = IMX23_PINCTRL_MUXSEL_SHIFT(pad);
  uint32_t func;

  if (PAL_IS_ALTERNATE(mode)) {
    func = mode & PAL_MODE_ALTERNATE_MASK;
  }
  else {
    func = IMX23_PINCTRL_MUXSEL_GPIO;
  }

  /* Set the mux function. */
  *muxsel = (*muxsel & ~(0x3U << shift)) | (func << shift);

  /* Direction: output enable for push-pull/open-drain, input otherwise. */
  if ((mode == PAL_MODE_OUTPUT_PUSHPULL) || (mode == PAL_MODE_OUTPUT_OPENDRAIN)) {
    IMX23_REG(IMX23_PINCTRL_DOE(port) + 4U) = 1U << pad;
  }
  else {
    IMX23_REG(IMX23_PINCTRL_DOE(port) + 8U) = 1U << pad;
  }
}

/**
 * @brief   Configures a pad: mux function and drive strength/voltage/pull.
 * @note    Used by board code to apply the static pinmux table (ported from
 *          the U-Boot mxs iomux setup).
 *
 * @param[in] bank      pinctrl bank (0..2)
 * @param[in] pin       pin 0..31
 * @param[in] func      mux function 0..3 (3 = GPIO)
 * @param[in] drive     IMX23_DRIVE_* combination
 *
 * @notapi
 */
void imx23_iomux_set_pad(uint32_t bank, uint32_t pin, uint32_t func,
                         uint32_t drive) {
  volatile uint32_t *muxsel = (volatile uint32_t *)(uintptr_t)
      IMX23_PINCTRL_MUXSEL(bank, pin);
  volatile uint32_t *drv = (volatile uint32_t *)(uintptr_t)
      IMX23_PINCTRL_DRIVE(bank, pin);
  uint32_t mshift = IMX23_PINCTRL_MUXSEL_SHIFT(pin);
  uint32_t dshift = IMX23_PINCTRL_DRIVE_SHIFT(pin);

  *muxsel = (*muxsel & ~(0x3U << mshift)) | ((func & 0x3U) << mshift);
  *drv    = (*drv & ~(0xFU << dshift)) | ((drive & 0xFU) << dshift);
}

#endif /* HAL_USE_PAL == TRUE */

/** @} */
