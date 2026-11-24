/*
    ChibiOS/HAL - Copyright (C) 2024 Xael South

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
 * @file    hal_buram_lld.h
 * @brief   BURAM Driver subsystem low level driver header.
 *
 * @addtogroup BURAM
 * @{
 */

#ifndef HAL_BURAM_LLD_H_
#define HAL_BURAM_LLD_H_


/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*
 * The FG23 BURAM is the 32 RETx_REG retention registers (RM 17.2): 32 x 32-bit
 * = 128 bytes at BURAM_BASE + 0x000..0x07C (SET/CLR/TGL aliases at +0x1000/
 * +0x2000/+0x3000). There is a single BURAM peripheral with a single bus-clock
 * gate (CMU_CLKEN0.BURAM), so the driver models it as ONE 128-byte block
 * (BURAMD1). Users partition it by agreeing on byte offsets; the allocation
 * bitmap in hal_buram.c prevents overlaps.
 */
#define BURAM1_ADDRESS          (BURAM_BASE + 0UL)
#define BURAM1_SIZE             128U

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if BURAM1_SIZE % 32U != 0U
#error "BURAM1_SIZE has to be a multiple of 32!"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/
/**
 * @brief   Type of a structure representing a BURAM driver.
 */
typedef struct BURAMDriver BURAMDriver;

/**
 * @brief   Type of a structure representing a BURAM driver configuration.
 */
typedef struct BURAMConfig BURAMConfig;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/
#if BURAM_USE_BURAM1 && !defined(__DOXYGEN__)
extern BURAMDriver BURAMD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void buram_lld_start(BURAMDriver *sramp, const BURAMConfig *cfgp);
  void buram_lld_stop(BURAMDriver *sramp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_BURAM_LLD_H_ */

/** @} */
