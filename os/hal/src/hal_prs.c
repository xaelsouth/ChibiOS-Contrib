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
 * @file    hal_prs.c
 * @brief   Generic peripheral reflex/routing driver (upper layer).
 *
 * @addtogroup PRS
 * @{
 */

#include "hal_prs.h"

#if (HAL_USE_PRS == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void prsInit(void) {
  prs_lld_init();
}

void prsObjectInit(PRSDriver *pp) {
  pp->state  = PRS_STOP;
  pp->config = NULL;
}

void prsStart(PRSDriver *pp, const PRSConfig *config) {
  osalDbgCheck(pp != NULL);

  if (pp->state == PRS_STOP) {
    prs_lld_start(pp);
  }
  pp->config = config;
  pp->state  = PRS_READY;
}

void prsStop(PRSDriver *pp) {
  osalDbgCheck(pp != NULL);

  if (pp->state == PRS_READY) {
    prs_lld_stop(pp);
    pp->state  = PRS_STOP;
    pp->config = NULL;
  }
}

void prsConnect(PRSDriver *pp, prs_path_t path, uint8_t ch,
                prs_source_t gen) {
  if (path == PRS_PATH_SYNC) {
    prs_lld_connect_sync(pp, ch, gen);
  } else {
    prs_lld_connect_async(pp, ch, gen);
  }
}

void prsDisconnect(PRSDriver *pp, prs_path_t path, uint8_t ch) {
  if (path == PRS_PATH_SYNC) {
    prs_lld_disconnect_sync(pp, ch);
  } else {
    prs_lld_disconnect_async(pp, ch);
  }
}

void prsSetLogic(PRSDriver *pp, uint8_t ch, prs_fn_t fn, uint8_t aux) {
  prs_lld_set_logic(pp, ch, fn, aux);
}

void prsConnectConsumer(PRSDriver *pp, prs_consumer_t consumer,
                        prs_path_t path, uint8_t ch) {
  prs_lld_connect_consumer(pp, consumer, path, ch);
}

void prsRoutePin(PRSDriver *pp, prs_path_t path, uint8_t ch,
                 ioportid_t port, ioportmask_t pad) {
  if (path == PRS_PATH_SYNC) {
    prs_lld_route_sync_pin(pp, ch, port, pad);
  } else {
    prs_lld_route_async_pin(pp, ch, port, pad);
  }
}

void prsUnroutePin(PRSDriver *pp, prs_path_t path, uint8_t ch) {
  if (path == PRS_PATH_SYNC) {
    prs_lld_unroute_sync_pin(pp, ch);
  } else {
    prs_lld_unroute_async_pin(pp, ch);
  }
}

void prsSwPulse(PRSDriver *pp, uint16_t ch_mask) {
  prs_lld_sw_pulse(pp, ch_mask);
}

void prsSwLevelSet(PRSDriver *pp, uint16_t ch_mask) {
  prs_lld_sw_level_set(pp, ch_mask);
}

void prsSwLevelClear(PRSDriver *pp, uint16_t ch_mask) {
  prs_lld_sw_level_clear(pp, ch_mask);
}

uint32_t prsPeek(PRSDriver *pp, prs_path_t path) {
  if (path == PRS_PATH_SYNC) {
    return prs_lld_peek_sync(pp);
  }
  return prs_lld_peek_async(pp);
}

#endif /* HAL_USE_PRS */

/** @} */
