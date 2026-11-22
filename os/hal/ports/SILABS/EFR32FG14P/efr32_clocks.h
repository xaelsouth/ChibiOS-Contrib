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
 * @file    efr32_clocks.h
 * @brief   EFR32FG14P clock inspection helpers.
 *
 * @details Declared in a separate header (not hal_lld.h) because
 *          BaseSequentialStream is only defined later in hal.h.
 *          Used e.g. by the xshell "clocks" command
 *          (LLD/xshell/efr32_clock_cmds.c).
 *
 * @addtogroup EFR32FG14P_CLOCKS
 * @{
 */

#ifndef EFR32_CLOCKS_H
#define EFR32_CLOCKS_H

#include "hal.h"

#ifdef __cplusplus
extern "C" {
#endif

  void efr32_clock_points_print(BaseSequentialStream *ssp);

#ifdef __cplusplus
}
#endif

#endif /* EFR32_CLOCKS_H */

/** @} */
