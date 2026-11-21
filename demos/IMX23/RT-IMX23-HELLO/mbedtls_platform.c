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
 * @file    mbedtls_platform.c
 * @brief   mbedTLS platform hooks: millisecond time from the ChibiOS
 *          system tick (MBEDTLS_PLATFORM_MS_TIME_ALT).
 */

#include "ch.h"
#include "mbedtls/platform.h"

#include <sys/time.h>

#if defined(MBEDTLS_PLATFORM_MS_TIME_ALT)

mbedtls_ms_time_t mbedtls_ms_time(void) {

  return (mbedtls_ms_time_t)(((uint64_t)chVTGetSystemTimeX() * 1000U) /
                             CH_CFG_ST_FREQUENCY);
}

#endif

/* newlib's gettimeofday() calls _gettimeofday(); provide it from the
   system tick so time()/gettimeofday() link and run. */
int _gettimeofday(struct timeval *tv, void *tz) {
  uint64_t ms = ((uint64_t)chVTGetSystemTimeX() * 1000U) / CH_CFG_ST_FREQUENCY;

  (void)tz;
  if (tv != NULL) {
    tv->tv_sec  = (time_t)(ms / 1000U);
    tv->tv_usec = (suseconds_t)((ms % 1000U) * 1000U);
  }
  return 0;
}
