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

/* Compat shim: mbedTLS 3.x removed mbedtls/net.h (legacy socket layer) but
   lwIP 2.1.2's altcp_tls_mbedtls.c still includes it and uses the
   MBEDTLS_ERR_NET_* codes (only as glue return values/comparisons; altcp
   does its own I/O). Recreate the 2.x error constants here. */

#ifndef MBEDTLS_NET_H_SHIM
#define MBEDTLS_NET_H_SHIM

#ifndef MBEDTLS_ERR_NET_SOCKET_FAILED
#define MBEDTLS_ERR_NET_SOCKET_FAILED       (-0x0042)
#endif
#ifndef MBEDTLS_ERR_NET_CONNECT_FAILED
#define MBEDTLS_ERR_NET_CONNECT_FAILED      (-0x0044)
#endif
#ifndef MBEDTLS_ERR_NET_BIND_FAILED
#define MBEDTLS_ERR_NET_BIND_FAILED         (-0x0046)
#endif
#ifndef MBEDTLS_ERR_NET_LISTEN_FAILED
#define MBEDTLS_ERR_NET_LISTEN_FAILED       (-0x0048)
#endif
#ifndef MBEDTLS_ERR_NET_ACCEPT_FAILED
#define MBEDTLS_ERR_NET_ACCEPT_FAILED       (-0x004A)
#endif
#ifndef MBEDTLS_ERR_NET_RECV_FAILED
#define MBEDTLS_ERR_NET_RECV_FAILED         (-0x004C)
#endif
#ifndef MBEDTLS_ERR_NET_SEND_FAILED
#define MBEDTLS_ERR_NET_SEND_FAILED         (-0x004E)
#endif
#ifndef MBEDTLS_ERR_NET_CONN_RESET
#define MBEDTLS_ERR_NET_CONN_RESET          (-0x0050)
#endif
#ifndef MBEDTLS_ERR_NET_INVALID_CONTEXT
#define MBEDTLS_ERR_NET_INVALID_CONTEXT     (-0x0052)
#endif

#endif
