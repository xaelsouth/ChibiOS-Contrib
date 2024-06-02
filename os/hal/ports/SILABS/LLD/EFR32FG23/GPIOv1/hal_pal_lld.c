/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

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
 * @file    hal_pal_lld.c
 * @brief   PLATFORM PAL subsystem low level driver source.
 *
 * @addtogroup PAL
 * @{
 */

#include "hal.h"

#if (HAL_USE_PAL == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if (PAL_USE_WAIT == TRUE) || (PAL_USE_CALLBACKS == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Event records for the PAL_EVENTS_SIZE GPIO odd and even channels.
 */
palevent_t _pal_events[PAL_EVENTS_SIZE];
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static inline void _pal_lld_reg_masked_write(volatile uint32_t* address,
                                             uint32_t mask,
                                             uint32_t value) {

  *address = (*address & ~mask) | (value & mask);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if (PAL_USE_WAIT || PAL_USE_CALLBACKS) || defined(__DOXYGEN__)
/**
 * @brief   EFR32_GPIO_ODD_HANDLER interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(EFR32_GPIO_ODD_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  /* Get and clear pending GPIO interrupts. */
  uint32_t flags = GPIO->IF;
  _pal_lld_reg_masked_write(&(GPIO->IF_CLR), 0xAAAUL, flags);

  for (uint32_t pad = 1U; pad < PAL_EVENTS_SIZE; pad += 2U) {
    if ((flags & (1U << pad)) != 0U) {
      _pal_isr_code(pad);
    }
  }

  OSAL_IRQ_EPILOGUE();
}

/**
 * @brief   EFR32_GPIO_ODD_HANDLER interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(EFR32_GPIO_EVEN_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  /* Get and clear pending GPIO interrupts. */
  uint32_t flags = GPIO->IF;
  _pal_lld_reg_masked_write(&(GPIO->IF_CLR), 0x555UL, flags);

  for (uint32_t pad = 0U; pad < PAL_EVENTS_SIZE; pad += 2U) {
    if ((flags & (1U << pad)) != 0U) {
      _pal_isr_code(pad);
    }
  }

  OSAL_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   I/O ports configuration.
 *
 * @notapi
 */
void _pal_lld_init(const PALConfig* config) {

#if PAL_USE_CALLBACKS || PAL_USE_WAIT || defined(__DOXYGEN__)
  int i;

  for (i = 0; i < 16; i++) {
    _pal_init_event(i);
  }
#endif

  /*
   * Enables the GPIO related clocks.
   */
  CMU->CLKEN0_SET = CMU_CLKEN0_GPIO;

  /*
   * Initial GPIO setup.
   */
  GPIOA->CTRL  = config->PAData.ctrlr;
  GPIOA->MODEL = config->PAData.modelr;
  GPIOA->MODEH = config->PAData.modehr;
  GPIOA->DOUT  = config->PAData.doutr;

  GPIOB->CTRL  = config->PBData.ctrlr;
  GPIOB->MODEL = config->PBData.modelr;
  GPIOB->MODEH = config->PBData.modehr;
  GPIOB->DOUT  = config->PBData.doutr;

  GPIOC->CTRL  = config->PCData.ctrlr;
  GPIOC->MODEL = config->PCData.modelr;
  GPIOC->MODEH = config->PCData.modehr;
  GPIOC->DOUT  = config->PCData.doutr;

  GPIOD->CTRL  = config->PDData.ctrlr;
  GPIOD->MODEL = config->PDData.modelr;
  GPIOD->MODEH = config->PDData.modehr;
  GPIOD->DOUT  = config->PDData.doutr;

#if (PAL_USE_WAIT || PAL_USE_CALLBACKS) || defined(__DOXYGEN__)
  nvicEnableVector(EFR32_GPIO_ODD_NUMBER, EFR32_GPIO_ODD_IRQ_PRIORITY);
  nvicEnableVector(EFR32_GPIO_EVEN_NUMBER, EFR32_GPIO_EVEN_IRQ_PRIORITY);
#endif
}

/**
 * @brief   Pads mode setup.
 * @details This function programs a pads group belonging to the same port
 *          with the specified mode.
 *
 * @param[in] port      the port identifier
 * @param[in] mask      the group mask
 * @param[in] mode      the mode
 *
 * @notapi
 */
void _pal_lld_setgroupmode(ioportid_t port,
                           ioportmask_t mask,
                           iomode_t mode) {

  uint32_t pad = 0;

  osalDbgCheck(mask <= PAL_WHOLE_PORT);

  while (mask) {
    if ((mask & 1) != 0) {
      _pal_lld_setpadmode(port, pad, mode);
    }

    mask >>= 1;
    pad++;
  }
}

void _pal_lld_setpadmode(ioportid_t port,
                         iopadid_t pad,
                         iomode_t mode) {

  uint32_t moder = (mode & PAL_EFR32_MODE_MASK) >> PAL_EFR32_MODE_SHIFT;
  uint32_t doutr = (mode & PAL_EFR32_DOUT_MASK) >> PAL_EFR32_DOUT_SHIFT;

  if (pad < 8) {
    _pal_lld_reg_masked_write(&(port->MODEL), 0xFu << ((pad - 0) * 4), moder << ((pad - 0) * 4));
  } else {
    _pal_lld_reg_masked_write(&(port->MODEH), 0xFu << ((pad - 8) * 4), moder << ((pad - 8) * 4));
  }

  pal_lld_writepad(port, pad, (doutr != 0) ? PAL_HIGH : PAL_LOW);
}

#if PAL_USE_CALLBACKS || PAL_USE_WAIT || defined(__DOXYGEN__)
/**
 * @brief   Pad event enable.
 * @note    Programming an unknown or unsupported mode is silently ignored.
 *
 * @param[in] port      port identifier
 * @param[in] pad       pad number within the port
 * @param[in] mode      pad event mode
 *
 * @notapi
 */
void _pal_lld_enablepadevent(ioportid_t port,
                             iopadid_t pad,
                             ioeventmode_t mode) {

  uint32_t int_no = pad;
  uint32_t rising_edge, falling_edge;

  osalDbgCheck(pad < GPIO_PIN_COUNT(port));

  switch (mode & PAL_EVENT_MODE_EDGES_MASK)
  {
    default:
    case PAL_EVENT_MODE_DISABLED:
      rising_edge = 0U;
      falling_edge = 0U;
      break;
    case PAL_EVENT_MODE_RISING_EDGE:
      rising_edge = 1U;
      falling_edge = 0U;
      break;
    case PAL_EVENT_MODE_FALLING_EDGE:
      rising_edge = 0U;
      falling_edge = 1U;
      break;
    case PAL_EVENT_MODE_BOTH_EDGES:
      rising_edge = 1U;
      falling_edge = 1U;
      break;
  }

  if (int_no < 8) {
    /* The EXTIPSELL register controls pins 0-7. */
    _pal_lld_reg_masked_write(&GPIO->EXTIPSELL, 0b11UL << (4U * int_no), GPIO_PORT_INDEX(port) << (4U * int_no));
    /* The EXTIPINSELL register controls interrupt 0-7. */
    _pal_lld_reg_masked_write(&GPIO->EXTIPINSELL, 0b11UL << (4U * int_no), (uint32_t)((pad % 4) & 0b11UL) << (4U * int_no));
  } else {
    uint32_t tmp = int_no - 8;
    /* EXTIPSELH controls pins 8-15 of the interrupt configuration. */
    _pal_lld_reg_masked_write(&GPIO->EXTIPSELH, 0b11UL << (4U * tmp), GPIO_PORT_INDEX(port) << (4U * tmp));
    /* EXTIPINSELH controls interrupt 8-15 of the interrupt/pin number mapping. */
    _pal_lld_reg_masked_write(&GPIO->EXTIPINSELH, 0b11UL << (4U * tmp), (uint32_t)((pad % 4) & 0b11UL) << (4U * tmp));
  }

  /* Enable/disable the rising edge interrupt. */
  _pal_lld_reg_masked_write(&(GPIO->EXTIRISE), (1U << int_no), (rising_edge << int_no));

  /* Enable/disable the falling edge interrupt. */
  _pal_lld_reg_masked_write(&(GPIO->EXTIFALL), (1U << int_no), (falling_edge << int_no));

  /* Clear any pending interrupt. */
  _pal_lld_reg_masked_write(&(GPIO->IF_CLR), (1U << int_no), (1U << int_no));

  /* Finally enable/disable interrupt. */
  _pal_lld_reg_masked_write(&(GPIO->IEN), (1U << int_no), (1U << int_no));
}

/**
 * @brief   Pad event disable.
 * @details This function disables previously programmed event callbacks.
 *
 * @param[in] port      port identifier
 * @param[in] pad       pad number within the port
 *
 * @notapi
 */
void _pal_lld_disablepadevent(ioportid_t port, iopadid_t pad) {

  (void)port;

  uint32_t int_no = pad;

  /* Disable the rising edge interrupt. */
  _pal_lld_reg_masked_write(&(GPIO->EXTIRISE), (1U << int_no), (0U << int_no));

  /* Disable the falling edge interrupt. */
  _pal_lld_reg_masked_write(&(GPIO->EXTIFALL), (1U << int_no), (0U << int_no));

  /* Finally disable interrupt. */
  _pal_lld_reg_masked_write(&(GPIO->IEN), (1U << int_no), (0U << int_no));

  /* Clear any pending interrupt. */
  _pal_lld_reg_masked_write(&(GPIO->IF_CLR), (1U << int_no), (1U << int_no));
}
#endif /* PAL_USE_CALLBACKS || PAL_USE_WAIT */

#endif /* HAL_USE_PAL == TRUE */

/** @} */