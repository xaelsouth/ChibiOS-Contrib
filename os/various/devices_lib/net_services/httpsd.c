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
 * @file    devices_lib/net_services/httpsd.c
 * @brief   Minimal HTTPS server (altcp_tls + mbedTLS) with firmware upload.
 *
 * @addtogroup NET_SERVICES
 * @{
 */

#include "hal.h"
#include "httpsd.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "lwip/tcpip.h"
#include "lwip/altcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"

#include "mbedtls/pk.h"
#include "mbedtls/rsa.h"
#include "mbedtls/x509.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/sha256.h"
#include "mbedtls/pem.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

#define HTTPSD_PORT             443U
#define HTTPSD_STACKSIZE        2048U

#define RX_BUF_SIZE             4096U
#define RESP_BUF_SIZE           2048U
#define CERT_PEM_MAX            2048U
#define KEY_PEM_MAX             2048U

#define UPDATE_FILE             "update.bin"
#define UPDATE_SHA_FILE         "update.sha256"
#define CERT_FILE               "cert.pem"
#define KEY_FILE                "key.pem"

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

typedef struct {
  struct altcp_pcb *pcb;          /* NULL once closed by a callback       */
  lfs_t            *lfs;
  /* linear RX accumulator */
  uint8_t           rx[RX_BUF_SIZE];
  size_t            rx_len;
  bool              hdr_done;
  /* parsed request */
  char              method[8];
  char              uri[64];
  long              content_len;
  /* upload state */
  lfs_file_t        upfile;
  bool              uploading;
  bool              file_open;
  long              received;
  /* response/close control */
  bool              close_after_sent;
  semaphore_t       done;
  bool              finished;
} https_conn_t;

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

static lfs_t            *httpsd_lfs;
static semaphore_t       accept_sem;
static struct altcp_pcb *pending_pcb;
static char              cert_pem[CERT_PEM_MAX];
static char              key_pem[KEY_PEM_MAX];
static char              resp_buf[RESP_BUF_SIZE];

/*===========================================================================*/
/* LittleFS helpers.                                                         */
/*===========================================================================*/

static int lfs_read_file(lfs_t *lfs, const char *name, char *buf, size_t max) {
  lfs_file_t f;
  lfs_ssize_t n;

  if (lfs_file_open(lfs, &f, name, LFS_O_RDONLY) != LFS_ERR_OK) {
    return -1;
  }
  n = lfs_file_read(lfs, &f, buf, max - 1U);
  lfs_file_close(lfs, &f);
  if (n < 0) {
    return -1;
  }
  buf[n] = '\0';
  return (int)n;
}

static int lfs_write_file(lfs_t *lfs, const char *name, const char *buf,
                          size_t len) {
  lfs_file_t f;
  lfs_ssize_t n;

  if (lfs_file_open(lfs, &f, name,
                    LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) != LFS_ERR_OK) {
    return -1;
  }
  n = lfs_file_write(lfs, &f, buf, len);
  lfs_file_close(lfs, &f);
  return (n == (lfs_ssize_t)len) ? 0 : -1;
}

/*===========================================================================*/
/* Certificate handling.                                                     */
/*===========================================================================*/

/* Generate a self-signed RSA-2048 certificate (one-time, a few seconds on
   the ARM926 at 480 MHz). */
static int generate_self_signed(char *cert_out, size_t cert_cap,
                                char *key_out, size_t key_cap) {
  mbedtls_pk_context pk;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context drbg;
  mbedtls_x509write_cert crt;
  mbedtls_mpi serial;
  unsigned char der[2048U];
  unsigned char raw_serial[16U];
  int rc, len;
  size_t olen = 0U;

  mbedtls_pk_init(&pk);
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&drbg);
  mbedtls_x509write_crt_init(&crt);
  mbedtls_mpi_init(&serial);

  rc = mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy,
                             (const unsigned char *)"imx23", 5U);
  if (rc != 0) {
    goto out;
  }
  rc = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
  if (rc != 0) {
    goto out;
  }
  rc = mbedtls_rsa_gen_key(mbedtls_pk_rsa(pk), mbedtls_ctr_drbg_random, &drbg,
                           2048U, 65537U);
  if (rc != 0) {
    goto out;
  }

  memset(raw_serial, 0xA5, sizeof(raw_serial));
  mbedtls_mpi_read_binary(&serial, raw_serial, sizeof(raw_serial));

  mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);
  mbedtls_x509write_crt_set_subject_name(&crt, "CN=imx23-evk,O=ChibiOS");
  mbedtls_x509write_crt_set_issuer_name(&crt, "CN=imx23-evk,O=ChibiOS");
  mbedtls_x509write_crt_set_subject_key(&crt, &pk);
  mbedtls_x509write_crt_set_issuer_key(&crt, &pk);
  mbedtls_x509write_crt_set_serial(&crt, &serial);
  mbedtls_x509write_crt_set_validity(&crt, "20260101000000", "20400101000000");
  mbedtls_x509write_crt_set_basic_constraints(&crt, 1, -1);
  mbedtls_x509write_crt_set_key_usage(&crt,
                                      MBEDTLS_X509_KU_DIGITAL_SIGNATURE |
                                      MBEDTLS_X509_KU_KEY_ENCIPHERMENT);

  /* x509write_crt_der writes the DER at the END of the buffer. */
  len = mbedtls_x509write_crt_der(&crt, der, sizeof(der),
                                  mbedtls_ctr_drbg_random, &drbg);
  if (len < 0) {
    rc = len;
    goto out;
  }
  rc = mbedtls_pem_write_buffer("-----BEGIN CERTIFICATE-----\n",
                                "-----END CERTIFICATE-----\n",
                                der + sizeof(der) - (size_t)len, (size_t)len,
                                (unsigned char *)cert_out, cert_cap, &olen);
  if (rc != 0) {
    goto out;
  }

  len = mbedtls_pk_write_key_der(&pk, der, sizeof(der));
  if (len < 0) {
    rc = len;
    goto out;
  }
  rc = mbedtls_pem_write_buffer("-----BEGIN RSA PRIVATE KEY-----\n",
                                "-----END RSA PRIVATE KEY-----\n",
                                der + sizeof(der) - (size_t)len, (size_t)len,
                                (unsigned char *)key_out, key_cap, &olen);

out:
  mbedtls_mpi_free(&serial);
  mbedtls_x509write_crt_free(&crt);
  mbedtls_ctr_drbg_free(&drbg);
  mbedtls_entropy_free(&entropy);
  mbedtls_pk_free(&pk);
  return rc;
}

/* Load cert/key from lfs, generating them on first boot. */
static int ensure_certificate(void) {

  if (lfs_read_file(httpsd_lfs, CERT_FILE, cert_pem, sizeof(cert_pem)) > 0 &&
      lfs_read_file(httpsd_lfs, KEY_FILE, key_pem, sizeof(key_pem)) > 0) {
    return 0;
  }
  if (generate_self_signed(cert_pem, sizeof(cert_pem),
                           key_pem, sizeof(key_pem)) != 0) {
    return -1;
  }
  (void)lfs_write_file(httpsd_lfs, CERT_FILE, cert_pem, strlen(cert_pem));
  (void)lfs_write_file(httpsd_lfs, KEY_FILE, key_pem, strlen(key_pem));
  return 0;
}

/*===========================================================================*/
/* HTTP handling (runs in the tcpip thread via altcp callbacks).             */
/*===========================================================================*/

static void send_response(https_conn_t *c, int status, const char *status_str,
                          const char *body) {
  int n = snprintf(resp_buf, sizeof(resp_buf),
                   "HTTP/1.1 %d %s\r\n"
                   "Content-Type: text/html\r\n"
                   "Content-Length: %d\r\n"
                   "Connection: close\r\n"
                   "\r\n"
                   "%s",
                   status, status_str, (int)strlen(body), body);

  altcp_write(c->pcb, resp_buf, (u16_t)n, TCP_WRITE_FLAG_COPY);
  altcp_output(c->pcb);
  c->close_after_sent = true;
}

static void index_page(https_conn_t *c) {
  const struct netif *n = netif_default;
  char body[1536U];
  char ip[16U] = "?";

  if (n != NULL) {
    snprintf(ip, sizeof(ip), "%u.%u.%u.%u",
             (unsigned)ip4_addr1(&n->ip_addr), (unsigned)ip4_addr2(&n->ip_addr),
             (unsigned)ip4_addr3(&n->ip_addr), (unsigned)ip4_addr4(&n->ip_addr));
  }
  snprintf(body, sizeof(body),
      "<html><head><title>i.MX23</title></head><body>"
      "<h1>i.MX23 EVK - firmware update</h1>"
      "<p>IP: %s</p>"
      "<form method=\"post\" action=\"/update\" "
      "enctype=\"application/octet-stream\">"
      "<input type=\"file\" name=\"fw\">"
      "<input type=\"submit\" value=\"Upload\">"
      "</form>"
      "<p>POST the raw firmware binary to /update. "
      "If %s exists it must hold the expected SHA-256 (hex). "
      "The file is stored as-is; any device-specific re-encryption "
      "(e.g. i.MX23 DCP) is done separately, see the port's sbtool.</p>"
      "</body></html>",
      ip, UPDATE_SHA_FILE);
  send_response(c, 200, "OK", body);
}


/* Finish an upload: close file, compute SHA-256, compare with update.sha256
   if present, respond. */
static void finish_upload(https_conn_t *c, bool ok) {
  char body[1024U];
  unsigned char hash[32U];
  char hash_hex[65U];
  bool verified = false;
  bool have_expect = false;
  char expect[80U];
  lfs_file_t f;
  mbedtls_sha256_context sha;
  uint8_t chunk[512U];
  lfs_ssize_t n;
  unsigned i;

  if (c->file_open) {
    lfs_file_close(c->lfs, &c->upfile);
    c->file_open = false;
  }
  if (!ok) {
    send_response(c, 500, "Internal Server Error",
                  "<html><body><h1>Upload failed</h1></body></html>");
    return;
  }

  mbedtls_sha256_init(&sha);
  mbedtls_sha256_starts(&sha, 0);
  if (lfs_file_open(c->lfs, &f, UPDATE_FILE, LFS_O_RDONLY) == LFS_ERR_OK) {
    while ((n = lfs_file_read(c->lfs, &f, chunk, sizeof(chunk))) > 0) {
      mbedtls_sha256_update(&sha, chunk, (size_t)n);
    }
    lfs_file_close(c->lfs, &f);
  }
  mbedtls_sha256_finish(&sha, hash);
  mbedtls_sha256_free(&sha);
  for (i = 0U; i < 32U; i++) {
    snprintf(hash_hex + i * 2U, 3U, "%02x", hash[i]);
  }

  if (lfs_read_file(c->lfs, UPDATE_SHA_FILE, expect, sizeof(expect)) > 0) {
    have_expect = true;
    for (i = 0U; i < 64U; i++) {
      char ch = expect[i];
      if (ch == '\n' || ch == '\r' || ch == ' ' || ch == '\0') {
        expect[i] = '\0';
        break;
      }
    }
    verified = (strncmp(expect, hash_hex, 64U) == 0);
  }

  snprintf(body, sizeof(body),
      "<html><body><h1>Upload complete</h1>"
      "<p>Bytes: %ld</p><p>SHA-256: %s</p>"
      "<p>Expected-hash file: %s</p><p>Result: %s</p>"
      "</body></html>",
      c->received, hash_hex,
      have_expect ? "present" : "absent",
      have_expect ? (verified ? "VERIFIED" : "MISMATCH") : "not checked");
  send_response(c, 200, "OK", body);
}

static char *ci_strstr(const char *hay, const char *needle) {
  size_t nl = strlen(needle);

  while (*hay != '\0') {
    if (strncasecmp(hay, needle, nl) == 0) {
      return (char *)hay;
    }
    hay++;
  }
  return NULL;
}

/* Parse the request line + content-length from rx[0..hdr_end). */
static bool parse_header(https_conn_t *c, size_t hdr_end) {
  char saved = c->rx[hdr_end];
  char *eol, *cl;
  bool ok;

  c->rx[hdr_end] = '\0';
  eol = strstr((char *)c->rx, "\r\n");
  if (eol == NULL) {
    c->rx[hdr_end] = saved;
    return false;
  }
  *eol = '\0';
  ok = (sscanf((char *)c->rx, "%7s %63s", c->method, c->uri) == 2);
  *eol = '\r';
  c->content_len = 0;
  cl = ci_strstr((char *)c->rx, "Content-Length:");
  if (cl != NULL) {
    c->content_len = strtol(cl + 15U, NULL, 10);
  }
  c->rx[hdr_end] = saved;
  return ok;
}

static err_t https_recv_cb(void *arg, struct altcp_pcb *pcb, struct pbuf *p,
                           err_t err) {
  https_conn_t *c = (https_conn_t *)arg;

  (void)pcb;
  if (err != ERR_OK || p == NULL) {
    /* peer closed / error */
    if (p != NULL) {
      pbuf_free(p);
    }
    c->finished = true;
    chSemSignal(&c->done);
    return ERR_OK;
  }

  /* Append to the linear RX buffer. */
  if (c->rx_len + p->tot_len <= sizeof(c->rx)) {
    pbuf_copy_partial(p, c->rx + c->rx_len, p->tot_len, 0U);
    c->rx_len += p->tot_len;
  }
  else {
    size_t room = sizeof(c->rx) - c->rx_len;
    pbuf_copy_partial(p, c->rx + c->rx_len, (u16_t)room, 0U);
    c->rx_len += room;
  }
  pbuf_free(p);
  c->rx[c->rx_len] = '\0';

  if (!c->hdr_done) {
    char *term = strstr((char *)c->rx, "\r\n\r\n");
    if (term == NULL) {
      if (c->rx_len >= sizeof(c->rx) - 1U) {
        c->hdr_done = true;
        send_response(c, 431, "Request Header Fields Too Large",
                      "<html><body>431</body></html>");
      }
      return ERR_OK;
    }
    size_t hdr_bytes = (size_t)(term - (char *)c->rx) + 4U;
    if (!parse_header(c, (size_t)(term - (char *)c->rx))) {
      c->hdr_done = true;
      send_response(c, 400, "Bad Request", "<html><body>400</body></html>");
      return ERR_OK;
    }
    c->hdr_done = true;

    if (strcmp(c->method, "POST") == 0 && strcmp(c->uri, "/update") == 0) {
      if (lfs_file_open(c->lfs, &c->upfile, UPDATE_FILE,
                        LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) == LFS_ERR_OK) {
        c->file_open = true;
        c->uploading = true;
        c->received  = 0;
      }
      else {
        send_response(c, 500, "Internal Server Error",
                      "<html><body>cannot open update.bin</body></html>");
        return ERR_OK;
      }
    }
    else if (strcmp(c->method, "GET") == 0) {
      index_page(c);
      c->rx_len = 0;
      return ERR_OK;
    }
    else {
      send_response(c, 404, "Not Found", "<html><body>404</body></html>");
      c->rx_len = 0;
      return ERR_OK;
    }
    /* fall through: body bytes may already be in rx */
    if (c->rx_len > hdr_bytes) {
      size_t body_len = c->rx_len - hdr_bytes;
      lfs_file_write(c->lfs, &c->upfile, c->rx + hdr_bytes, body_len);
      c->received += (long)body_len;
      c->rx_len = 0;
    }
    if (c->received >= c->content_len) {
      finish_upload(c, true);
    }
    return ERR_OK;
  }

  if (c->uploading && c->rx_len > 0U) {
    lfs_file_write(c->lfs, &c->upfile, c->rx, c->rx_len);
    c->received += (long)c->rx_len;
    c->rx_len = 0;
    if (c->received >= c->content_len) {
      finish_upload(c, true);
    }
  }
  return ERR_OK;
}

static err_t https_sent_cb(void *arg, struct altcp_pcb *pcb, u16_t len) {
  https_conn_t *c = (https_conn_t *)arg;

  (void)len;
  if (c->close_after_sent) {
    c->close_after_sent = false;
    altcp_close(pcb);
    c->pcb = NULL;
    c->finished = true;
    chSemSignal(&c->done);
  }
  return ERR_OK;
}

static void https_err_cb(void *arg, err_t err) {
  https_conn_t *c = (https_conn_t *)arg;

  (void)err;
  c->pcb = NULL;
  c->finished = true;
  chSemSignal(&c->done);
}

static err_t https_accept_cb(void *arg, struct altcp_pcb *newpcb, err_t err) {

  (void)arg; (void)err;
  pending_pcb = newpcb;
  chSemSignal(&accept_sem);
  return ERR_OK;
}

/*===========================================================================*/
/* Server thread.                                                            */
/*===========================================================================*/

static THD_WORKING_AREA(httpsd_wa, HTTPSD_STACKSIZE);
static THD_FUNCTION(httpsd_thread, arg) {
  struct altcp_tls_config *tls_conf;
  struct altcp_pcb *lpcb;

  (void)arg;
  chRegSetThreadName("httpsd");

  if (ensure_certificate() != 0) {
    return;     /* no cert -> no https */
  }

  tls_conf = altcp_tls_create_config_server_privkey_cert(
      (const u8_t *)key_pem, strlen(key_pem) + 1U, NULL, 0U,
      (const u8_t *)cert_pem, strlen(cert_pem) + 1U);
  if (tls_conf == NULL) {
    return;
  }
  lpcb = altcp_tls_new(tls_conf, IPADDR_TYPE_ANY);
  if (lpcb == NULL) {
    altcp_tls_free_config(tls_conf);
    return;
  }
  if (altcp_bind(lpcb, IP_ADDR_ANY, HTTPSD_PORT) != ERR_OK) {
    return;
  }
  lpcb = altcp_listen(lpcb);
  if (lpcb == NULL) {
    return;
  }
  altcp_accept(lpcb, https_accept_cb);

  while (!chThdShouldTerminateX()) {
    static https_conn_t conn;

    chSemWait(&accept_sem);
    if (pending_pcb == NULL) {
      continue;
    }
    memset(&conn, 0, sizeof(conn));
    chSemObjectInit(&conn.done, 0);
    conn.pcb = pending_pcb;
    conn.lfs = httpsd_lfs;
    pending_pcb = NULL;

    altcp_arg(conn.pcb, &conn);
    altcp_recv(conn.pcb, https_recv_cb);
    altcp_sent(conn.pcb, https_sent_cb);
    altcp_err(conn.pcb, https_err_cb);

    /* wait until the connection is done */
    while (!conn.finished && !chThdShouldTerminateX()) {
      chSemWaitTimeout(&conn.done, TIME_MS2I(500));
    }
    if (conn.file_open) {
      lfs_file_close(conn.lfs, &conn.upfile);
      conn.file_open = false;
    }
    if (conn.pcb != NULL) {
      altcp_close(conn.pcb);
      conn.pcb = NULL;
    }
  }
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

void httpsd_start(lfs_t *lfs) {

  httpsd_lfs = lfs;
  chSemObjectInit(&accept_sem, 0);
  chThdCreateStatic(httpsd_wa, sizeof(httpsd_wa),
                    NORMALPRIO - 2U, httpsd_thread, NULL);
}

/** @} */
