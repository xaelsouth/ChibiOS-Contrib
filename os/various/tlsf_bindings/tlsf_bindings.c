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
 * @file    tlsf_bindings.c
 * @brief   TLSF constant-time allocator bindings code.
 *
 * @addtogroup TLSF_BINDINGS
 * @{
 */

#include "tlsf_bindings.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/* The global pool lives in SDRAM (static object -> .bss in RAM). */
static uint8_t tlsf_pool[TLSF_POOL_SIZE] __attribute__((aligned(8)));
static tlsf_t  tlsf_heap;
static bool    tlsf_ready;

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Initializes the global TLSF heap over the static pool.
 * @note    Call once after halInit()/chSysInit().
 */
void tlsfHeapInit(void) {

  tlsf_heap = tlsf_create_with_pool(tlsf_pool, sizeof(tlsf_pool));
  osalDbgAssert(tlsf_heap != NULL, "tlsf_create_with_pool failed");
  tlsf_ready = true;
}

void *tlsfHeapAlloc(size_t size) {

  osalDbgCheck(tlsf_ready);
  return tlsf_malloc(tlsf_heap, size);
}

void *tlsfHeapAllocAligned(size_t size, size_t align) {

  osalDbgCheck(tlsf_ready);
  return tlsf_memalign(tlsf_heap, align, size);
}

void tlsfHeapFree(void *p) {

  if (p != NULL) {
    tlsf_free(tlsf_heap, p);
  }
}

size_t tlsfHeapLargest(void) {

  return tlsf_heap ? tlsf_block_size_max() : 0U;
}

/** @} */
