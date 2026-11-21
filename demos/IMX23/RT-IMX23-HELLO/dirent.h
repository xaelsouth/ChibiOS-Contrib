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

/* Stub: mbedTLS's x509_crt.c includes <dirent.h> on non-Windows even when
   MBEDTLS_FS_IO is off; arm-none-eabi newlib's <dirent.h> is a hard #error.
   This stub (found first via -I.) satisfies the include; the DIR type is
   only used under MBEDTLS_FS_IO, which we do not enable. */

#ifndef DIRENT_H_STUB
#define DIRENT_H_STUB

typedef struct DIR DIR;
struct dirent {
  char d_name[256];
};

#endif
