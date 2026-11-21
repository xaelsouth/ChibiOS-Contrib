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
 * @file    mbedtls_config.h
 * @brief   mbedTLS configuration for the i.MX23 demo (HTTPS server with a
 *          runtime-generated self-signed RSA-2048 certificate).
 */

#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

/* Crypto primitives. */
#define MBEDTLS_AES_C
#define MBEDTLS_GCM_C
#define MBEDTLS_CCM_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_MD_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA1_C
#define MBEDTLS_RSA_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_OID_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_GENPRIME

/* RNG (software fallback in mbedtls_bindings/swrng.inc). */
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_ENTROPY_MAX_SOURCES     2
#define MBEDTLS_NO_PLATFORM_ENTROPY

/* X.509 / PKI: parse (TLS) + write (self-signed cert generation). */
#define MBEDTLS_X509_USE_C
#define MBEDTLS_X509_CREATE_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_X509_CRT_WRITE_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_PK_WRITE_C
#define MBEDTLS_PKCS1_V15
#define MBEDTLS_PEM_PARSE_C
#define MBEDTLS_PEM_WRITE_C
#define MBEDTLS_BASE64_C

/* TLS server, TLS 1.2, RSA key exchange (RSA-2048 cert). */
#define MBEDTLS_SSL_CLI_C
#define MBEDTLS_SSL_SRV_C
#define MBEDTLS_SSL_TLS_C
#define MBEDTLS_SSL_PROTO_TLS1_2
#define MBEDTLS_SSL_SERVER_NAME_INDICATION
#define MBEDTLS_SSL_CACHE_C
#define MBEDTLS_DEBUG_C
#define MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
#define MBEDTLS_SSL_CIPHERSUITES                          \
    MBEDTLS_TLS_RSA_WITH_AES_128_GCM_SHA256,              \
    MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA256

/* Buffers: TLS handshake on a 480 MHz ARM926 needs some headroom. */
#define MBEDTLS_SSL_IN_CONTENT_LEN      4096
#define MBEDTLS_SSL_OUT_CONTENT_LEN     4096

#define MBEDTLS_NET_C
#define MBEDTLS_TIMING_C

/* Wall-clock time: bare-metal newlib has no _POSIX_VERSION, so mbedtls
   requires MBEDTLS_PLATFORM_MS_TIME_ALT and we provide mbedtls_ms_time
   from the ChibiOS system tick (mbedtls_platform.c). */
#define MBEDTLS_HAVE_TIME
#define MBEDTLS_PLATFORM_MS_TIME_ALT

/* No filesystem. */
/* MBEDTLS_FS_IO off. */

#endif /* MBEDTLS_CONFIG_H */
