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

#include <string.h>

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
/* The heap is shared between users on different threads (lwIP mem,
   LVGL, littlefs, ...), so serialize all pool operations. */
static mutex_t tlsf_lock;

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Initializes the global TLSF heap over the static pool.
 * @note    Call once after halInit()/chSysInit().
 */
void tlsfHeapInit(void) {

  chMtxObjectInit(&tlsf_lock);
  tlsf_heap = tlsf_create_with_pool(tlsf_pool, sizeof(tlsf_pool));
  osalDbgAssert(tlsf_heap != NULL, "tlsf_create_with_pool failed");
  tlsf_ready = true;
}

void *tlsfHeapAlloc(size_t size) {
  void *p;

  osalDbgCheck(tlsf_ready);
  chMtxLock(&tlsf_lock);
  p = tlsf_malloc(tlsf_heap, size);
  chMtxUnlock(&tlsf_lock);
  return p;
}

void *tlsfHeapAllocAligned(size_t size, size_t align) {
  void *p;

  osalDbgCheck(tlsf_ready);
  chMtxLock(&tlsf_lock);
  p = tlsf_memalign(tlsf_heap, align, size);
  chMtxUnlock(&tlsf_lock);
  return p;
}

void *tlsfHeapCalloc(size_t count, size_t size) {
  void *p = tlsfHeapAlloc(count * size);

  if (p != NULL) {
    memset(p, 0, count * size);
  }
  return p;
}

void *tlsfHeapRealloc(void *p, size_t size) {
  void *np;

  osalDbgCheck(tlsf_ready);
  chMtxLock(&tlsf_lock);
  np = tlsf_realloc(tlsf_heap, p, size);
  chMtxUnlock(&tlsf_lock);
  return np;
}

void tlsfHeapFree(void *p) {

  if (p != NULL) {
    chMtxLock(&tlsf_lock);
    tlsf_free(tlsf_heap, p);
    chMtxUnlock(&tlsf_lock);
  }
}

size_t tlsfHeapLargest(void) {

  return tlsf_heap ? tlsf_block_size_max() : 0U;
}

/** @} */
