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

/* Compat shim: lwIP 2.1.2's altcp_tls_mbedtls.c includes
   "mbedtls/ssl_internal.h" (present in mbedTLS 2.x, removed in 3.x where
   the declaration moved to the private library/ssl_misc.h). The symbol
   mbedtls_ssl_flush_output still exists in the compiled library, so
   declaring it here is enough. */

#ifndef MBEDTLS_SSL_INTERNAL_H_SHIM
#define MBEDTLS_SSL_INTERNAL_H_SHIM

#include "mbedtls/ssl.h"

int mbedtls_ssl_flush_output(mbedtls_ssl_context *ssl);

/* mbedTLS 3.x dropped the MBEDTLS_SSL_MAX_CONTENT_LEN alias. */
#ifndef MBEDTLS_SSL_MAX_CONTENT_LEN
#define MBEDTLS_SSL_MAX_CONTENT_LEN    MBEDTLS_SSL_IN_CONTENT_LEN
#endif

#endif
