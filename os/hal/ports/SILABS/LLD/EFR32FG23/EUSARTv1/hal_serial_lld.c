/*
    ChibiOS - Copyright (C) 2024 Xael South

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
 * @file    EUSARTv1/hal_serial_lld.c
 * @brief   SERIAL Driver subsystem low level driver source.
 *
 * @addtogroup SERIAL
 * @{
 */

#include "hal.h"

#if (HAL_USE_SERIAL == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if (EFR32_SERIAL_USE_USART1 == TRUE) || defined(__DOXYGEN__)
SerialDriver SD1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

static const SerialConfig default_config = {
  .speed = SERIAL_DEFAULT_BITRATE
};

/* USART0 (hardware) -> SD1 (ChibiOS) register/clock mapping. */
#define SERIAL_USART      USART0
#define SERIAL_CLKEN      CMU_CLKEN0_USART0
#define SERIAL_CLK        EFR32_USART1CLK
#define SERIAL_RX_IRQN    EFR32_USART1_RX_NUMBER
#define SERIAL_TX_IRQN    EFR32_USART1_TX_NUMBER
#define SERIAL_RX_PRIO    EFR32_USART1_RX_IRQ_PRIORITY
#define SERIAL_TX_PRIO    EFR32_USART1_TX_IRQ_PRIORITY

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static uint32_t serial_lld_clkdiv(uint32_t clock, uint32_t ovs, uint32_t baud) {
  /* CLKDIV = (clock * 256) / (ovs * baud) - 256, in 20.8 fixed point. */
  uint64_t num = (uint64_t)clock * 256ULL;
  uint64_t den = (uint64_t)ovs * baud;
  uint32_t div = (uint32_t)(num / den) - 256U;
  return div << _USART_CLKDIV_DIV_SHIFT;
}

__STATIC_INLINE void serial_lld_set_error(SerialDriver *sdp, uint32_t iflags) {
  if (iflags & USART_IF_RXOF)  chnAddFlagsI(sdp, SD_OVERRUN_ERROR);
  if (iflags & USART_IF_PERR)  chnAddFlagsI(sdp, SD_PARITY_ERROR);
  if (iflags & USART_IF_FERR)  chnAddFlagsI(sdp, SD_FRAMING_ERROR);
}

/*===========================================================================*/
/* Driver interrupt handlers (dispatched from hal_eusart_irq.c).             */
/*===========================================================================*/

void sd_lld_serve_interrupt(SerialDriver *sdp) {
  USART_TypeDef *u = (USART_TypeDef *)sdp->usart;
  uint32_t isr = u->IF;

  /* RX data available or errors. */
  if (isr & (USART_IF_RXDATAV | USART_IF_RXFULL | USART_IF_RXOF |
             USART_IF_PERR | USART_IF_FERR)) {
    osalSysLockFromISR();
    if (isr & (USART_IF_RXOF | USART_IF_PERR | USART_IF_FERR))
      serial_lld_set_error(sdp, isr);
    if (isr & (USART_IF_RXDATAV | USART_IF_RXFULL)) {
      uint8_t b = (uint8_t)(u->RXDATA & 0xFFU);
      sdIncomingDataI(sdp, b);
    }
    osalSysUnlockFromISR();
  }

  /* TX buffer level (room for more data). */
  if ((u->IEN & USART_IEN_TXBL) && (isr & USART_IF_TXBL)) {
    msg_t b;
    osalSysLockFromISR();
    b = oqGetI(&sdp->oqueue);
    if (b < MSG_OK) {
      chnAddFlagsI(sdp, CHN_OUTPUT_EMPTY);
      u->IEN_CLR = USART_IEN_TXBL;
    } else {
      u->TXDATA = (uint32_t)b;
    }
    osalSysUnlockFromISR();
  }

  /* TX complete. */
  if ((u->IEN & USART_IEN_TXC) && (isr & USART_IF_TXC)) {
    osalSysLockFromISR();
    if (oqIsEmptyI(&sdp->oqueue)) {
      chnAddFlagsI(sdp, CHN_TRANSMISSION_END);
      u->IEN_CLR = USART_IEN_TXC;
    }
    osalSysUnlockFromISR();
  }

  /* Clear handled flags. */
  u->IF_CLR = isr;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void sd_lld_init(void) {
#if EFR32_SERIAL_USE_USART1 == TRUE
  sdObjectInit(&SD1, NULL, NULL);
  SD1.usart = (void *)SERIAL_USART;
#endif
}

void sd_lld_start(SerialDriver *sdp, const SerialConfig *config) {
  USART_TypeDef *u = (USART_TypeDef *)sdp->usart;

  if (config == NULL)
    config = &default_config;

  if (sdp->state == SD_STOP) {
    /* Enable bus clock. */
    CMU->CLKEN0_SET = SERIAL_CLKEN;

    /* Reset and configure. */
    u->EN_CLR = USART_EN_EN;
    while ((u->EN & USART_EN_DISABLING) == USART_EN_DISABLING) {}

    u->CFG = 0;  /* default async */
    u->FRAMECFG = USART_FRAME_DATABITS_EIGHT | USART_FRAME_PARITY_EVEN |
                  USART_FRAME_STOPBITS_ONE;
    u->CLKDIV = serial_lld_clkdiv(SERIAL_CLK, 16, config->speed);

    /* IrDA fields present in SerialConfig for API consistency with SIO/UART,
       but USART0 has no IrDA hardware — ignored here. */
    (void)config->irhfcg;
    (void)config->irlfcfg;
    (void)config->irctrl;

    u->EN_SET = USART_EN_EN;
    while ((u->SYNCBUSY & _USART_SYNCBUSY_MASK) != 0U) {}

    /* Clear FIFOs and flags. */
    u->CMD_SET = USART_CMD_RXDIS | USART_CMD_TXDIS |
                 USART_CMD_CLEARTX | USART_CMD_CLEARRX;
    u->IF_CLR = u->IF;

    /* Enable RX/TX. */
    u->CMD_SET = USART_CMD_RXEN | USART_CMD_TXEN;

    /* Enable interrupts. */
    u->IEN_SET = USART_IEN_RXDATAV | USART_IEN_RXFULL | USART_IEN_TXBL |
                 USART_IEN_TXC | USART_IEN_RXOF | USART_IEN_PERR |
                 USART_IEN_FERR;
    nvicEnableVector(SERIAL_RX_IRQN, SERIAL_RX_PRIO);
    nvicEnableVector(SERIAL_TX_IRQN, SERIAL_TX_PRIO);
  }
}

void sd_lld_stop(SerialDriver *sdp) {
  USART_TypeDef *u = (USART_TypeDef *)sdp->usart;

  if (sdp->state == SD_READY) {
    nvicDisableVector(SERIAL_RX_IRQN);
    nvicDisableVector(SERIAL_TX_IRQN);
    u->IEN_CLR = _USART_IEN_MASK;
    u->CMD_SET = USART_CMD_RXDIS | USART_CMD_TXDIS;
    while ((u->SYNCBUSY & _USART_SYNCBUSY_MASK) != 0U) {}
    u->EN_CLR = USART_EN_EN;
    while ((u->EN & USART_EN_DISABLING) == USART_EN_DISABLING) {}
    CMU->CLKEN0_CLR = SERIAL_CLKEN;
  }
}

#endif /* HAL_USE_SERIAL == TRUE */

/** @} */
