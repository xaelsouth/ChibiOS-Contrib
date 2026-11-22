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
 * @file    demos/IMX23/RT-IMX23-HELLO/net/tftpd_lfs.h
 * @brief   lwIP TFTP server backed by a LittleFS volume.
 *
 * @details Maps the TFTP file operations (open/read/write/close) onto a
 *          mounted littlefs instance, so a field host can push/pull files
 *          with a standard tftp client (read and write).
 *
 * @addtogroup NET_SERVICES
 * @{
 */

#ifndef TFTPD_LFS_H
#define TFTPD_LFS_H

#include "hal.h"
#include "lfs.h"

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  /* Start the TFTP server (UDP port 69) on the given mounted littlefs
     volume. Safe to call from thread context after lwIP is up. */
  void tftpd_lfs_start(lfs_t *lfs);
#ifdef __cplusplus
}
#endif

#endif /* TFTPD_LFS_H */

/** @} */
