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
 * @file    devices_lib/crypto/mxs_dcp.c
 * @brief   MXS-family DCP (Data Co-Path) hardware crypto engine driver.
 *
 * @details Register model follows Linux drivers/crypto/mxs-dcp.c. The DCP has
 *          its own AHB master and walks 8-word descriptors; with the MMU
 *          off, RAM addresses are physical addresses. Channel 0 is used,
 *          completion is via IRQ 53 (DCP0).
 */

#include "hal.h"
#if defined(__has_include)
#if __has_include("imx23.h")
#define MXS_DCP_HAVE_IMX23_PORT 1
#include "imx23.h"
#include "icoll.h"
#endif
#endif
#include "mxs_dcp.h"

#include <string.h>

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

#define DCP_CHAN                0U
#define DCP_TIMEOUT_MS          1000U

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

MXSDCPDriver MXSDCP1;

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

static MXSDCPDriver *dcp_active;

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

static bool dcp_irq_handler(void) {
  uint32_t stat = MXS_DCP_REG(MXS_DCP_STAT);

  if ((stat & (1U << DCP_CHAN)) != 0U) {
    dcp_active->last_ch_stat = MXS_DCP_REG(MXS_DCP_CH_STAT(DCP_CHAN));
    MXS_DCP_REG(MXS_DCP_CH_STAT_CLR(DCP_CHAN)) = 0xFFFFFFFFU;
    MXS_DCP_REG(MXS_DCP_STAT_CLR) = stat;
    chSemSignalI(&dcp_active->done);
  }
  return true;    /* handled, keep the IRQ enabled */
}

/* Run one descriptor on channel 0, wait for completion. */
static int dcp_run(MXSDCPDriver *dcp, mxs_dcp_desc_t *desc) {
  unsigned i;

  MXS_DCP_REG(MXS_DCP_CH_STAT_CLR(DCP_CHAN)) = 0xFFFFFFFFU;
  MXS_DCP_REG(MXS_DCP_CH_CMDPTR(DCP_CHAN)) = (uint32_t)desc;
  MXS_DCP_REG(MXS_DCP_CH_SEMA(DCP_CHAN)) = 1U;    /* kick the channel      */

  for (i = 0U; i < DCP_TIMEOUT_MS; i++) {
    if (chSemWaitTimeout(&dcp->done, TIME_MS2I(1)) == MSG_OK) {
      break;
    }
    chThdSleepMilliseconds(1);
  }
  if (i == DCP_TIMEOUT_MS) {
    return MXS_DCP_ERR_TIMEOUT;
  }
  /* CH_STAT bit 31 is the error flag on STMP-style channels. */
  if ((dcp->last_ch_stat & (1U << 31)) != 0U) {
    return MXS_DCP_ERR_HW;
  }
  return MXS_DCP_OK;
}

static bool need_bounce(const void *p) {

  return (((uint32_t)p & 0x3U) != 0U);
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

int mxs_dcp_init(MXSDCPDriver *dcp) {
  unsigned i;

  memset(dcp, 0, sizeof(*dcp));
  chSemObjectInit(&dcp->done, 0);
  dcp_active = dcp;

  /* Ungate and de-reset the DCP block (STMP-style CTRL SFTRST/CLKGATE). */
  MXS_DCP_REG(MXS_DCP_CTRL) = MXS_DCP_CTRL_SFTRST | MXS_DCP_CTRL_CLKGATE;
  MXS_DCP_REG(MXS_DCP_CTRL) = MXS_DCP_CTRL_CLKGATE;
  for (i = 0U; i < 100U; i++) {
    if ((MXS_DCP_REG(MXS_DCP_CTRL) & MXS_DCP_CTRL_SFTRST) == 0U) {
      break;
    }
  }
  MXS_DCP_REG_CLR(MXS_DCP_CTRL, MXS_DCP_CTRL_CLKGATE);

  /* Control: gather residual writes + context caching + channel enables. */
  MXS_DCP_REG(MXS_DCP_CTRL) = MXS_DCP_CTRL_GATHER_RESIDUAL_WRITES |
                               MXS_DCP_CTRL_ENABLE_CONTEXT_CACHING | 0x0FU;
  /* Enable all DCP DMA channels. */
  MXS_DCP_REG(MXS_DCP_CHANNELCTRL) = 0xFFU;
  /* No context switching: point the context buffer at ROM (illegal for
     the DCP DMA) as Linux does. */
  MXS_DCP_REG(MXS_DCP_CONTEXT) = 0xFFFF0000U;
  for (i = 0U; i < 4U; i++) {
    MXS_DCP_REG(MXS_DCP_CH_STAT_CLR(i)) = 0xFFFFFFFFU;
  }
  MXS_DCP_REG(MXS_DCP_STAT_CLR) = 0xFFFFFFFFU;

#if defined(MXS_DCP_HAVE_IMX23_PORT)
  icoll_register_handler(IMX23_IRQ_DCP0, dcp_irq_handler);
  icoll_irq_set_priority(IMX23_IRQ_DCP0, 4U);
  icoll_irq_enable(IMX23_IRQ_DCP0);
#else
  /* Non-i.MX23 MXS port: wire the DCP IRQ to dcp_irq_handler() yourself. */
#endif

  return MXS_DCP_OK;
}

int mxs_dcp_sha256(MXSDCPDriver *dcp, const uint8_t *in, size_t len,
                     uint8_t out[MXS_DCP_SHA256_LEN]) {
  mxs_dcp_desc_t *desc = &dcp->desc;
  const uint8_t *src = in;
  int rc;

  if (need_bounce(in) || need_bounce(out)) {
    if (len > sizeof(dcp->bounce)) {
      return MXS_DCP_ERR_SIZE;
    }
    memcpy(dcp->bounce, in, len);
    src = dcp->bounce;
  }

  memset(desc, 0, sizeof(*desc));
  desc->control0 = MXS_DCP_C0_DECR_SEMAPHORE | MXS_DCP_C0_INTERRUPT |
                    MXS_DCP_C0_ENABLE_HASH | MXS_DCP_C0_HASH_INIT |
                    MXS_DCP_C0_HASH_TERM;
  desc->control1 = MXS_DCP_C1_HASH_SELECT_SHA256;
  desc->source      = (uint32_t)src;
  desc->destination = 0U;
  desc->size        = (uint32_t)len;
  desc->payload     = (uint32_t)out;   /* digest lands here on HASH_TERM */

  rc = dcp_run(dcp, desc);
  return rc;
}

int mxs_dcp_aes128(MXSDCPDriver *dcp, const uint8_t *in, uint8_t *out,
                     size_t len, const uint8_t *key, uint8_t key_select,
                     bool encrypt, bool cbc, const uint8_t *iv) {
  mxs_dcp_desc_t *desc = &dcp->desc;
  const uint8_t *src = in;
  uint8_t *dst = out;
  int rc;

  if ((len % MXS_DCP_AES_BLOCK) != 0U) {
    return MXS_DCP_ERR_SIZE;
  }
  if (cbc && iv == NULL) {
    return MXS_DCP_ERR_ALIGN;
  }

  /* Bounce unaligned src/dst through the aligned chunk buffer. */
  if (need_bounce(in) || need_bounce(out)) {
    if (len > sizeof(dcp->bounce)) {
      return MXS_DCP_ERR_SIZE;
    }
    memcpy(dcp->bounce, in, len);
    src = dcp->bounce;
    dst = dcp->bounce;   /* in-place style: desc src=dst is not allowed,
                            so use a second pass below */
  }

  /* Payload: explicit key -> key||IV (32 bytes); slot/OTP/unique -> IV only
     in the second half (first half ignored). */
  memset(dcp->keybuf, 0, sizeof(dcp->keybuf));
  if (key != NULL) {
    memcpy(dcp->keybuf, key, 16U);
  }
  if (cbc && iv != NULL) {
    memcpy(dcp->keybuf + 16U, iv, 16U);
  }

  memset(desc, 0, sizeof(*desc));
  desc->control0 = MXS_DCP_C0_DECR_SEMAPHORE | MXS_DCP_C0_INTERRUPT |
                    MXS_DCP_C0_ENABLE_CIPHER | MXS_DCP_C0_CIPHER_INIT;
  if (encrypt) {
    desc->control0 |= MXS_DCP_C0_CIPHER_ENCRYPT;
  }
  if (key != NULL) {
    desc->control0 |= MXS_DCP_C0_PAYLOAD_KEY;
  }
  else if (key_select == MXS_DCP_KEY_OTP) {
    desc->control0 |= MXS_DCP_C0_OTP_KEY;
  }
  desc->control1 = MXS_DCP_C1_CIPHER_SELECT_AES128 |
                    (cbc ? MXS_DCP_C1_CIPHER_MODE_CBC
                         : MXS_DCP_C1_CIPHER_MODE_ECB) |
                    ((uint32_t)key_select << MXS_DCP_C1_KEY_SELECT_SHIFT);
  desc->source      = (uint32_t)src;
  desc->destination = (uint32_t)dst;
  desc->size        = (uint32_t)len;
  desc->payload     = (uint32_t)dcp->keybuf;

  if (src == dst) {
    /* Bounced case: run in place is fine (DCP reads then writes), but we
       bounced into one buffer, so src/dst already alias. */
  }
  rc = dcp_run(dcp, desc);
  if (rc == MXS_DCP_OK && dst == dcp->bounce && out != dcp->bounce) {
    memcpy(out, dcp->bounce, len);
  }
  return rc;
}
