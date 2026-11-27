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
 * @file    hal_crypto_lld.h
 * @brief   EFR32FG23 RADIOAES crypto low level driver header.
 *
 * @details Implements the ChibiOS CRY LLD on top of the FG23 RADIOAES
 *          engine (BA411E AES core, fetcher/pusher scatter-gather
 *          interface, polled). Supports AES-128/192/256 in ECB, CBC and
 *          CTR mode plus single-block AES. Enable with
 *          EFR32_CRYPTO_USE_AES1=TRUE in mcuconf.h and HAL_USE_CRY=TRUE
 *          in halconf.h.
 *
 * @addtogroup CRYPTO
 * @{
 */

#ifndef HAL_CRYPTO_LLD_H
#define HAL_CRYPTO_LLD_H

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   AES1 (RADIOAES) driver enable switch.
 * @details If set to @p TRUE the support for the RADIOAES-based crypto
 *          driver is included.
 * @note    The default is @p FALSE.
 */
#if !defined(EFR32_CRYPTO_USE_AES1) || defined(__DOXYGEN__)
#define EFR32_CRYPTO_USE_AES1               FALSE
#endif

/**
 * @brief   RADIOAES completion mode.
 * @details If @p TRUE the driver waits for operation completion via the
 *          AES interrupt (semaphore), so the calling thread sleeps during
 *          bulk operations. If @p FALSE (default) completion is detected
 *          by busy-polling the STATUS busy bits, which is cheaper for
 *          single blocks.
 * @note    The default is @p FALSE.
 */
#if !defined(EFR32_CRYPTO_AES_IRQ_MODE) || defined(__DOXYGEN__)
#define EFR32_CRYPTO_AES_IRQ_MODE           FALSE
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if (EFR32_CRYPTO_USE_AES1 == TRUE) && (HAL_USE_CRY == TRUE) || defined(__DOXYGEN__)

#define PLATFORM_CRY_USE_CRY1               TRUE

#define CRY_LLD_SUPPORTS_AES                TRUE
/* RADIOAES uses a DMA fetcher/pusher even when completion is polled. */
#define CRY_LLD_SUPPORTS_AES_X              FALSE
#define CRY_LLD_SUPPORTS_AES_ECB            TRUE
#define CRY_LLD_SUPPORTS_AES_CBC            TRUE
#define CRY_LLD_SUPPORTS_AES_CFB            FALSE
#define CRY_LLD_SUPPORTS_AES_CTR            TRUE
#define CRY_LLD_SUPPORTS_AES_GCM            FALSE
#define CRY_LLD_SUPPORTS_DES                FALSE
#define CRY_LLD_SUPPORTS_DES_X              FALSE
#define CRY_LLD_SUPPORTS_DES_ECB            FALSE
#define CRY_LLD_SUPPORTS_DES_CBC            FALSE
#define CRY_LLD_SUPPORTS_SHA1               FALSE
#define CRY_LLD_SUPPORTS_SHA256             FALSE
#define CRY_LLD_SUPPORTS_SHA512             FALSE
#define CRY_LLD_SUPPORTS_HMAC_SHA256        FALSE
#define CRY_LLD_SUPPORTS_HMAC_SHA512        FALSE

#else

#define PLATFORM_CRY_USE_CRY1               FALSE

#define CRY_LLD_SUPPORTS_AES                FALSE
#define CRY_LLD_SUPPORTS_AES_X              FALSE
#define CRY_LLD_SUPPORTS_AES_ECB            FALSE
#define CRY_LLD_SUPPORTS_AES_CBC            FALSE
#define CRY_LLD_SUPPORTS_AES_CFB            FALSE
#define CRY_LLD_SUPPORTS_AES_CTR            FALSE
#define CRY_LLD_SUPPORTS_AES_GCM            FALSE
#define CRY_LLD_SUPPORTS_DES                FALSE
#define CRY_LLD_SUPPORTS_DES_X              FALSE
#define CRY_LLD_SUPPORTS_DES_ECB            FALSE
#define CRY_LLD_SUPPORTS_DES_CBC            FALSE
#define CRY_LLD_SUPPORTS_SHA1               FALSE
#define CRY_LLD_SUPPORTS_SHA256             FALSE
#define CRY_LLD_SUPPORTS_SHA512             FALSE
#define CRY_LLD_SUPPORTS_HMAC_SHA256        FALSE
#define CRY_LLD_SUPPORTS_HMAC_SHA512        FALSE

#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   CRY key identifier type.
 */
typedef uint32_t crykey_t;

/**
 * @brief   Type of a structure representing a CRY driver.
 */
typedef struct CRYDriver CRYDriver;

/**
 * @brief   Driver configuration structure.
 * @note    It could be empty on some architectures.
 */
typedef struct {
  uint32_t                  dummy;
} CRYConfig;

/**
 * @brief   Structure representing a CRY driver.
 */
struct CRYDriver {
  /**
   * @brief   Driver state.
   */
  crystate_t                state;
  /**
   * @brief   Current configuration data.
   */
  const CRYConfig           *config;
#if (CRY_USE_MUTUAL_EXCLUSION == TRUE) || defined(__DOXYGEN__)
  /**
   * @brief   Mutex protecting the peripheral.
   */
  mutex_t                   mutex;
#endif
  /* End of the mandatory fields.*/
  /**
   * @brief   Transient AES key data (128/192/256 bit).
   */
  uint8_t                   aes_key[32];
  /**
   * @brief   Transient AES key size in bytes (16, 24 or 32).
   */
  size_t                    aes_keysize;
#if (EFR32_CRYPTO_AES_IRQ_MODE == TRUE) || defined(__DOXYGEN__)
  /**
   * @brief   Posted by the AES ISR when an operation completes.
   */
  semaphore_t               aes_sem;
  /**
   * @brief   Set by the AES ISR on a fetcher/pusher error.
   */
  bool                      aes_error;
#endif
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if (PLATFORM_CRY_USE_CRY1 == TRUE) && !defined(__DOXYGEN__)
extern CRYDriver CRYD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void cry_lld_init(void);
  void cry_lld_start(CRYDriver *cryp);
  void cry_lld_stop(CRYDriver *cryp);
#if (CRY_LLD_SUPPORTS_AES == TRUE) || defined(__DOXYGEN__)
  cryerror_t cry_lld_aes_loadkey(CRYDriver *cryp,
                                 size_t size,
                                 const uint8_t *keyp);
  cryerror_t cry_lld_encrypt_AES(CRYDriver *cryp,
                                 crykey_t key_id,
                                 const uint8_t *in,
                                 uint8_t *out);
  cryerror_t cry_lld_decrypt_AES(CRYDriver *cryp,
                                 crykey_t key_id,
                                 const uint8_t *in,
                                 uint8_t *out);
#endif
#if (CRY_LLD_SUPPORTS_AES_ECB == TRUE) || defined(__DOXYGEN__)
  cryerror_t cry_lld_encrypt_AES_ECB(CRYDriver *cryp,
                                     crykey_t key_id,
                                     size_t size,
                                     const uint8_t *in,
                                     uint8_t *out);
  cryerror_t cry_lld_decrypt_AES_ECB(CRYDriver *cryp,
                                     crykey_t key_id,
                                     size_t size,
                                     const uint8_t *in,
                                     uint8_t *out);
#endif
#if (CRY_LLD_SUPPORTS_AES_CBC == TRUE) || defined(__DOXYGEN__)
  cryerror_t cry_lld_encrypt_AES_CBC(CRYDriver *cryp,
                                     crykey_t key_id,
                                     size_t size,
                                     const uint8_t *in,
                                     uint8_t *out,
                                     const uint8_t *iv);
  cryerror_t cry_lld_decrypt_AES_CBC(CRYDriver *cryp,
                                     crykey_t key_id,
                                     size_t size,
                                     const uint8_t *in,
                                     uint8_t *out,
                                     const uint8_t *iv);
#endif
#if (CRY_LLD_SUPPORTS_AES_CTR == TRUE) || defined(__DOXYGEN__)
  cryerror_t cry_lld_encrypt_AES_CTR(CRYDriver *cryp,
                                     crykey_t key_id,
                                     size_t size,
                                     const uint8_t *in,
                                     uint8_t *out,
                                     const uint8_t *iv);
  cryerror_t cry_lld_decrypt_AES_CTR(CRYDriver *cryp,
                                     crykey_t key_id,
                                     size_t size,
                                     const uint8_t *in,
                                     uint8_t *out,
                                     const uint8_t *iv);
#endif
#ifdef __cplusplus
}
#endif

#endif /* HAL_CRYPTO_LLD_H */

/** @} */
