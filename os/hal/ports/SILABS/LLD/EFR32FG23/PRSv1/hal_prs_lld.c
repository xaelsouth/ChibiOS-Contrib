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

#if (EFR32_PRS_USE_PRS == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void prs_lld_init(void) {
  /* Nothing to configure at init. */
}

void prs_lld_start(void) {
  CMU->CLKEN0_SET = CMU_CLKEN0_PRS;
}

void prs_lld_stop(void) {
  /* Disconnect all channels before gating the clock. */
  for (uint8_t ch = 0U; ch < EFR32_PRS_NUM_ASYNC_CH; ch++) {
    PRS->ASYNC_CH[ch].CTRL = 0U;
  }
  for (uint8_t ch = 0U; ch < EFR32_PRS_NUM_SYNC_CH; ch++) {
    PRS->SYNC_CH[ch].CTRL = 0U;
  }
  CMU->CLKEN0_CLR = CMU_CLKEN0_PRS;
}

void prs_lld_connect_async(uint8_t ch, uint32_t signal) {
  osalDbgAssert(ch < EFR32_PRS_NUM_ASYNC_CH, "async ch out of range");
  /* Pass-through: FNSEL_A with the combined SOURCESEL|SIGSEL signal. */
  PRS->ASYNC_CH[ch].CTRL = PRS_ASYNC_CH_CTRL_FNSEL_A | signal;
}

void prs_lld_connect_async_fn(uint8_t ch, uint32_t signal, uint32_t fn) {
  osalDbgAssert(ch < EFR32_PRS_NUM_ASYNC_CH, "async ch out of range");
  PRS->ASYNC_CH[ch].CTRL = (fn & _PRS_ASYNC_CH_CTRL_FNSEL_MASK) | signal;
}

void prs_lld_disconnect_async(uint8_t ch) {
  osalDbgAssert(ch < EFR32_PRS_NUM_ASYNC_CH, "async ch out of range");
  PRS->ASYNC_CH[ch].CTRL = 0U;   /* SIGSEL_NONE */
}

void prs_lld_connect_sync(uint8_t ch, uint32_t signal) {
  osalDbgAssert(ch < EFR32_PRS_NUM_SYNC_CH, "sync ch out of range");
  PRS->SYNC_CH[ch].CTRL = signal;   /* combined PRS_SYNC_* macro */
}

void prs_lld_disconnect_sync(uint8_t ch) {
  osalDbgAssert(ch < EFR32_PRS_NUM_SYNC_CH, "sync ch out of range");
  PRS->SYNC_CH[ch].CTRL = 0U;       /* SIGSEL_NONE */
}

void prs_lld_sw_pulse(uint16_t ch_mask) {
  PRS->ASYNC_SWPULSE = (uint32_t)ch_mask & _PRS_ASYNC_SWPULSE_MASK;
}

void prs_lld_sw_level_set(uint16_t ch_mask) {
  PRS->ASYNC_SWLEVEL = PRS->ASYNC_SWLEVEL
                       | ((uint32_t)ch_mask & _PRS_ASYNC_SWLEVEL_MASK);
}

void prs_lld_sw_level_clear(uint16_t ch_mask) {
  PRS->ASYNC_SWLEVEL = PRS->ASYNC_SWLEVEL
                       & ~((uint32_t)ch_mask & _PRS_ASYNC_SWLEVEL_MASK);
}

uint32_t prs_lld_peek_async(void) {
  return PRS->ASYNC_PEEK & _PRS_ASYNC_PEEK_MASK;
}

uint32_t prs_lld_peek_sync(void) {
  return PRS->SYNC_PEEK & _PRS_SYNC_PEEK_MASK;
}

void prs_lld_route_async_pin(uint8_t ch, ioportid_t port, ioportmask_t pad) {
  osalDbgAssert(ch < EFR32_PRS_NUM_ASYNC_CH, "async ch out of range");
  volatile uint32_t *route = &GPIO->PRSROUTE[0].ASYNCH0ROUTE;
  route[ch] = (GPIO_PORT_INDEX(port) << _GPIO_PRS_ASYNCH0ROUTE_PORT_SHIFT)
            | ((uint32_t)pad << _GPIO_PRS_ASYNCH0ROUTE_PIN_SHIFT);
  GPIO->PRSROUTE[0].ROUTEEN |= (GPIO_PRS_ROUTEEN_ASYNCH0PEN << ch);
}

void prs_lld_route_sync_pin(uint8_t ch, ioportid_t port, ioportmask_t pad) {
  osalDbgAssert(ch < EFR32_PRS_NUM_SYNC_CH, "sync ch out of range");
  volatile uint32_t *route = &GPIO->PRSROUTE[0].SYNCH0ROUTE;
  route[ch] = (GPIO_PORT_INDEX(port) << _GPIO_PRS_SYNCH0ROUTE_PORT_SHIFT)
            | ((uint32_t)pad << _GPIO_PRS_SYNCH0ROUTE_PIN_SHIFT);
  GPIO->PRSROUTE[0].ROUTEEN |= (GPIO_PRS_ROUTEEN_SYNCH0PEN << ch);
}

void prs_lld_unroute_async_pin(uint8_t ch) {
  osalDbgAssert(ch < EFR32_PRS_NUM_ASYNC_CH, "async ch out of range");
  volatile uint32_t *route = &GPIO->PRSROUTE[0].ASYNCH0ROUTE;
  route[ch] = 0U;
  GPIO->PRSROUTE[0].ROUTEEN &= ~(GPIO_PRS_ROUTEEN_ASYNCH0PEN << ch);
}

void prs_lld_unroute_sync_pin(uint8_t ch) {
  osalDbgAssert(ch < EFR32_PRS_NUM_SYNC_CH, "sync ch out of range");
  volatile uint32_t *route = &GPIO->PRSROUTE[0].SYNCH0ROUTE;
  route[ch] = 0U;
  GPIO->PRSROUTE[0].ROUTEEN &= ~(GPIO_PRS_ROUTEEN_SYNCH0PEN << ch);
}

#endif /* EFR32_PRS_USE_PRS == TRUE */

/** @} */
