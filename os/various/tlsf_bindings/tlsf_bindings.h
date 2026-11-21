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
 * @file    tlsf_bindings.h
 * @brief   TLSF constant-time allocator bindings header.
 *
 * @details Wraps the TLSF two-level segregated fit allocator (ext/tlsf,
 *          Matthew Conte, BSD-style license, see the header of tlsf.h)
 *          in a single global pool carved out of SDRAM, and exposes a
 *          chHeapAlloc/chHeapFree-compatible API so it can be used in
 *          place of the first-fit ChibiOS heap (e.g. for littlefs
 *          lfs_malloc or lwIP MEM_CUSTOM_ALLOC).
 *
 * @addtogroup TLSF_BINDINGS
 * @{
 */

#ifndef TLSF_BINDINGS_H
#define TLSF_BINDINGS_H

#include "hal.h"
#include "tlsf.h"

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   Size of the global TLSF pool in bytes.
 */
#if !defined(TLSF_POOL_SIZE) || defined(__DOXYGEN__)
#define TLSF_POOL_SIZE          (1024U * 1024U)
#endif

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void  tlsfHeapInit(void);
  void *tlsfHeapAlloc(size_t size);
  void *tlsfHeapAllocAligned(size_t size, size_t align);
  void  tlsfHeapFree(void *p);
  size_t tlsfHeapLargest(void);
#ifdef __cplusplus
}
#endif

#endif /* TLSF_BINDINGS_H */

/** @} */
