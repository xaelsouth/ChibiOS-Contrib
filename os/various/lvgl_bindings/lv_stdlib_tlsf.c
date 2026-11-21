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
 * @file    lvgl_bindings/lv_stdlib_tlsf.c
 * @brief   LVGL custom stdlib allocator over the shared TLSF heap.
 *
 * @details With LV_USE_STDLIB_MALLOC == LV_STDLIB_CUSTOM, LVGL expects the
 *          application to provide lv_malloc_core/lv_realloc_core/lv_free_core
 *          (plus the monitor/test hooks). They are routed to the global
 *          TLSF heap (tlsf_bindings), which is mutex-protected so lwIP,
 *          littlefs and LVGL can share it from different threads.
 *
 * @addtogroup LVGL_BINDINGS
 * @{
 */

#include "lvgl/lvgl.h"

#if LV_USE_STDLIB_MALLOC == LV_STDLIB_CUSTOM

#include "tlsf_bindings.h"

/* The TLSF heap is initialized by tlsfHeapInit() from the application. */
void lv_mem_init(void) {}
void lv_mem_deinit(void) {}

void *lv_malloc_core(size_t size) {

  return tlsfHeapAlloc(size);
}

void *lv_realloc_core(void *p, size_t new_size) {

  return tlsfHeapRealloc(p, new_size);
}

void lv_free_core(void *p) {

  tlsfHeapFree(p);
}

void lv_mem_monitor_core(lv_mem_monitor_t *mon_p) {

  /* No per-pool stats available from the wrapper; report the basics. */
  mon_p->total_size        = TLSF_POOL_SIZE;
  mon_p->free_size         = (size_t)tlsfHeapLargest();
  mon_p->free_biggest_size = mon_p->free_size;
  mon_p->free_cnt          = 0U;
  mon_p->used_cnt          = 0U;
  mon_p->max_used          = 0U;
  mon_p->used_pct          = 0U;
  mon_p->frag_pct          = 0U;
}

lv_result_t lv_mem_test_core(void) {

  return LV_RESULT_OK;
}

#endif /* LV_USE_STDLIB_MALLOC == LV_STDLIB_CUSTOM */

/** @} */
