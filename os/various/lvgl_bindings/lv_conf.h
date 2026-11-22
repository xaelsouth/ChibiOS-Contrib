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
 * @file    lvgl_bindings/lv_conf.h
 * @brief   LVGL configuration for ChibiOS (minimal overrides).
 *
 * @details Only the options that differ from the LVGL defaults are set
 *          here; lv_conf_internal.h provides defaults for everything else.
 *          This file is found via __has_include("lv_conf.h") because the
 *          bindings directory is on the include path (see lvgl.mk).
 *
 * @addtogroup LVGL_BINDINGS
 * @{
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*--------------------------------------------------------------------------*/
/* Display / color.                                                          */
/*--------------------------------------------------------------------------*/
/* Default color format: RGB565 (16 bpp) — compact, and the usual choice for
   small embedded displays. Ports whose display controller needs another
   format (or a hardware pixel-conversion step) override this per project
   with -DLV_COLOR_FORMAT_DEFAULT=... and handle the flush in their LVGL
   port glue. */
#define LV_COLOR_FORMAT_DEFAULT     LV_COLOR_FORMAT_RGB565

/*--------------------------------------------------------------------------*/
/* Allocator / stdlib.                                                       */
/*--------------------------------------------------------------------------*/
/* Route LVGL allocations to the shared TLSF heap (lv_stdlib_tlsf.c). */
#define LV_USE_STDLIB_MALLOC        LV_STDLIB_CUSTOM
#define LV_USE_STDLIB_STRING        LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF       LV_STDLIB_CLIB

/*--------------------------------------------------------------------------*/
/* OS / system.                                                              */
/*--------------------------------------------------------------------------*/
#define LV_USE_OS                   LV_OS_NONE
#define LV_USE_LOG                  0
#define LV_USE_SYSMON               0
#define LV_USE_PERF_MONITOR         0
#define LV_USE_MEM_MONITOR          0

/*--------------------------------------------------------------------------*/
/* Drawing.                                                                  */
/*--------------------------------------------------------------------------*/
#define LV_USE_DRAW_SW              1
#define LV_USE_DRAW_SW_ASM          LV_DRAW_SW_ASM_NONE   /* ARM926: no NEON */

#define LV_DEF_REFR_PERIOD          33      /* ms, ~30 fps                */
#define LV_DPI_DEF                  130

/*--------------------------------------------------------------------------*/
/* Widgets / theme used by the demos.                                        */
/*--------------------------------------------------------------------------*/
#define LV_USE_BUTTON               1
#define LV_USE_LABEL                1
#define LV_USE_SLIDER               1
#define LV_USE_ARC                  1
#define LV_USE_BAR                  1
#define LV_USE_SWITCH               1
#define LV_USE_THEME_DEFAULT        1
#define LV_FONT_MONTSERRAT_14       1

#endif /* LV_CONF_H */

/** @} */
