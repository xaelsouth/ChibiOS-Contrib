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
 * @file    crc16iec870ft3.h
 * @brief   CRC-16 IEC 870 FT3 (M-Bus) — compatibility wrapper.
 *
 * @details The implementation now lives in crcsw.c (merged standalone CRC
 *          algorithms). This header only pulls in crcsw.h, which provides
 *          the inline one-shot crc16iec870ft3() and the
 *          crcsw_crc16iec870ft3_cfg configuration.
 *
 * @addtogroup CRC
 * @{
 */

#ifndef CRC16IEC870FT3_H
#define CRC16IEC870FT3_H

#include "crcsw.h"

#endif /* CRC16IEC870FT3_H */

/** @} */
