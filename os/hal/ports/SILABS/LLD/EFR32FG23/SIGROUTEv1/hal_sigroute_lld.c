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
 * @file    SIGROUTEv1/hal_sigroute_lld.c
 * @brief   EFR32FG23 PRS hardware LLD source for hal_sigroute.
 *
 * @details Implements the generic SIGROUTE driver on the Silicon Labs
 *          PRS (Peripheral Reflex System) peripheral — SIGROUTEv1 is
 *          the driver version, the underlying hardware is PRS.
 *
 * @addtogroup SIGROUTE
 * @{
 */

#include "hal.h"
#include "hal_sigroute_lld.h"

#if (HAL_USE_SIGROUTE == TRUE) && (EFR32_SIGROUTE_USE_PRS == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* PRS has no enable bit and no SYNCBUSY; only the CMU bus clock gate. */
#define sigroute_clock_enable()      (CMU->CLKEN0_SET = CMU_CLKEN0_PRS)
#define sigroute_clock_disable()     (CMU->CLKEN0_CLR = CMU_CLKEN0_PRS)

/* Map a portable sigroute_fn_t onto the hardware FNSEL encoding (bits 16..19). */
static uint32_t sigroute_fn_to_fnsel(sigroute_fn_t fn) {
  switch (fn) {
  case SIGROUTE_FN_PASS_A: return PRS_ASYNC_CH_CTRL_FNSEL_A;
  case SIGROUTE_FN_NOT_A:  return PRS_ASYNC_CH_CTRL_FNSEL_NOT_A;
  case SIGROUTE_FN_AND:    return PRS_ASYNC_CH_CTRL_FNSEL_A_AND_B;
  case SIGROUTE_FN_OR:     return PRS_ASYNC_CH_CTRL_FNSEL_A_OR_B;
  case SIGROUTE_FN_XOR:    return PRS_ASYNC_CH_CTRL_FNSEL_A_XOR_B;
  case SIGROUTE_FN_NAND:   return PRS_ASYNC_CH_CTRL_FNSEL_A_NAND_B;
  case SIGROUTE_FN_NOR:    return PRS_ASYNC_CH_CTRL_FNSEL_A_NOR_B;
  case SIGROUTE_FN_XNOR:   return PRS_ASYNC_CH_CTRL_FNSEL_A_XNOR_B;
  case SIGROUTE_FN_ZERO:   return PRS_ASYNC_CH_CTRL_FNSEL_LOGICAL_ZERO;
  case SIGROUTE_FN_ONE:    return PRS_ASYNC_CH_CTRL_FNSEL_LOGICAL_ONE;
  }
  return PRS_ASYNC_CH_CTRL_FNSEL_A;
}

/* Map a SIGROUTE_CON_* encoding onto its CONSUMER_* register. */
static volatile uint32_t *sigroute_consumer_reg(sigroute_consumer_t consumer) {
  switch (consumer) {
  case SIGROUTE_CON_LETIMER0_START:         return &PRS->CONSUMER_LETIMER0_START;
  case SIGROUTE_CON_LETIMER0_STOP:          return &PRS->CONSUMER_LETIMER0_STOP;
  case SIGROUTE_CON_LETIMER0_CLEAR:         return &PRS->CONSUMER_LETIMER0_CLEAR;
  case SIGROUTE_CON_LESENSE_START:          return &PRS->CONSUMER_LESENSE_START;
  case SIGROUTE_CON_IADC0_SINGLETRIGGER:    return &PRS->CONSUMER_IADC0_SINGLETRIGGER;
  case SIGROUTE_CON_IADC0_SCANTRIGGER:      return &PRS->CONSUMER_IADC0_SCANTRIGGER;
  case SIGROUTE_CON_PCNT0_S0IN:             return &PRS->CONSUMER_PCNT0_S0IN;
  case SIGROUTE_CON_LDMAXBAR_DMAREQ0:       return &PRS->CONSUMER_LDMAXBAR_DMAREQ0;
  case SIGROUTE_CON_LDMAXBAR_DMAREQ1:       return &PRS->CONSUMER_LDMAXBAR_DMAREQ1;
  default:                             return (volatile uint32_t *)0;
  }
}

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

SIGROUTEDriver SIGROUTED1;

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void sigroute_lld_init(void) {
  /* Nothing to configure at init. */
}

void sigroute_lld_start(SIGROUTEDriver *pp) {
  (void)pp;
  sigroute_clock_enable();
}

void sigroute_lld_stop(SIGROUTEDriver *pp) {
  (void)pp;
  /* Disconnect all channels before gating the clock. */
  for (uint8_t ch = 0U; ch < SIGROUTE_NUM_ASYNC_CH; ch++) {
    PRS->ASYNC_CH[ch].CTRL = 0U;
  }
  for (uint8_t ch = 0U; ch < SIGROUTE_NUM_SYNC_CH; ch++) {
    PRS->SYNC_CH[ch].CTRL = 0U;
  }
  sigroute_clock_disable();
}

void sigroute_lld_connect_async(SIGROUTEDriver *pp, uint8_t ch, sigroute_source_t gen) {
  (void)pp;
  osalDbgAssert(ch < SIGROUTE_NUM_ASYNC_CH, "async ch out of range");
  /* Pass-through: FNSEL_A with the combined SOURCESEL|SIGSEL signal. */
  PRS->ASYNC_CH[ch].CTRL = PRS_ASYNC_CH_CTRL_FNSEL_A | gen;
}

void sigroute_lld_connect_sync(SIGROUTEDriver *pp, uint8_t ch, sigroute_source_t gen) {
  (void)pp;
  osalDbgAssert(ch < SIGROUTE_NUM_SYNC_CH, "sync ch out of range");
  PRS->SYNC_CH[ch].CTRL = gen;   /* combined PRS_SYNC_* macro */
}

void sigroute_lld_disconnect_async(SIGROUTEDriver *pp, uint8_t ch) {
  (void)pp;
  osalDbgAssert(ch < SIGROUTE_NUM_ASYNC_CH, "async ch out of range");
  PRS->ASYNC_CH[ch].CTRL = 0U;   /* SIGSEL_NONE */
}

void sigroute_lld_disconnect_sync(SIGROUTEDriver *pp, uint8_t ch) {
  (void)pp;
  osalDbgAssert(ch < SIGROUTE_NUM_SYNC_CH, "sync ch out of range");
  PRS->SYNC_CH[ch].CTRL = 0U;    /* SIGSEL_NONE */
}

void sigroute_lld_set_logic(SIGROUTEDriver *pp, uint8_t ch, sigroute_fn_t fn, uint8_t aux) {
  (void)pp;
  osalDbgAssert(ch < SIGROUTE_NUM_ASYNC_CH, "async ch out of range");
  osalDbgAssert(aux < SIGROUTE_NUM_ASYNC_CH, "aux ch out of range");
  uint32_t ctrl = PRS->ASYNC_CH[ch].CTRL;
  ctrl &= ~(_PRS_ASYNC_CH_CTRL_FNSEL_MASK | _PRS_ASYNC_CH_CTRL_AUXSEL_MASK);
  ctrl |= sigroute_fn_to_fnsel(fn);
  ctrl |= ((uint32_t)aux << _PRS_ASYNC_CH_CTRL_AUXSEL_SHIFT)
          & _PRS_ASYNC_CH_CTRL_AUXSEL_MASK;
  PRS->ASYNC_CH[ch].CTRL = ctrl;
}

void sigroute_lld_connect_consumer(SIGROUTEDriver *pp, sigroute_consumer_t consumer,
                              sigroute_path_t path, uint8_t ch) {
  (void)pp;
  (void)path;   /* PRSSEL field selects async or sync channel number. */
  volatile uint32_t *reg = sigroute_consumer_reg(consumer);
  osalDbgAssert(reg != (volatile uint32_t *)0, "unknown consumer");
  osalDbgAssert(ch < ((path == SIGROUTE_PATH_SYNC) ? SIGROUTE_NUM_SYNC_CH
                                              : SIGROUTE_NUM_ASYNC_CH),
                "ch out of range");
  *reg = (uint32_t)ch & 0x0000000FUL;   /* PRSSEL field, bits 0..3 */
}

void sigroute_lld_sw_pulse(SIGROUTEDriver *pp, uint16_t ch_mask) {
  (void)pp;
  PRS->ASYNC_SWPULSE = (uint32_t)ch_mask & _PRS_ASYNC_SWPULSE_MASK;
}

void sigroute_lld_sw_level_set(SIGROUTEDriver *pp, uint16_t ch_mask) {
  (void)pp;
  PRS->ASYNC_SWLEVEL = PRS->ASYNC_SWLEVEL
                       | ((uint32_t)ch_mask & _PRS_ASYNC_SWLEVEL_MASK);
}

void sigroute_lld_sw_level_clear(SIGROUTEDriver *pp, uint16_t ch_mask) {
  (void)pp;
  PRS->ASYNC_SWLEVEL = PRS->ASYNC_SWLEVEL
                       & ~((uint32_t)ch_mask & _PRS_ASYNC_SWLEVEL_MASK);
}

uint32_t sigroute_lld_peek_async(SIGROUTEDriver *pp) {
  (void)pp;
  return PRS->ASYNC_PEEK & _PRS_ASYNC_PEEK_MASK;
}

uint32_t sigroute_lld_peek_sync(SIGROUTEDriver *pp) {
  (void)pp;
  return PRS->SYNC_PEEK & _PRS_SYNC_PEEK_MASK;
}

void sigroute_lld_route_async_pin(SIGROUTEDriver *pp, uint8_t ch,
                             ioportid_t port, ioportmask_t pad) {
  (void)pp;
  osalDbgAssert(ch < SIGROUTE_NUM_ASYNC_CH, "async ch out of range");
  volatile uint32_t *route = &GPIO->PRSROUTE[0].ASYNCH0ROUTE;
  route[ch] = (GPIO_PORT_INDEX(port) << _GPIO_PRS_ASYNCH0ROUTE_PORT_SHIFT)
            | ((uint32_t)pad << _GPIO_PRS_ASYNCH0ROUTE_PIN_SHIFT);
  GPIO->PRSROUTE[0].ROUTEEN |= (GPIO_PRS_ROUTEEN_ASYNCH0PEN << ch);
}

void sigroute_lld_route_sync_pin(SIGROUTEDriver *pp, uint8_t ch,
                            ioportid_t port, ioportmask_t pad) {
  (void)pp;
  osalDbgAssert(ch < SIGROUTE_NUM_SYNC_CH, "sync ch out of range");
  volatile uint32_t *route = &GPIO->PRSROUTE[0].SYNCH0ROUTE;
  route[ch] = (GPIO_PORT_INDEX(port) << _GPIO_PRS_SYNCH0ROUTE_PORT_SHIFT)
            | ((uint32_t)pad << _GPIO_PRS_SYNCH0ROUTE_PIN_SHIFT);
  GPIO->PRSROUTE[0].ROUTEEN |= (GPIO_PRS_ROUTEEN_SYNCH0PEN << ch);
}

void sigroute_lld_unroute_async_pin(SIGROUTEDriver *pp, uint8_t ch) {
  (void)pp;
  osalDbgAssert(ch < SIGROUTE_NUM_ASYNC_CH, "async ch out of range");
  volatile uint32_t *route = &GPIO->PRSROUTE[0].ASYNCH0ROUTE;
  route[ch] = 0U;
  GPIO->PRSROUTE[0].ROUTEEN &= ~(GPIO_PRS_ROUTEEN_ASYNCH0PEN << ch);
}

void sigroute_lld_unroute_sync_pin(SIGROUTEDriver *pp, uint8_t ch) {
  (void)pp;
  osalDbgAssert(ch < SIGROUTE_NUM_SYNC_CH, "sync ch out of range");
  volatile uint32_t *route = &GPIO->PRSROUTE[0].SYNCH0ROUTE;
  route[ch] = 0U;
  GPIO->PRSROUTE[0].ROUTEEN &= ~(GPIO_PRS_ROUTEEN_SYNCH0PEN << ch);
}

#endif /* HAL_USE_SIGROUTE && EFR32_SIGROUTE_USE_PRS */

/** @} */
