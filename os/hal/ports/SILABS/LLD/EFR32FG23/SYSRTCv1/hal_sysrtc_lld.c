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
 * @file    SYSRTCv1/hal_sysrtc_lld.c
 * @brief   EFR32FG23 SYSRTC low-level driver source.
 *
 * @addtogroup EFR32_SYSRTC
 * @{
 */

#include "hal.h"
#include "hal_sysrtc_lld.h"

#if EFR32_SYSRTC_USE_SYSRTC || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if EFR32_SYSRTC_USE_SYSRTC && !defined(__DOXYGEN__)
SYSRTCDriver SYSRTCD1;
#endif

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   SYSRTC_SEQ interrupt handler (group 0 events).
 * @note    Not defined when SYSTICKv2 owns the SYSRTC_SEQ vector
 *          (EFR32_ST_OWNS_SYSRTC_SEQ) — in that case SYSTICKv2 forwards
 *          events to sysrtc_lld_serve_interrupt().
 *
 * @isr
 */
#if !defined(EFR32_ST_OWNS_SYSRTC_SEQ) || (EFR32_ST_OWNS_SYSRTC_SEQ == 0) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(EFR32_SYSRTC_SEQ_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  sysrtc_lld_serve_interrupt(&SYSRTCD1);
  OSAL_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   SYSRTC low-level initialization.
 *
 * @notapi
 */
void sysrtc_lld_init(void) {
#if EFR32_SYSRTC_USE_SYSRTC && !defined(__DOXYGEN__)
  SYSRTCD1.state = 0U;
  SYSRTCD1.callback = NULL;
  SYSRTCD1.callback_arg = NULL;
#endif
}

/**
 * @brief   Starts the SYSRTC counter from the configured LF clock.
 *
 * @param[in] drvp      pointer to the @p SYSRTCDriver object
 * @param[in] cb        event callback (compare/capture/overflow), or NULL
 * @param[in] arg       callback argument
 *
 * @notapi
 */
void sysrtc_lld_start(SYSRTCDriver *drvp, efr32_sysrtc_cb_t cb, void *arg) {
  if (drvp->state == 0U) {
    /* Enable bus clock. */
    CMU->CLKEN0_SET = CMU_CLKEN0_SYSRTC0;

    /* Select LF clock source. */
    switch (EFR32_SYSRTC_CLK_SOURCE) {
    case EFR32_SYSRTC_CLKSEL_LFXO:
      CMU->SYSRTC0CLKCTRL = (CMU->SYSRTC0CLKCTRL & ~_CMU_SYSRTC0CLKCTRL_CLKSEL_MASK)
                            | CMU_SYSRTC0CLKCTRL_CLKSEL_LFXO;
      break;
    case EFR32_SYSRTC_CLKSEL_LFRCO:
      CMU->SYSRTC0CLKCTRL = (CMU->SYSRTC0CLKCTRL & ~_CMU_SYSRTC0CLKCTRL_CLKSEL_MASK)
                            | CMU_SYSRTC0CLKCTRL_CLKSEL_LFRCO;
      break;
    case EFR32_SYSRTC_CLKSEL_ULFRCO:
      CMU->SYSRTC0CLKCTRL = (CMU->SYSRTC0CLKCTRL & ~_CMU_SYSRTC0CLKCTRL_CLKSEL_MASK)
                            | CMU_SYSRTC0CLKCTRL_CLKSEL_ULFRCO;
      break;
    default:
      osalDbgAssert(false, "invalid SYSRTC clock source");
      break;
    }

    /* Enable module. */
    SYSRTC0->EN_SET = SYSRTC_EN_EN;

    /* Debug run enable (keep counting when halted). */
    SYSRTC0->CFG = SYSRTC_CFG_DEBUGRUN_ENABLE;

    /* Clear group 0: disable all channels, clear flags. */
    SYSRTC0->GRP0_CTRL = 0U;
    SYSRTC0->GRP0_IF_CLR = _SYSRTC_GRP0_IF_MASK;
    SYSRTC0->GRP0_IEN = 0U;

    /* Start counter. */
    SYSRTC0->CMD_SET = SYSRTC_CMD_START;
    while ((SYSRTC0->SYNCBUSY & _SYSRTC_SYNCBUSY_MASK) != 0U) {}

    /* Enable SEQ interrupt. */
    nvicEnableVector(EFR32_SYSRTC_SEQ_NUMBER, EFR32_SYSRTC_IRQ_PRIORITY);

    drvp->state = 1U;
  }

  drvp->callback = cb;
  drvp->callback_arg = arg;
}

/**
 * @brief   Stops the SYSRTC counter.
 *
 * @param[in] drvp      pointer to the @p SYSRTCDriver object
 *
 * @notapi
 */
void sysrtc_lld_stop(SYSRTCDriver *drvp) {
  if (drvp->state == 1U) {
    SYSRTC0->CMD_SET = SYSRTC_CMD_STOP;
    while ((SYSRTC0->SYNCBUSY & _SYSRTC_SYNCBUSY_MASK) != 0U) {}

    SYSRTC0->GRP0_IEN = 0U;
    SYSRTC0->GRP0_IF_CLR = _SYSRTC_GRP0_IF_MASK;
    nvicDisableVector(EFR32_SYSRTC_SEQ_NUMBER);

    SYSRTC0->EN_CLR = SYSRTC_EN_EN;
    CMU->CLKEN0_CLR = CMU_CLKEN0_SYSRTC0;

    drvp->state = 0U;
  }
}

/**
 * @brief   Returns the current SYSRTC counter value.
 *
 * @return              32-bit counter value (ticks at LF clock rate).
 *
 * @notapi
 */
uint32_t sysrtc_lld_get_counter(void) {
  return SYSRTC0->CNT;
}

/**
 * @brief   Sets the SYSRTC counter value.
 *
 * @param[in] value     new counter value
 *
 * @notapi
 */
void sysrtc_lld_set_counter(uint32_t value) {
  SYSRTC0->CNT = value;
  while ((SYSRTC0->SYNCBUSY & _SYSRTC_SYNCBUSY_CNT_MASK) != 0U) {}
}

/**
 * @brief   Sets a compare (alarm) channel value.
 *
 * @param[in] drvp      pointer to the @p SYSRTCDriver object
 * @param[in] channel   compare channel (0 or 1)
 * @param[in] value     compare value in ticks
 *
 * @notapi
 */
void sysrtc_lld_set_compare(SYSRTCDriver *drvp, uint32_t channel, uint32_t value) {
  (void)drvp;
  osalDbgCheck(channel < EFR32_SYSRTC_NUM_COMPARE);

  if (channel == 0U) {
    SYSRTC0->GRP0_CMP0VALUE = value;
  } else {
    SYSRTC0->GRP0_CMP1VALUE = value;
  }
}

/**
 * @brief   Returns a compare channel value.
 *
 * @param[in] drvp      pointer to the @p SYSRTCDriver object
 * @param[in] channel   compare channel (0 or 1)
 * @return              The compare value.
 *
 * @notapi
 */
uint32_t sysrtc_lld_get_compare(SYSRTCDriver *drvp, uint32_t channel) {
  (void)drvp;
  osalDbgCheck(channel < EFR32_SYSRTC_NUM_COMPARE);

  return (channel == 0U) ? SYSRTC0->GRP0_CMP0VALUE : SYSRTC0->GRP0_CMP1VALUE;
}

/**
 * @brief   Enables a compare (alarm) channel and its interrupt.
 *
 * @param[in] drvp      pointer to the @p SYSRTCDriver object
 * @param[in] channel   compare channel (0 or 1)
 *
 * @notapi
 */
void sysrtc_lld_enable_compare(SYSRTCDriver *drvp, uint32_t channel) {
  (void)drvp;
  osalDbgCheck(channel < EFR32_SYSRTC_NUM_COMPARE);

  uint32_t ctrl_bit = (channel == 0U) ? _SYSRTC_GRP0_CTRL_CMP0EN_MASK
                                       : _SYSRTC_GRP0_CTRL_CMP1EN_MASK;
  uint32_t if_bit   = (channel == 0U) ? SYSRTC_GRP0_IF_CMP0
                                       : SYSRTC_GRP0_IF_CMP1;

  SYSRTC0->GRP0_IF_CLR = if_bit;
  SYSRTC0->GRP0_CTRL_SET = ctrl_bit;
  SYSRTC0->GRP0_IEN_SET = if_bit;
}

/**
 * @brief   Disables a compare (alarm) channel.
 *
 * @param[in] drvp      pointer to the @p SYSRTCDriver object
 * @param[in] channel   compare channel (0 or 1)
 *
 * @notapi
 */
void sysrtc_lld_disable_compare(SYSRTCDriver *drvp, uint32_t channel) {
  (void)drvp;
  osalDbgCheck(channel < EFR32_SYSRTC_NUM_COMPARE);

  uint32_t ctrl_bit = (channel == 0U) ? _SYSRTC_GRP0_CTRL_CMP0EN_MASK
                                       : _SYSRTC_GRP0_CTRL_CMP1EN_MASK;
  uint32_t if_bit   = (channel == 0U) ? SYSRTC_GRP0_IF_CMP0
                                       : SYSRTC_GRP0_IF_CMP1;

  SYSRTC0->GRP0_CTRL_CLR = ctrl_bit;
  SYSRTC0->GRP0_IEN_CLR = if_bit;
  SYSRTC0->GRP0_IF_CLR = if_bit;
}

/**
 * @brief   Enables a capture channel (PRS input) and its interrupt.
 *
 * @param[in] drvp      pointer to the @p SYSRTCDriver object
 * @param[in] channel   capture channel (0)
 *
 * @notapi
 */
void sysrtc_lld_enable_capture(SYSRTCDriver *drvp, uint32_t channel) {
  (void)drvp;
  osalDbgCheck(channel < EFR32_SYSRTC_NUM_CAPTURE);

  SYSRTC0->GRP0_IF_CLR = SYSRTC_GRP0_IF_CAP0;
  SYSRTC0->GRP0_CTRL_SET = _SYSRTC_GRP0_CTRL_CAP0EN_MASK;
  SYSRTC0->GRP0_IEN_SET = SYSRTC_GRP0_IF_CAP0;
}

/**
 * @brief   Disables a capture channel.
 *
 * @param[in] drvp      pointer to the @p SYSRTCDriver object
 * @param[in] channel   capture channel (0)
 *
 * @notapi
 */
void sysrtc_lld_disable_capture(SYSRTCDriver *drvp, uint32_t channel) {
  (void)drvp;
  osalDbgCheck(channel < EFR32_SYSRTC_NUM_CAPTURE);

  SYSRTC0->GRP0_CTRL_CLR = _SYSRTC_GRP0_CTRL_CAP0EN_MASK;
  SYSRTC0->GRP0_IEN_CLR = SYSRTC_GRP0_IF_CAP0;
  SYSRTC0->GRP0_IF_CLR = SYSRTC_GRP0_IF_CAP0;
}

/**
 * @brief   Returns the last captured counter value.
 *
 * @param[in] drvp      pointer to the @p SYSRTCDriver object
 * @param[in] channel   capture channel (0)
 * @return              The captured CNT value.
 *
 * @notapi
 */
uint32_t sysrtc_lld_get_capture(SYSRTCDriver *drvp, uint32_t channel) {
  (void)drvp;
  osalDbgCheck(channel < EFR32_SYSRTC_NUM_CAPTURE);

  return SYSRTC0->GRP0_CAP0VALUE;
}

/**
 * @brief   Enables the overflow interrupt.
 *
 * @param[in] drvp      pointer to the @p SYSRTCDriver object
 *
 * @notapi
 */
void sysrtc_lld_enable_ovf_irq(SYSRTCDriver *drvp) {
  (void)drvp;
  SYSRTC0->GRP0_IF_CLR = SYSRTC_GRP0_IF_OVF;
  SYSRTC0->GRP0_IEN_SET = SYSRTC_GRP0_IF_OVF;
}

/**
 * @brief   Disables the overflow interrupt.
 *
 * @param[in] drvp      pointer to the @p SYSRTCDriver object
 *
 * @notapi
 */
void sysrtc_lld_disable_ovf_irq(SYSRTCDriver *drvp) {
  (void)drvp;
  SYSRTC0->GRP0_IEN_CLR = SYSRTC_GRP0_IF_OVF;
  SYSRTC0->GRP0_IF_CLR = SYSRTC_GRP0_IF_OVF;
}

/**
 * @brief   SYSRTC SEQ interrupt service routine.
 * @details Dispatches compare/capture/overflow events to the registered
 *          callback.
 *
 * @param[in] drvp      pointer to the @p SYSRTCDriver object
 *
 * @notapi
 */
void sysrtc_lld_serve_interrupt(SYSRTCDriver *drvp) {
  uint32_t isr = SYSRTC0->GRP0_IF;

#if !defined(EFR32_ST_OWNS_SYSRTC_SEQ) || (EFR32_ST_OWNS_SYSRTC_SEQ == 0) || defined(__DOXYGEN__)
  /* OVF and CMP0 are owned by SYSTICKv2 when it uses SYSRTC_SEQ. */
  if ((isr & SYSRTC_GRP0_IF_OVF) != 0U) {
    SYSRTC0->GRP0_IF_CLR = SYSRTC_GRP0_IF_OVF;
    if (drvp->callback != NULL) {
      drvp->callback(EFR32_SYSRTC_EVT_OVF, 0U, drvp->callback_arg);
    }
  }

  if ((isr & SYSRTC_GRP0_IF_CMP0) != 0U) {
    SYSRTC0->GRP0_IF_CLR = SYSRTC_GRP0_IF_CMP0;
    if (drvp->callback != NULL) {
      drvp->callback(EFR32_SYSRTC_EVT_CMP0, SYSRTC0->GRP0_CMP0VALUE, drvp->callback_arg);
    }
  }
#endif

  if ((isr & SYSRTC_GRP0_IF_CMP1) != 0U) {
    SYSRTC0->GRP0_IF_CLR = SYSRTC_GRP0_IF_CMP1;
    if (drvp->callback != NULL) {
      drvp->callback(EFR32_SYSRTC_EVT_CMP1, SYSRTC0->GRP0_CMP1VALUE, drvp->callback_arg);
    }
  }

  if ((isr & SYSRTC_GRP0_IF_CAP0) != 0U) {
    SYSRTC0->GRP0_IF_CLR = SYSRTC_GRP0_IF_CAP0;
    if (drvp->callback != NULL) {
      drvp->callback(EFR32_SYSRTC_EVT_CAP0, SYSRTC0->GRP0_CAP0VALUE, drvp->callback_arg);
    }
  }
}

#endif /* EFR32_SYSRTC_USE_SYSRTC */

/** @} */
