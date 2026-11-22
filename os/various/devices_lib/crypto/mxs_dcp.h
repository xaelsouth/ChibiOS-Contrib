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
 * @file    devices_lib/crypto/mxs_dcp.h
 * @brief   MXS-family DCP (Data Co-Path) hardware crypto engine driver
 *          (i.MX23 / i.MX28 / STMP37xx).
 *
 * @details The DCP provides AES-128 (ECB/CBC) and SHA-1/SHA-256 in hardware,
 *          with key material either supplied in the DMA descriptor payload
 *          or taken from the on-chip key slots: slot 0..3 (loaded via the
 *          DCP_KEY register), the device-unique key (0xFE, derived from the
 *          OTP fuses) or the raw OTP key (0xFF).
 *
 *          Typical secure scheme: a firmware image is distributed encrypted
 *          with an all-zero key; on the device it is decrypted with the zero
 *          key and re-encrypted with the device-unique DCP key
 *          (mxs_dcp_aes128() with MXS_DCP_KEY_UNIQUE), so the copy at
 *          rest is bound to this one chip.
 *
 *          The DCP is a single hardware block: one driver object, channel 0,
 *          MMU is off so descriptors/buffers live in RAM and are passed by
 *          physical (= virtual) address.
 */

#ifndef MXS_DCP_H
#define MXS_DCP_H

#include "hal.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/* DCP register block (STMP37xx/i.MX23/i.MX28 "MXS" family, AHB APBH). The
   driver is self-contained: absolute addresses, no port header needed. */
#define MXS_DCP_BASE            0x80028000U
#define MXS_DCP_REG(addr)       (*(volatile uint32_t *)(uintptr_t)(addr))
#define MXS_DCP_REG_CLR(addr, mask)                                             \
  (MXS_DCP_REG(addr) = MXS_DCP_REG(addr) & ~(uint32_t)(mask))

#define MXS_DCP_CTRL            (MXS_DCP_BASE + 0x000U)
#define MXS_DCP_CTRL_SFTRST     (1U << 31)
#define MXS_DCP_CTRL_CLKGATE    (1U << 30)
#define MXS_DCP_CTRL_GATHER_RESIDUAL_WRITES   (1U << 23)
#define MXS_DCP_CTRL_ENABLE_CONTEXT_CACHING   (1U << 22)
#define MXS_DCP_STAT            (MXS_DCP_BASE + 0x010U)
#define MXS_DCP_STAT_CLR        (MXS_DCP_BASE + 0x018U)
#define MXS_DCP_CHANNELCTRL     (MXS_DCP_BASE + 0x020U)
#define MXS_DCP_KEY             (MXS_DCP_BASE + 0x030U)
#define MXS_DCP_CAPABILITY1     (MXS_DCP_BASE + 0x040U)
#define MXS_DCP_CONTEXT         (MXS_DCP_BASE + 0x050U)
#define MXS_DCP_CH_CMDPTR(n)    (MXS_DCP_BASE + 0x100U + (n) * 0x40U)
#define MXS_DCP_CH_SEMA(n)      (MXS_DCP_BASE + 0x110U + (n) * 0x40U)
#define MXS_DCP_CH_STAT(n)      (MXS_DCP_BASE + 0x120U + (n) * 0x40U)
#define MXS_DCP_CH_STAT_CLR(n)  (MXS_DCP_BASE + 0x128U + (n) * 0x40U)

/* DCP DMA descriptor control0 bits. */
#define MXS_DCP_C0_INTERRUPT          (1U << 0)
#define MXS_DCP_C0_DECR_SEMAPHORE     (1U << 1)
#define MXS_DCP_C0_ENABLE_CIPHER      (1U << 5)
#define MXS_DCP_C0_ENABLE_HASH        (1U << 6)
#define MXS_DCP_C0_CIPHER_ENCRYPT     (1U << 8)
#define MXS_DCP_C0_CIPHER_INIT        (1U << 9)
#define MXS_DCP_C0_OTP_KEY            (1U << 10)
#define MXS_DCP_C0_PAYLOAD_KEY        (1U << 11)
#define MXS_DCP_C0_HASH_INIT          (1U << 12)
#define MXS_DCP_C0_HASH_TERM          (1U << 13)

/* DCP DMA descriptor control1 bits. */
#define MXS_DCP_C1_CIPHER_SELECT_AES128   (0U << 0)
#define MXS_DCP_C1_CIPHER_MODE_ECB        (0U << 4)
#define MXS_DCP_C1_CIPHER_MODE_CBC        (1U << 4)
#define MXS_DCP_C1_KEY_SELECT_SHIFT       8
#define MXS_DCP_C1_HASH_SELECT_SHA1       (0U << 16)
#define MXS_DCP_C1_HASH_SELECT_SHA256     (2U << 16)

/* Key select values for control1 KEY_SELECT (and the OTP_KEY bit). */
#define MXS_DCP_KEY_SLOT(n)     (n)         /* slots 0..3, key via DCP_KEY  */
#define MXS_DCP_KEY_UNIQUE      0xFEU       /* device-unique key (from OTP) */
#define MXS_DCP_KEY_OTP         0xFFU       /* raw OTP key, needs C0_OTP_KEY */

#define MXS_DCP_OK            0
#define MXS_DCP_ERR_TIMEOUT   (-1)
#define MXS_DCP_ERR_HW        (-2)
#define MXS_DCP_ERR_ALIGN     (-3)
#define MXS_DCP_ERR_SIZE      (-4)

#define MXS_DCP_AES_BLOCK     16U
#define MXS_DCP_SHA256_LEN    32U
#define MXS_DCP_SHA1_LEN      20U

/* Max bounce chunk when an unaligned buffer is passed (DCP wants aligned). */
#define MXS_DCP_CHUNK         4096U

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/* DCP DMA descriptor (8 words, 64-byte aligned as the Linux driver does). */
typedef struct {
  uint32_t next_cmd_addr;
  uint32_t control0;
  uint32_t control1;
  uint32_t source;
  uint32_t destination;
  uint32_t size;
  uint32_t payload;
  uint32_t status;
} mxs_dcp_desc_t;

typedef struct {
  /* Driver state, all instance state lives here (single HW block). */
  semaphore_t       done;         /* signalled by the channel-0 IRQ       */
  mxs_dcp_desc_t  desc __attribute__((aligned(64)));
  uint8_t           bounce[MXS_DCP_CHUNK] __attribute__((aligned(64)));
  uint8_t           keybuf[32U] __attribute__((aligned(16)));
  uint32_t          last_ch_stat;
} MXSDCPDriver;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  extern MXSDCPDriver MXSDCP1;

  int  mxs_dcp_init(MXSDCPDriver *dcp);
  int  mxs_dcp_sha256(MXSDCPDriver *dcp, const uint8_t *in, size_t len,
                        uint8_t out[MXS_DCP_SHA256_LEN]);
  /* AES-128, ECB or CBC, encrypt or decrypt.
     key == NULL        -> use key_select (MXS_DCP_KEY_SLOT/UNIQUE/OTP)
     key != NULL        -> 16-byte explicit key (payload key)
     iv  == NULL for ECB; 16-byte IV for CBC.
     len must be a multiple of 16. */
  int  mxs_dcp_aes128(MXSDCPDriver *dcp, const uint8_t *in, uint8_t *out,
                        size_t len, const uint8_t *key, uint8_t key_select,
                        bool encrypt, bool cbc, const uint8_t *iv);
#ifdef __cplusplus
}
#endif

#endif /* MXS_DCP_H */
