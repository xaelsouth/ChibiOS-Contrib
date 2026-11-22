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
 * @file    demos/IMX23/RT-IMX23-HELLO/net/httpsd.h
 * @brief   Minimal HTTPS server (altcp_tls + mbedTLS) with a firmware
 *          upload web UI.
 *
 * @details Listens on TCP/443 (TLS). Serves a small HTML UI on GET / and
 *          accepts POST /update with the raw firmware image body, streaming
 *          it into <lfs>/update.bin, then SHA-256 verifies it against
 *          <lfs>/update.sha256 (if present) and reports the result.
 *
 *          On first start a self-signed RSA-2048 certificate is generated
 *          with mbedTLS and stored in the LittleFS volume (cert.pem/key.pem).
 *
 * @addtogroup NET_SERVICES
 * @{
 */

#ifndef HTTPSD_H
#define HTTPSD_H

#include "hal.h"
#include "lfs.h"

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  /* Start the HTTPS server thread. lfs must be a mounted littlefs volume.
     Safe to call from thread context after lwIP is up. */
  void httpsd_start(lfs_t *lfs);
#ifdef __cplusplus
}
#endif

#endif /* HTTPSD_H */

/** @} */
