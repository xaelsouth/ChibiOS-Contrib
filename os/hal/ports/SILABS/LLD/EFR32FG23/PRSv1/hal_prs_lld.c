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
 * @file    PRSv1/hal_prs_lld.c
 * @brief   EFR32FG23 PRS (Peripheral Reflex System) LLD source.
 *
 * @addtogroup PRS
 * @{
 */

#include "hal.h"
#include "hal_prs_lld.h"

#if (HAL_USE_PRS == TRUE) && (EFR32_PRS_USE_PRS == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* PRS has no enable bit and no SYNCBUSY; only the CMU bus clock gate. */
#define prs_clock_enable()      (CMU->CLKEN0_SET = CMU_CLKEN0_PRS)
#define prs_clock_disable()     (CMU->CLKEN0_CLR = CMU_CLKEN0_PRS)

/* Map a portable prs_fn_t onto the hardware FNSEL encoding (bits 16..19). */
static uint32_t prs_fn_to_fnsel(prs_fn_t fn) {
  switch (fn) {
  case PRS_FN_PASS_A: return PRS_ASYNC_CH_CTRL_FNSEL_A;
  case PRS_FN_NOT_A:  return PRS_ASYNC_CH_CTRL_FNSEL_NOT_A;
  case PRS_FN_AND:    return PRS_ASYNC_CH_CTRL_FNSEL_A_AND_B;
  case PRS_FN_OR:     return PRS_ASYNC_CH_CTRL_FNSEL_A_OR_B;
  case PRS_FN_XOR:    return PRS_ASYNC_CH_CTRL_FNSEL_A_XOR_B;
  case PRS_FN_NAND:   return PRS_ASYNC_CH_CTRL_FNSEL_A_NAND_B;
  case PRS_FN_NOR:    return PRS_ASYNC_CH_CTRL_FNSEL_A_NOR_B;
  case PRS_FN_XNOR:   return PRS_ASYNC_CH_CTRL_FNSEL_A_XNOR_B;
  case PRS_FN_ZERO:   return PRS_ASYNC_CH_CTRL_FNSEL_LOGICAL_ZERO;
  case PRS_FN_ONE:    return PRS_ASYNC_CH_CTRL_FNSEL_LOGICAL_ONE;
  }
  return PRS_ASYNC_CH_CTRL_FNSEL_A;
}

/* Map a PRS_CON_* encoding onto its CONSUMER_* register. */
static volatile uint32_t *prs_consumer_reg(prs_consumer_t consumer) {
  switch (consumer) {
  case PRS_CON_LETIMER0_START:         return &PRS->CONSUMER_LETIMER0_START;
  case PRS_CON_LETIMER0_STOP:          return &PRS->CONSUMER_LETIMER0_STOP;
  case PRS_CON_LETIMER0_CLEAR:         return &PRS->CONSUMER_LETIMER0_CLEAR;
  case PRS_CON_LESENSE_START:          return &PRS->CONSUMER_LESENSE_START;
  case PRS_CON_IADC0_SINGLETRIGGER:    return &PRS->CONSUMER_IADC0_SINGLETRIGGER;
  case PRS_CON_IADC0_SCANTRIGGER:      return &PRS->CONSUMER_IADC0_SCANTRIGGER;
  default:                             return (volatile uint32_t *)0;
  }
}

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

PRSDriver PRSD1;

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void prs_lld_init(void) {
  /* Nothing to configure at init. */
}

void prs_lld_start(PRSDriver *pp) {
  (void)pp;
  prs_clock_enable();
}

void prs_lld_stop(PRSDriver *pp) {
  (void)pp;
  /* Disconnect all channels before gating the clock. */
  for (uint8_t ch = 0U; ch < PRS_NUM_ASYNC_CH; ch++) {
    PRS->ASYNC_CH[ch].CTRL = 0U;
  }
  for (uint8_t ch = 0U; ch < PRS_NUM_SYNC_CH; ch++) {
    PRS->SYNC_CH[ch].CTRL = 0U;
  }
  prs_clock_disable();
}

void prs_lld_connect_async(PRSDriver *pp, uint8_t ch, prs_source_t gen) {
  (void)pp;
  osalDbgAssert(ch < PRS_NUM_ASYNC_CH, "async ch out of range");
  /* Pass-through: FNSEL_A with the combined SOURCESEL|SIGSEL signal. */
  PRS->ASYNC_CH[ch].CTRL = PRS_ASYNC_CH_CTRL_FNSEL_A | gen;
}

void prs_lld_connect_sync(PRSDriver *pp, uint8_t ch, prs_source_t gen) {
  (void)pp;
  osalDbgAssert(ch < PRS_NUM_SYNC_CH, "sync ch out of range");
  PRS->SYNC_CH[ch].CTRL = gen;   /* combined PRS_SYNC_* macro */
}

void prs_lld_disconnect_async(PRSDriver *pp, uint8_t ch) {
  (void)pp;
  osalDbgAssert(ch < PRS_NUM_ASYNC_CH, "async ch out of range");
  PRS->ASYNC_CH[ch].CTRL = 0U;   /* SIGSEL_NONE */
}

void prs_lld_disconnect_sync(PRSDriver *pp, uint8_t ch) {
  (void)pp;
  osalDbgAssert(ch < PRS_NUM_SYNC_CH, "sync ch out of range");
  PRS->SYNC_CH[ch].CTRL = 0U;    /* SIGSEL_NONE */
}

void prs_lld_set_logic(PRSDriver *pp, uint8_t ch, prs_fn_t fn, uint8_t aux) {
  (void)pp;
  osalDbgAssert(ch < PRS_NUM_ASYNC_CH, "async ch out of range");
  osalDbgAssert(aux < PRS_NUM_ASYNC_CH, "aux ch out of range");
  uint32_t ctrl = PRS->ASYNC_CH[ch].CTRL;
  ctrl &= ~(_PRS_ASYNC_CH_CTRL_FNSEL_MASK | _PRS_ASYNC_CH_CTRL_AUXSEL_MASK);
  ctrl |= prs_fn_to_fnsel(fn);
  ctrl |= ((uint32_t)aux << _PRS_ASYNC_CH_CTRL_AUXSEL_SHIFT)
          & _PRS_ASYNC_CH_CTRL_AUXSEL_MASK;
  PRS->ASYNC_CH[ch].CTRL = ctrl;
}

void prs_lld_connect_consumer(PRSDriver *pp, prs_consumer_t consumer,
                              prs_path_t path, uint8_t ch) {
  (void)pp;
  (void)path;   /* PRSSEL field selects async or sync channel number. */
  volatile uint32_t *reg = prs_consumer_reg(consumer);
  osalDbgAssert(reg != (volatile uint32_t *)0, "unknown consumer");
  osalDbgAssert(ch < ((path == PRS_PATH_SYNC) ? PRS_NUM_SYNC_CH
                                              : PRS_NUM_ASYNC_CH),
                "ch out of range");
  *reg = (uint32_t)ch & 0x0000000FUL;   /* PRSSEL field, bits 0..3 */
}

void prs_lld_sw_pulse(PRSDriver *pp, uint16_t ch_mask) {
  (void)pp;
  PRS->ASYNC_SWPULSE = (uint32_t)ch_mask & _PRS_ASYNC_SWPULSE_MASK;
}

void prs_lld_sw_level_set(PRSDriver *pp, uint16_t ch_mask) {
  (void)pp;
  PRS->ASYNC_SWLEVEL = PRS->ASYNC_SWLEVEL
                       | ((uint32_t)ch_mask & _PRS_ASYNC_SWLEVEL_MASK);
}

void prs_lld_sw_level_clear(PRSDriver *pp, uint16_t ch_mask) {
  (void)pp;
  PRS->ASYNC_SWLEVEL = PRS->ASYNC_SWLEVEL
                       & ~((uint32_t)ch_mask & _PRS_ASYNC_SWLEVEL_MASK);
}

uint32_t prs_lld_peek_async(PRSDriver *pp) {
  (void)pp;
  return PRS->ASYNC_PEEK & _PRS_ASYNC_PEEK_MASK;
}

uint32_t prs_lld_peek_sync(PRSDriver *pp) {
  (void)pp;
  return PRS->SYNC_PEEK & _PRS_SYNC_PEEK_MASK;
}

void prs_lld_route_async_pin(PRSDriver *pp, uint8_t ch,
                             ioportid_t port, ioportmask_t pad) {
  (void)pp;
  osalDbgAssert(ch < PRS_NUM_ASYNC_CH, "async ch out of range");
  volatile uint32_t *route = &GPIO->PRSROUTE[0].ASYNCH0ROUTE;
  route[ch] = (GPIO_PORT_INDEX(port) << _GPIO_PRS_ASYNCH0ROUTE_PORT_SHIFT)
            | ((uint32_t)pad << _GPIO_PRS_ASYNCH0ROUTE_PIN_SHIFT);
  GPIO->PRSROUTE[0].ROUTEEN |= (GPIO_PRS_ROUTEEN_ASYNCH0PEN << ch);
}

void prs_lld_route_sync_pin(PRSDriver *pp, uint8_t ch,
                            ioportid_t port, ioportmask_t pad) {
  (void)pp;
  osalDbgAssert(ch < PRS_NUM_SYNC_CH, "sync ch out of range");
  volatile uint32_t *route = &GPIO->PRSROUTE[0].SYNCH0ROUTE;
  route[ch] = (GPIO_PORT_INDEX(port) << _GPIO_PRS_SYNCH0ROUTE_PORT_SHIFT)
            | ((uint32_t)pad << _GPIO_PRS_SYNCH0ROUTE_PIN_SHIFT);
  GPIO->PRSROUTE[0].ROUTEEN |= (GPIO_PRS_ROUTEEN_SYNCH0PEN << ch);
}

void prs_lld_unroute_async_pin(PRSDriver *pp, uint8_t ch) {
  (void)pp;
  osalDbgAssert(ch < PRS_NUM_ASYNC_CH, "async ch out of range");
  volatile uint32_t *route = &GPIO->PRSROUTE[0].ASYNCH0ROUTE;
  route[ch] = 0U;
  GPIO->PRSROUTE[0].ROUTEEN &= ~(GPIO_PRS_ROUTEEN_ASYNCH0PEN << ch);
}

void prs_lld_unroute_sync_pin(PRSDriver *pp, uint8_t ch) {
  (void)pp;
  osalDbgAssert(ch < PRS_NUM_SYNC_CH, "sync ch out of range");
  volatile uint32_t *route = &GPIO->PRSROUTE[0].SYNCH0ROUTE;
  route[ch] = 0U;
  GPIO->PRSROUTE[0].ROUTEEN &= ~(GPIO_PRS_ROUTEEN_SYNCH0PEN << ch);
}

#endif /* HAL_USE_PRS && EFR32_PRS_USE_PRS */

/** @} */
