/*
    ChibiOS - Copyright (C) 2026 Xael South.

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
 * @file    demos/IMX23/RT-IMX23-HELLO/net/sio_sio.h
 * @brief   lwIP sio_* serial shim over ChibiOS SIO drivers.
 *
 * @details lwIP's slipif (netif/slipif.c) expects the port to provide the
 *          sio_open/sio_read/sio_tryread/sio_send/sio_write/sio_read_abort
 *          functions declared in lwip/sio.h. This module implements them on
 *          top of ChibiOS SIODriver objects registered with
 *          sio_sio_register().
 *
 * @addtogroup NET_SERVICES
 * @{
 */

#ifndef SIO_SIO_H
#define SIO_SIO_H

#include "hal.h"

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void      sio_sio_register(uint8_t devnum, SIODriver *siop);
  SIODriver *sio_sio_lookup(uint8_t devnum);
#ifdef __cplusplus
}
#endif

#endif /* SIO_SIO_H */

/** @} */
