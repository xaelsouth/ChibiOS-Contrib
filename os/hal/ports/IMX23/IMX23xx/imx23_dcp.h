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
 * @file    imx23_dcp.h
 * @brief   i.MX23 DCP (Data Co-Path) hardware crypto engine driver.
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
 *          (imx23_dcp_aes128() with IMX23_DCP_KEY_UNIQUE), so the copy at
 *          rest is bound to this one chip.
 *
 *          The DCP is a single hardware block: one driver object, channel 0,
 *          MMU is off so descriptors/buffers live in RAM and are passed by
 *          physical (= virtual) address.
 */

#ifndef IMX23_DCP_H
#define IMX23_DCP_H

#include "imx23.h"

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

#define IMX23_DCP_OK            0
#define IMX23_DCP_ERR_TIMEOUT   (-1)
#define IMX23_DCP_ERR_HW        (-2)
#define IMX23_DCP_ERR_ALIGN     (-3)
#define IMX23_DCP_ERR_SIZE      (-4)

#define IMX23_DCP_AES_BLOCK     16U
#define IMX23_DCP_SHA256_LEN    32U
#define IMX23_DCP_SHA1_LEN      20U

/* Max bounce chunk when an unaligned buffer is passed (DCP wants aligned). */
#define IMX23_DCP_CHUNK         4096U

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
} imx23_dcp_desc_t;

typedef struct {
  /* Driver state, all instance state lives here (single HW block). */
  semaphore_t       done;         /* signalled by the channel-0 IRQ       */
  imx23_dcp_desc_t  desc __attribute__((aligned(64)));
  uint8_t           bounce[IMX23_DCP_CHUNK] __attribute__((aligned(64)));
  uint8_t           keybuf[32U] __attribute__((aligned(16)));
  uint32_t          last_ch_stat;
} IMX23DCPDriver;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  extern IMX23DCPDriver IMX23DCP1;

  int  imx23_dcp_init(IMX23DCPDriver *dcp);
  int  imx23_dcp_sha256(IMX23DCPDriver *dcp, const uint8_t *in, size_t len,
                        uint8_t out[IMX23_DCP_SHA256_LEN]);
  /* AES-128, ECB or CBC, encrypt or decrypt.
     key == NULL        -> use key_select (IMX23_DCP_KEY_SLOT/UNIQUE/OTP)
     key != NULL        -> 16-byte explicit key (payload key)
     iv  == NULL for ECB; 16-byte IV for CBC.
     len must be a multiple of 16. */
  int  imx23_dcp_aes128(IMX23DCPDriver *dcp, const uint8_t *in, uint8_t *out,
                        size_t len, const uint8_t *key, uint8_t key_select,
                        bool encrypt, bool cbc, const uint8_t *iv);
#ifdef __cplusplus
}
#endif

#endif /* IMX23_DCP_H */
