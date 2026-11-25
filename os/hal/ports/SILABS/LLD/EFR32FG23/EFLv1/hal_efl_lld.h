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
 * @file    EFLv1/hal_efl_lld.h
 * @brief   EFR32FG23 Embedded Flash subsystem low level driver header.
 *
 * @details The FG23 MSC (Memory System Controller) manages the main flash
 *          (512 KB at 0x08000000, 8 KB pages) and the USERDATA page
 *          (1 KB at 0x0FE00000). Flash is memory-mapped for reads;
 *          program/erase go through MSC registers.
 *
 *          The EFL driver exposes the main flash region. The USERDATA page
 *          is typically used for calibration/config storage and can be
 *          accessed separately via efl_lld_read/program/erase on the
 *          user sector.
 *
 * @addtogroup HAL_EFL
 * @{
 */

#ifndef HAL_EFL_LLD_H
#define HAL_EFL_LLD_H

#if (HAL_USE_EFL == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/** Main flash base address (memory mapped). */
#define EFR32_FLASH_BASE            0x08000000UL
/** Main flash size (512 KB). */
#define EFR32_FLASH_SIZE            0x00080000UL
/** Flash page size (8 KB). */
#define EFR32_FLASH_PAGE_SIZE       0x00002000UL
/** Number of pages in main flash. */
#define EFR32_FLASH_PAGES           (EFR32_FLASH_SIZE / EFR32_FLASH_PAGE_SIZE)

/** USERDATA page base address. */
#define EFR32_USERDATA_BASE         0x0FE00000UL
/** USERDATA page size (1 KB). */
#define EFR32_USERDATA_SIZE         0x00000400UL

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

#if !defined(EFR32_EFL_USE_EFL1) || defined(__DOXYGEN__)
#define EFR32_EFL_USE_EFL1          FALSE
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

#define efl_lld_driver_fields                                               \
  /* Pointer to MSC registers (always MSC). */                             \
  void                     *msc;


#define efl_lld_config_fields                                               \
  uint32_t                  dummy;


/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if (EFR32_EFL_USE_EFL1 == TRUE) && !defined(__DOXYGEN__)
extern EFlashDriver EFLD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void efl_lld_init(void);
  void efl_lld_start(EFlashDriver *eflp);
  void efl_lld_stop(EFlashDriver *eflp);
  const flash_descriptor_t *efl_lld_get_descriptor(void *instance);
  flash_error_t efl_lld_read(void *instance, flash_offset_t offset,
                             size_t n, uint8_t *rp);
  flash_error_t efl_lld_program(void *instance, flash_offset_t offset,
                                size_t n, const uint8_t *pp);
  flash_error_t efl_lld_start_erase_all(void *instance);
  flash_error_t efl_lld_start_erase_sector(void *instance,
                                           flash_sector_t sector);
  flash_error_t efl_lld_query_erase(void *instance, uint32_t *msec);
  flash_error_t efl_lld_verify_erase(void *instance, flash_sector_t sector);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_EFL == TRUE */

#endif /* HAL_EFL_LLD_H */

/** @} */
