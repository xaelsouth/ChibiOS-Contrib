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
 * @file    RAILv1/hal_rail_lld.h
 * @brief   EFR32FG23 RAIL (Radio Abstraction Interface Library) glue header.
 *
 * @details This module provides the emlib/RAIL compatibility shims that the
 *          Gecko SDK RAIL library expects from the application:
 *          - CORE_EnterCritical/ExitCritical/EnterAtomic/ExitAtomic
 *          - SystemSYSCLKGet, SystemHFXOClockGet, SystemHFRCODPLLClockGet
 *          - CMU_ClockEnable
 *          - SYSTEM_ChipRevisionGet
 *          - OSAL IRQ handler wrappers for all radio IRQs
 *
 *          Enable with EFR32_RAIL_USE_RAIL = TRUE in mcuconf.h.
 *
 * @addtogroup RAIL
 * @{
 */

#ifndef HAL_RAIL_LLD_H
#define HAL_RAIL_LLD_H

#if !defined(EFR32_RAIL_USE_RAIL) || defined(__DOXYGEN__)
#define EFR32_RAIL_USE_RAIL     FALSE
#endif

#endif /* HAL_RAIL_LLD_H */

/** @} */
