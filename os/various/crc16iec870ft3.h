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
 * @brief   CRC-16 IEC 870 FT3 software calculation.
 *
 * @details Polynomial 0x3D65, initial value 0x0000, final XOR 0xFFFF.
 *          Used by M-Bus (EN 13757) long frame checksum.
 *
 * @addtogroup CRC
 * @{
 */

#ifndef CRC16IEC870FT3_H
#define CRC16IEC870FT3_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Calculate CRC-16 IEC 870 FT3 over a data blob.
 *
 * @param[in] blob      pointer to the data
 * @param[in] bloblen   length of the data in bytes
 *
 * @return              The computed CRC-16 value.
 */
uint16_t crc16iec870ft3(const void *blob, size_t bloblen);

#ifdef __cplusplus
}
#endif

#endif /* CRC16IEC870FT3_H */

/** @} */
