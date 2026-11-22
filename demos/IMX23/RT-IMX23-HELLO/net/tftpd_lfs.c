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
 * @file    demos/IMX23/RT-IMX23-HELLO/net/tftpd_lfs.c
 * @brief   lwIP TFTP server backed by a LittleFS volume.
 *
 * @addtogroup NET_SERVICES
 * @{
 */

#include "hal.h"
#include "tftpd_lfs.h"

#include <string.h>

#include "lwip/apps/tftp_server.h"
#include "lwip/tcpip.h"
#include "lwip/pbuf.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/* Max simultaneously open files through TFTP. */
#define TFTPD_MAX_FILES         2U

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

typedef struct {
  lfs_t     *lfs;
  lfs_file_t file[TFTPD_MAX_FILES];
  bool       used[TFTPD_MAX_FILES];
} tftpd_lfs_ctx_t;

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

static tftpd_lfs_ctx_t tftpd_ctx;

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

static void *tftp_open(const char *fname, const char *mode, u8_t write) {
  tftpd_lfs_ctx_t *ctx = &tftpd_ctx;
  unsigned i;
  int flags;

  (void)mode;
  /* Strip a leading '/' so paths are relative to the lfs root. */
  if (fname[0] == '/') {
    fname++;
  }
  if (fname[0] == '\0') {
    return NULL;
  }

  for (i = 0U; i < TFTPD_MAX_FILES; i++) {
    if (!ctx->used[i]) {
      break;
    }
  }
  if (i == TFTPD_MAX_FILES) {
    return NULL;
  }

  flags = write ? (LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC)
                : LFS_O_RDONLY;
  if (lfs_file_open(ctx->lfs, &ctx->file[i], fname, flags) != LFS_ERR_OK) {
    return NULL;
  }
  ctx->used[i] = true;
  return &ctx->file[i];
}

static void tftp_close(void *handle) {
  tftpd_lfs_ctx_t *ctx = &tftpd_ctx;
  unsigned i = (unsigned)((lfs_file_t *)handle - ctx->file);

  if (i < TFTPD_MAX_FILES && ctx->used[i]) {
    lfs_file_close(ctx->lfs, &ctx->file[i]);
    ctx->used[i] = false;
  }
}

static int tftp_read(void *handle, void *buf, int bytes) {
  tftpd_lfs_ctx_t *ctx = &tftpd_ctx;
  lfs_ssize_t n;

  n = lfs_file_read(ctx->lfs, (lfs_file_t *)handle, buf, (lfs_size_t)bytes);
  return (n < 0) ? (int)n : (int)n;
}

static int tftp_write(void *handle, struct pbuf *p) {
  tftpd_lfs_ctx_t *ctx = &tftpd_ctx;
  lfs_ssize_t n;

  n = lfs_file_write(ctx->lfs, (lfs_file_t *)handle,
                     p->payload, p->len);
  return (n < 0) ? (int)n : (int)n;
}

/* Runs in the tcpip thread via tcpip_callback. */
static void tftp_init_cb(void *arg) {
  struct tftp_context ctx;

  (void)arg;
  memset(&ctx, 0, sizeof(ctx));
  ctx.open  = tftp_open;
  ctx.close = tftp_close;
  ctx.read  = tftp_read;
  ctx.write = tftp_write;
  tftp_init(&ctx);
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

void tftpd_lfs_start(lfs_t *lfs) {

  tftpd_ctx.lfs = lfs;
  memset(tftpd_ctx.used, 0, sizeof(tftpd_ctx.used));
  tcpip_callback(tftp_init_cb, NULL);
}

/** @} */
