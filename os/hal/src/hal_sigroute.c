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
 * @file    hal_sigroute.c
 * @brief   Generic peripheral reflex/routing driver (upper layer).
 *
 * @addtogroup SIGROUTE
 * @{
 */

#include "hal_sigroute.h"

#if (HAL_USE_SIGROUTE == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void sigrouteInit(void) {
  sigroute_lld_init();
}

void sigrouteObjectInit(SIGROUTEDriver *pp) {
  pp->state  = SIGROUTE_STOP;
  pp->config = NULL;
}

void sigrouteStart(SIGROUTEDriver *pp, const SIGROUTEConfig *config) {
  osalDbgCheck(pp != NULL);

  if (pp->state == SIGROUTE_STOP) {
    sigroute_lld_start(pp);
  }
  pp->config = config;
  pp->state  = SIGROUTE_READY;
}

void sigrouteStop(SIGROUTEDriver *pp) {
  osalDbgCheck(pp != NULL);

  if (pp->state == SIGROUTE_READY) {
    sigroute_lld_stop(pp);
    pp->state  = SIGROUTE_STOP;
    pp->config = NULL;
  }
}

void sigrouteConnect(SIGROUTEDriver *pp, sigroute_path_t path, uint8_t ch,
                sigroute_source_t gen) {
  if (path == SIGROUTE_PATH_SYNC) {
    sigroute_lld_connect_sync(pp, ch, gen);
  } else {
    sigroute_lld_connect_async(pp, ch, gen);
  }
}

void sigrouteDisconnect(SIGROUTEDriver *pp, sigroute_path_t path, uint8_t ch) {
  if (path == SIGROUTE_PATH_SYNC) {
    sigroute_lld_disconnect_sync(pp, ch);
  } else {
    sigroute_lld_disconnect_async(pp, ch);
  }
}

void sigrouteSetLogic(SIGROUTEDriver *pp, uint8_t ch, sigroute_fn_t fn, uint8_t aux) {
  sigroute_lld_set_logic(pp, ch, fn, aux);
}

void sigrouteConnectConsumer(SIGROUTEDriver *pp, sigroute_consumer_t consumer,
                        sigroute_path_t path, uint8_t ch) {
  sigroute_lld_connect_consumer(pp, consumer, path, ch);
}

void sigrouteRoutePin(SIGROUTEDriver *pp, sigroute_path_t path, uint8_t ch,
                 ioportid_t port, ioportmask_t pad) {
  if (path == SIGROUTE_PATH_SYNC) {
    sigroute_lld_route_sync_pin(pp, ch, port, pad);
  } else {
    sigroute_lld_route_async_pin(pp, ch, port, pad);
  }
}

void sigrouteUnroutePin(SIGROUTEDriver *pp, sigroute_path_t path, uint8_t ch) {
  if (path == SIGROUTE_PATH_SYNC) {
    sigroute_lld_unroute_sync_pin(pp, ch);
  } else {
    sigroute_lld_unroute_async_pin(pp, ch);
  }
}

void sigrouteSwPulse(SIGROUTEDriver *pp, uint16_t ch_mask) {
  sigroute_lld_sw_pulse(pp, ch_mask);
}

void sigrouteSwLevelSet(SIGROUTEDriver *pp, uint16_t ch_mask) {
  sigroute_lld_sw_level_set(pp, ch_mask);
}

void sigrouteSwLevelClear(SIGROUTEDriver *pp, uint16_t ch_mask) {
  sigroute_lld_sw_level_clear(pp, ch_mask);
}

uint32_t sigroutePeek(SIGROUTEDriver *pp, sigroute_path_t path) {
  if (path == SIGROUTE_PATH_SYNC) {
    return sigroute_lld_peek_sync(pp);
  }
  return sigroute_lld_peek_async(pp);
}

#endif /* HAL_USE_SIGROUTE */

/** @} */
