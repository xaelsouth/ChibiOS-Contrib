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
#include "icoll.h"

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

#if (PAL_USE_WAIT == TRUE) || (PAL_USE_CALLBACKS == TRUE) || defined(__DOXYGEN__)
/* One event record per pad per bank (3 banks x 32 pads). */
palevent_t _pal_events[3U * 32U];
/* Shadow of the programmed event mode per pad. */
static uint32_t _pal_event_mode[3U * 32U];
#endif

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

#if (PAL_USE_WAIT == TRUE) || (PAL_USE_CALLBACKS == TRUE) || defined(__DOXYGEN__)

/* GPIO bank IRQ dispatcher (ICOLL vectors 16/17/18 = GPIO0/1/2). */
static bool pal_gpio_irq_handler(unsigned bank) {
  volatile uint32_t *irqstat = (volatile uint32_t *)IMX23_PINCTRL_IRQSTAT(bank);
  uint32_t stat = *irqstat;
  uint32_t pending;

  /* Acknowledge (write-1-clear). */
  *irqstat = stat;

  pending = stat;
  while (pending != 0U) {
    uint32_t pad = 31U - __builtin_clz(pending);
    palevent_t *pep = &_pal_events[bank * 32U + pad];
    uint32_t mode = _pal_event_mode[bank * 32U + pad];

    pending &= ~(1U << pad);

    if (mode == PAL_EVENT_MODE_DISABLED) {
      continue;
    }
#if (PAL_USE_CALLBACKS == TRUE) || defined(__DOXYGEN__)
    if (pep->cb != NULL) {
      pep->cb(pep->arg);
    }
#endif
#if (PAL_USE_WAIT == TRUE) || defined(__DOXYGEN__)
    osalThreadDequeueAllI(&pep->threads, MSG_OK);
#endif
  }

  return false;
}

static bool pal_gpio0_irq(void) { return pal_gpio_irq_handler(0U); }
static bool pal_gpio1_irq(void) { return pal_gpio_irq_handler(1U); }
static bool pal_gpio2_irq(void) { return pal_gpio_irq_handler(2U); }

static bool (*const pal_gpio_handlers[3])(void) = {
  pal_gpio0_irq, pal_gpio1_irq, pal_gpio2_irq
};
#endif /* PAL_USE_WAIT || PAL_USE_CALLBACKS */

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

#if (PAL_USE_WAIT == TRUE) || (PAL_USE_CALLBACKS == TRUE) || defined(__DOXYGEN__)

/**
 * @brief   Enables a pad event (pinctrl GPIO interrupt).
 *
 * @notapi
 */
void _pal_lld_enablepadevent(uint32_t port, uint32_t pad, uint32_t mode) {
  volatile uint32_t *irqen  = (volatile uint32_t *)IMX23_PINCTRL_IRQEN(port);
  volatile uint32_t *irlvl  = (volatile uint32_t *)IMX23_PINCTRL_IRQLEVEL(port);
  volatile uint32_t *irpol  = (volatile uint32_t *)IMX23_PINCTRL_IRQPOL(port);

  if (port > 2U || pad > 31U) {
    return;
  }

  _pal_event_mode[port * 32U + pad] = mode & PAL_EVENT_MODE_EDGES_MASK;

  /* Edge triggered (IRQLEVEL=0); polarity per mode. */
  *irlvl &= ~(1U << pad);
  switch (mode & PAL_EVENT_MODE_EDGES_MASK) {
  case PAL_EVENT_MODE_RISING_EDGE:
    *irpol |= (1U << pad);
    break;
  case PAL_EVENT_MODE_FALLING_EDGE:
    *irpol &= ~(1U << pad);
    break;
  case PAL_EVENT_MODE_BOTH_EDGES:
    /* Both edges: emulate with falling + rising is not possible in HW;
       use falling edge only as a sane default for INT-style lines. */
    *irpol &= ~(1U << pad);
    break;
  default:
    *irqen &= ~(1U << pad);
    return;
  }

  /* Route the bank IRQ to the CPU once. */
  icoll_register_handler(IMX23_IRQ_GPIO0 + port, pal_gpio_handlers[port]);
  icoll_irq_enable(IMX23_IRQ_GPIO0 + port);
  *irqen |= (1U << pad);
}

/**
 * @brief   Disables a pad event.
 *
 * @notapi
 */
void _pal_lld_disablepadevent(uint32_t port, uint32_t pad) {
  volatile uint32_t *irqen = (volatile uint32_t *)IMX23_PINCTRL_IRQEN(port);

  if (port > 2U || pad > 31U) {
    return;
  }
  _pal_event_mode[port * 32U + pad] = PAL_EVENT_MODE_DISABLED;
  *irqen &= ~(1U << pad);
}

#endif /* PAL_USE_WAIT || PAL_USE_CALLBACKS */

#endif /* HAL_USE_PAL == TRUE */

/** @} */
