/*
    ChibiOS - Copyright (C) 2026 Xael South

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
 * @file    hal_crypto_lld.c
 * @brief   EFR32FG23 RADIOAES crypto low level driver source.
 *
 * @details Raw-register, polled driver for the FG23 RADIOAES engine
 *          (BA411E AES core behind a fetcher/pusher scatter-gather DMA
 *          interface). The engine is programmed through descriptor
 *          chains in RAM:
 *
 *            fetcher: key -> config -> [IV] -> payload
 *            pusher:  output -> [context]
 *
 *          Descriptor/tag layout follows the Silicon Labs RADIOAES
 *          programming model (see the Series-2 reference manual and
 *          sl_protocol_crypto). AES-128/192/256 in ECB, CBC and CTR mode
 *          is supported; key, IV and payload are passed in natural byte
 *          order (no reversal).
 *
 * @addtogroup CRYPTO
 * @{
 */

#include <string.h>

#include "hal.h"

#if (HAL_USE_CRY == TRUE) && (EFR32_CRYPTO_USE_AES1 == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* RADIOAES DMA scatter-gather descriptor (must live in RAM). */
typedef struct {
  volatile uint32_t address;
  volatile uint32_t nextDescr;
  volatile uint32_t lengthAndIrq;
  volatile uint32_t tag;
} aes_dma_descr_t;

/* Descriptor nextDescr stop value. */
#define AES_DESCR_NEXT_STOP         0x00000001UL

/* lengthAndIrq address-mode bits. */
#define AES_BLOCK_INCR_ADDR         0x00000000UL
#define AES_BLOCK_CONST_ADDR        0x10000000UL
#define AES_BLOCK_ADDR_MODE_MASK    0x70000000UL

/* Descriptor tag bits. */
#define AES_TAG_ENGINE_BA411E       0x00000001UL  /* data flows through AES  */
#define AES_TAG_ISCONFIG            0x00000010UL
#define AES_TAG_ISLAST              0x00000020UL
#define AES_TAG_DATATYPE_HEADER     0x00000040UL
#define AES_TAG_DATATYPE_PAYLOAD    0x00000000UL
#define AES_TAG_SETCFGOFFSET(a)     (((uint32_t)(a) & 0xFFUL) << 8)
#define AES_TAG_SETINVALIDBYTES(a)  (((uint32_t)(a) & 0x1FUL) << 8)

/* BA411E configuration word offsets (in bytes inside its context RAM). */
#define AES_OFFSET_CFG              0
#define AES_OFFSET_KEY              8
#define AES_OFFSET_IV               40

/* BA411E mode register values. */
#define AES_MODE_ECB                0x00000100UL
#define AES_MODE_CBC                0x00000200UL
#define AES_MODE_CTR                0x00000400UL
#define AES_MODE_CX_LOAD            0x00000010UL
#define AES_MODE_CX_SAVE            0x00000020UL
#define AES_MODE_NO_CX              0x00000000UL
#define AES_MODE_KEY128             0x00000000UL
#define AES_MODE_KEY256             0x00000004UL
#define AES_MODE_KEY192             0x00000008UL
#define AES_MODE_ENCRYPT            0x00000000UL
#define AES_MODE_DECRYPT            0x00000001UL

#define AES_BLOCK_BYTES             16U

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief CRYD1 driver identifier. */
CRYDriver CRYD1;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/* Key size in bytes -> BA411E mode bits. */
static uint32_t aes_keysize_mode(size_t size) {
  switch (size) {
    case 16: return AES_MODE_KEY128;
    case 24: return AES_MODE_KEY192;
    case 32: return AES_MODE_KEY256;
    default: return 0xFFFFFFFFUL;
  }
}

/*
 * Run one RADIOAES operation. The fetcher chain pulls key/config/IV/payload
 * from memory, the pusher chain writes the result. Completion is detected
 * by busy-polling (default) or via the AES interrupt + semaphore when
 * EFR32_CRYPTO_AES_IRQ_MODE is TRUE (bulk operations, thread sleeps).
 */
static cryerror_t aes_run(aes_dma_descr_t *fetch_first,
                          aes_dma_descr_t *push_first) {

  RADIOAES->CTRL = AES_CTRL_FETCHERSCATTERGATHER | AES_CTRL_PUSHERSCATTERGATHER;
  RADIOAES->FETCHADDR = (uint32_t)fetch_first;
  RADIOAES->PUSHADDR  = (uint32_t)push_first;
#if EFR32_CRYPTO_AES_IRQ_MODE == TRUE
  CRYD1.aes_error = false;
  RADIOAES->IF_CLR = 0xFFFFFFFFUL;
  RADIOAES->IEN = AES_IEN_FETCHERENDOFBLOCK | AES_IEN_PUSHERENDOFBLOCK |
                   AES_IEN_FETCHERERROR | AES_IEN_PUSHERERROR;
#endif
  RADIOAES->CMD = AES_CMD_STARTPUSHER | AES_CMD_STARTFETCHER;

#if EFR32_CRYPTO_AES_IRQ_MODE == TRUE
  /* Sleep until the ISR reports completion or error. */
  chSemWait(&CRYD1.aes_sem);
  RADIOAES->IEN = 0U;
  if (CRYD1.aes_error) {
    return CRY_ERR_OP_FAILURE;
  }
#else
  while ((RADIOAES->STATUS & (AES_STATUS_FETCHERBSY | AES_STATUS_PUSHERBSY)) != 0U) {
  }
#endif

  return CRY_NOERROR;
}

#if EFR32_CRYPTO_AES_IRQ_MODE == TRUE
/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

OSAL_IRQ_HANDLER(EFR32_AES_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  uint32_t iflags = RADIOAES->IF;
  RADIOAES->IF_CLR = iflags;

  if ((iflags & (AES_IF_FETCHERERROR | AES_IF_PUSHERERROR)) != 0U) {
    CRYD1.aes_error = true;
    osalSysLockFromISR();
    chSemSignalI(&CRYD1.aes_sem);
    osalSysUnlockFromISR();
  } else if (((iflags & (AES_IF_FETCHERENDOFBLOCK | AES_IF_PUSHERENDOFBLOCK)) ==
              (AES_IF_FETCHERENDOFBLOCK | AES_IF_PUSHERENDOFBLOCK)) &&
             ((RADIOAES->STATUS & (AES_STATUS_FETCHERBSY | AES_STATUS_PUSHERBSY)) == 0U)) {
    /* Both engines finished the whole chain (bulk-safe: ENDOFBLOCK may
       fire per block, only the final one has both busy bits clear). */
    osalSysLockFromISR();
    chSemSignalI(&CRYD1.aes_sem);
    osalSysUnlockFromISR();
  }

  OSAL_IRQ_EPILOGUE();
}
#endif

/*
 * Build the common fetcher prefix (key + config) and run a mode operation.
 * mode_reg is the BA411E mode word without key size / direction.
 * iv may be NULL (ECB). For CBC/CTR the IV is fed as a config block.
 */
static cryerror_t aes_mode_op(CRYDriver *cryp,
                              uint32_t mode_reg,
                              size_t size,
                              const uint8_t *in,
                              uint8_t *out,
                              const uint8_t *iv) {
  uint32_t config;
  uint32_t keysize_mode;
  uint32_t invalid = 0U;
  aes_dma_descr_t fetch_key, fetch_config, fetch_iv, fetch_data;
  aes_dma_descr_t push_data;

  keysize_mode = aes_keysize_mode(cryp->aes_keysize);
  if (keysize_mode == 0xFFFFFFFFUL) {
    return CRY_ERR_INV_KEY_SIZE;
  }
  config = mode_reg | keysize_mode;

  /* CTR allows a partial final block; mark the trailing bytes invalid. */
  if ((mode_reg & 0x00000F00UL) == AES_MODE_CTR) {
    size_t rem = size % AES_BLOCK_BYTES;
    if (rem != 0U) {
      invalid = AES_BLOCK_BYTES - rem;
    }
  } else {
    /* ECB/CBC require whole blocks. */
    if ((size % AES_BLOCK_BYTES) != 0U || size == 0U) {
      return CRY_ERR_INV_ALGO;
    }
  }

  /* Pusher: result payload out. */
  push_data.address      = (uint32_t)out;
  push_data.nextDescr    = AES_DESCR_NEXT_STOP;
  push_data.lengthAndIrq = (uint32_t)size | AES_BLOCK_INCR_ADDR;
  push_data.tag          = AES_TAG_ENGINE_BA411E | AES_TAG_ISLAST;

  /* Fetcher: payload (data) last. */
  fetch_data.address      = (uint32_t)in;
  fetch_data.nextDescr    = AES_DESCR_NEXT_STOP;
  fetch_data.lengthAndIrq = (uint32_t)size | AES_BLOCK_INCR_ADDR;
  fetch_data.tag          = AES_TAG_ENGINE_BA411E | AES_TAG_ISLAST
                          | AES_TAG_DATATYPE_PAYLOAD | AES_TAG_SETINVALIDBYTES(invalid);

  if (iv != NULL) {
    /* Fetcher: IV/context block (CBC, CTR). */
    fetch_iv.address      = (uint32_t)iv;
    fetch_iv.nextDescr    = (uint32_t)&fetch_data;
    fetch_iv.lengthAndIrq = AES_BLOCK_BYTES | AES_BLOCK_INCR_ADDR;
    fetch_iv.tag          = AES_TAG_ENGINE_BA411E | AES_TAG_ISCONFIG
                          | AES_TAG_SETCFGOFFSET(AES_OFFSET_IV);

    fetch_config.nextDescr = (uint32_t)&fetch_iv;
  } else {
    fetch_config.nextDescr = (uint32_t)&fetch_data;
  }

  /* Fetcher: configuration word. */
  fetch_config.address      = (uint32_t)&config;
  fetch_config.lengthAndIrq = sizeof(config);
  fetch_config.tag          = AES_TAG_ENGINE_BA411E | AES_TAG_ISCONFIG
                            | AES_TAG_SETCFGOFFSET(AES_OFFSET_CFG);

  /* Fetcher: key first. */
  fetch_key.address      = (uint32_t)cryp->aes_key;
  fetch_key.nextDescr    = (uint32_t)&fetch_config;
  fetch_key.lengthAndIrq = (uint32_t)cryp->aes_keysize | AES_BLOCK_INCR_ADDR;
  fetch_key.tag          = AES_TAG_ENGINE_BA411E | AES_TAG_ISCONFIG
                         | AES_TAG_SETCFGOFFSET(AES_OFFSET_KEY);

  return aes_run(&fetch_key, &push_data);
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void cry_lld_init(void) {
  cryObjectInit(&CRYD1);
  CRYD1.aes_keysize = 0U;
#if EFR32_CRYPTO_AES_IRQ_MODE == TRUE
  chSemObjectInit(&CRYD1.aes_sem, 0);
  CRYD1.aes_error = false;
#endif
}

void cry_lld_start(CRYDriver *cryp) {
  (void)cryp;

  /* Enable the RADIOAES bus clock and reset the engine. */
  CMU->CLKEN0_SET = CMU_CLKEN0_RADIOAES;
  RADIOAES->CTRL = AES_CTRL_SWRESET;
  while ((RADIOAES->STATUS & AES_STATUS_SOFTRSTBSY) != 0U) {
  }
#if EFR32_CRYPTO_AES_IRQ_MODE == TRUE
  RADIOAES->IF_CLR = 0xFFFFFFFFUL;
  RADIOAES->IEN = 0U;
  nvicEnableVector(EFR32_AES_NUMBER, EFR32_AES_IRQ_PRIORITY);
#endif
}

void cry_lld_stop(CRYDriver *cryp) {
  (void)cryp;
#if EFR32_CRYPTO_AES_IRQ_MODE == TRUE
  nvicDisableVector(EFR32_AES_NUMBER);
  RADIOAES->IEN = 0U;
#endif
  RADIOAES->CTRL = AES_CTRL_SWRESET;
  while ((RADIOAES->STATUS & AES_STATUS_SOFTRSTBSY) != 0U) {
  }
  CMU->CLKEN0_CLR = CMU_CLKEN0_RADIOAES;
}

cryerror_t cry_lld_aes_loadkey(CRYDriver *cryp,
                               size_t size,
                               const uint8_t *keyp) {

  if ((size != 16U) && (size != 24U) && (size != 32U)) {
    return CRY_ERR_INV_KEY_SIZE;
  }
  memcpy(cryp->aes_key, keyp, size);
  cryp->aes_keysize = size;
  return CRY_NOERROR;
}

cryerror_t cry_lld_encrypt_AES(CRYDriver *cryp,
                               crykey_t key_id,
                               const uint8_t *in,
                               uint8_t *out) {
  if (key_id != 0U) {
    return CRY_ERR_INV_KEY_ID;
  }
  return aes_mode_op(cryp, AES_MODE_ECB | AES_MODE_NO_CX | AES_MODE_ENCRYPT,
                     AES_BLOCK_BYTES, in, out, NULL);
}

cryerror_t cry_lld_decrypt_AES(CRYDriver *cryp,
                               crykey_t key_id,
                               const uint8_t *in,
                               uint8_t *out) {
  if (key_id != 0U) {
    return CRY_ERR_INV_KEY_ID;
  }
  return aes_mode_op(cryp, AES_MODE_ECB | AES_MODE_NO_CX | AES_MODE_DECRYPT,
                     AES_BLOCK_BYTES, in, out, NULL);
}

cryerror_t cry_lld_encrypt_AES_ECB(CRYDriver *cryp,
                                   crykey_t key_id,
                                   size_t size,
                                   const uint8_t *in,
                                   uint8_t *out) {
  if (key_id != 0U) {
    return CRY_ERR_INV_KEY_ID;
  }
  return aes_mode_op(cryp, AES_MODE_ECB | AES_MODE_NO_CX | AES_MODE_ENCRYPT,
                     size, in, out, NULL);
}

cryerror_t cry_lld_decrypt_AES_ECB(CRYDriver *cryp,
                                   crykey_t key_id,
                                   size_t size,
                                   const uint8_t *in,
                                   uint8_t *out) {
  if (key_id != 0U) {
    return CRY_ERR_INV_KEY_ID;
  }
  return aes_mode_op(cryp, AES_MODE_ECB | AES_MODE_NO_CX | AES_MODE_DECRYPT,
                     size, in, out, NULL);
}

cryerror_t cry_lld_encrypt_AES_CBC(CRYDriver *cryp,
                                   crykey_t key_id,
                                   size_t size,
                                   const uint8_t *in,
                                   uint8_t *out,
                                   const uint8_t *iv) {
  if (key_id != 0U) {
    return CRY_ERR_INV_KEY_ID;
  }
  return aes_mode_op(cryp, AES_MODE_CBC | AES_MODE_CX_LOAD | AES_MODE_ENCRYPT,
                     size, in, out, iv);
}

cryerror_t cry_lld_decrypt_AES_CBC(CRYDriver *cryp,
                                   crykey_t key_id,
                                   size_t size,
                                   const uint8_t *in,
                                   uint8_t *out,
                                   const uint8_t *iv) {
  if (key_id != 0U) {
    return CRY_ERR_INV_KEY_ID;
  }
  return aes_mode_op(cryp, AES_MODE_CBC | AES_MODE_CX_LOAD | AES_MODE_DECRYPT,
                     size, in, out, iv);
}

cryerror_t cry_lld_encrypt_AES_CTR(CRYDriver *cryp,
                                   crykey_t key_id,
                                   size_t size,
                                   const uint8_t *in,
                                   uint8_t *out,
                                   const uint8_t *iv) {
  if (key_id != 0U) {
    return CRY_ERR_INV_KEY_ID;
  }
  return aes_mode_op(cryp, AES_MODE_CTR | AES_MODE_CX_LOAD | AES_MODE_ENCRYPT,
                     size, in, out, iv);
}

cryerror_t cry_lld_decrypt_AES_CTR(CRYDriver *cryp,
                                   crykey_t key_id,
                                   size_t size,
                                   const uint8_t *in,
                                   uint8_t *out,
                                   const uint8_t *iv) {
  if (key_id != 0U) {
    return CRY_ERR_INV_KEY_ID;
  }
  /* CTR is symmetric: encryption == decryption. */
  return aes_mode_op(cryp, AES_MODE_CTR | AES_MODE_CX_LOAD | AES_MODE_ENCRYPT,
                     size, in, out, iv);
}

#endif /* HAL_USE_CRY && EFR32_CRYPTO_USE_AES1 */

/** @} */
