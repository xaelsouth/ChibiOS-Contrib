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
 * @file    EFLv1/hal_efl_lld.c
 * @brief   EFR32FG23 Embedded Flash subsystem low level driver.
 *
 * @details Implements read/program/erase on the FG23 main flash via the
 *          MSC (Memory System Controller). Flash is memory-mapped at
 *          0x08000000 for reads. Program and erase go through MSC registers:
 *
 *          Unlock:  MSC->LOCK = MSC_LOCK_LOCKKEY_UNLOCK (0x1B71)
 *          Enable:  MSC->WRITECTRL_SET = MSC_WRITECTRL_WREN
 *          Erase:   MSC->ADDRB = page_addr; MSC->WRITECMD = MSC_WRITECMD_ERASEPAGE
 *          Program: MSC->ADDRB = addr; MSC->WDATA = word; (repeat per word)
 *                   MSC->WRITECMD = MSC_WRITECMD_WRITEEND
 *          Lock:    MSC->LOCK = MSC_LOCK_LOCKKEY_LOCK (0x0000)
 *
 *          The USERDATA page (0x0FE00000, 1 KB) is sector number
 *          EFR32_FLASH_PAGES (i.e. index 64) in the descriptor.
 *
 * @addtogroup HAL_EFL
 * @{
 */

#include <string.h>
#include "hal.h"

#if (HAL_USE_EFL == TRUE) && (EFR32_EFL_USE_EFL1 == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

EFlashDriver EFLD1;

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/* Non-uniform sector descriptors (main pages uniform, userdata differs).
   Filled at init time by efl_lld_init(). */
static flash_sector_descriptor_t efl_sector_desc[EFR32_FLASH_PAGES + 1];

/* Flash descriptor.  Non-const only because it points at the runtime-filled
   sector array; it is never written after init. */
static flash_descriptor_t efl_descriptor = {
  .attributes    = FLASH_ATTR_ERASED_IS_ONE | FLASH_ATTR_MEMORY_MAPPED | FLASH_ATTR_REWRITABLE,
  .page_size     = 4,                          /* 32-bit word programming */
  .sectors_count = EFR32_FLASH_PAGES + 1,      /* 64 main + 1 userdata */
  .sectors       = efl_sector_desc,            /* non-uniform: userdata differs */
  .sectors_size  = 0,                          /* non-uniform sectors */
  .address       = (uint8_t *)EFR32_FLASH_BASE,
  .size          = EFR32_FLASH_SIZE,
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static void msc_unlock(void) {
  MSC->LOCK = MSC_LOCK_LOCKKEY_UNLOCK;
}

static void msc_lock(void) {
  MSC->LOCK = MSC_LOCK_LOCKKEY_LOCK;
}

static void msc_write_enable(void) {
  MSC->WRITECTRL_SET = MSC_WRITECTRL_WREN;
}

static void msc_write_disable(void) {
  MSC->WRITECTRL_CLR = MSC_WRITECTRL_WREN;
}

static bool msc_is_busy(void) {
  return (MSC->STATUS & MSC_STATUS_BUSY) != 0U;
}

/*
 * MSC flash erase and write require the EM0/EM1 voltage scale at
 * level VSCALE2 (see em_msc.c: "Flash erase and write requires VSCALE2").
 * These helpers read back the current level from EMU->STATUS, switch to
 * VSCALE2 and restore the saved level afterwards.  They are idempotent,
 * so the VSCALE2 level can be held across the asynchronous erase until
 * efl_lld_query_erase()/efl_lld_verify_erase() report completion.
 * Only ever raises to VSCALE2 and restores a level that was valid for the
 * configured HCLK, so no invalid transition can occur.
 */
static uint32_t msc_saved_vscale;
static bool msc_vscale_held;

static void msc_vscale_enter(void) {
  if (msc_vscale_held) {
    return;
  }
  msc_saved_vscale = (EMU->STATUS & _EMU_STATUS_VSCALE_MASK) >> _EMU_STATUS_VSCALE_SHIFT;
  EMU->CMD = (EMU->CMD & ~(_EMU_CMD_EM01VSCALE1_MASK | _EMU_CMD_EM01VSCALE2_MASK)) |
             (EMU_STATUS_VSCALE_VSCALE2 >> _EMU_STATUS_VSCALE_SHIFT << _EMU_CMD_EM01VSCALE1_SHIFT);
  while ((EMU->STATUS & _EMU_STATUS_VSCALEBUSY_MASK) == EMU_STATUS_VSCALEBUSY) {
  }
  msc_vscale_held = true;
}

static void msc_vscale_exit(void) {
  if (!msc_vscale_held) {
    return;
  }
  EMU->CMD = (EMU->CMD & ~(_EMU_CMD_EM01VSCALE1_MASK | _EMU_CMD_EM01VSCALE2_MASK)) |
             (msc_saved_vscale << _EMU_CMD_EM01VSCALE1_SHIFT);
  while ((EMU->STATUS & _EMU_STATUS_VSCALEBUSY_MASK) == EMU_STATUS_VSCALEBUSY) {
  }
  msc_vscale_held = false;
}

static void msc_wait_not_busy(void) {
  while (msc_is_busy()) {
    /* Wait for erase/program to complete. */
  }
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void efl_lld_init(void) {
  /* Enable the MSC bus clock — without it register access bus-faults. */
  CMU->CLKEN1_SET = CMU_CLKEN1_MSC;

  /* No VSCALE2 handling here: init only touches MSC registers, never the
     flash array.  VSCALE2 is forced by efl_lld_program and the erase
     paths, where the flash pump actually needs it. */

  /* Build sector descriptors at init time. */
  for (uint32_t i = 0; i < EFR32_FLASH_PAGES; i++) {
    efl_sector_desc[i].offset = i * EFR32_FLASH_PAGE_SIZE;
    efl_sector_desc[i].size   = EFR32_FLASH_PAGE_SIZE;
  }
  /* USERDATA is the last "sector". */
  efl_sector_desc[EFR32_FLASH_PAGES].offset = EFR32_FLASH_SIZE;
  efl_sector_desc[EFR32_FLASH_PAGES].size   = EFR32_USERDATA_SIZE;

  eflObjectInit(&EFLD1);
}

void efl_lld_start(EFlashDriver *eflp) {
  if (eflp->state == FLASH_STOP) {
    eflp->msc = (void *)MSC;
    eflp->state = FLASH_READY;
  }
}

void efl_lld_stop(EFlashDriver *eflp) {
  if (eflp->state == FLASH_READY) {
    msc_write_disable();
    msc_lock();
    eflp->state = FLASH_STOP;
  }
}

const flash_descriptor_t *efl_lld_get_descriptor(void *instance) {
  (void)instance;
  /* Single stable descriptor object; the sector list is filled by
     efl_lld_init() before the driver is started. */
  return &efl_descriptor;
}

/* Map a flash offset to an absolute address; offsets at/above
   EFR32_FLASH_SIZE address the USERDATA page. */
static uint32_t efl_lld_addr(flash_offset_t offset, size_t n, bool *ok) {
  if (offset >= EFR32_FLASH_SIZE) {
    if (offset + n > EFR32_FLASH_SIZE + EFR32_USERDATA_SIZE) {
      *ok = false;
      return 0U;
    }
    *ok = true;
    return EFR32_USERDATA_BASE + (offset - EFR32_FLASH_SIZE);
  }
  if (offset + n > EFR32_FLASH_SIZE) {
    *ok = false;
    return 0U;
  }
  *ok = true;
  return EFR32_FLASH_BASE + offset;
}

flash_error_t efl_lld_read(void *instance, flash_offset_t offset,
                           size_t n, uint8_t *rp) {
  (void)instance;
  bool ok;
  uint32_t addr = efl_lld_addr(offset, n, &ok);
  if (!ok) {
    return FLASH_ERROR_READ;
  }
  memcpy(rp, (const void *)addr, n);
  return FLASH_NO_ERROR;
}

flash_error_t efl_lld_program(void *instance, flash_offset_t offset,
                              size_t n, const uint8_t *pp) {
  (void)instance;

  if ((n % 4U) != 0U || (((uint32_t)pp & 3U) != 0U)) {
    /* FG23 MSC programs 32-bit words only, aligned. */
    return FLASH_ERROR_PROGRAM;
  }
  bool ok;
  uint32_t addr = efl_lld_addr(offset, n, &ok);
  if (!ok) {
    return FLASH_ERROR_PROGRAM;
  }

  osalSysLock();
  msc_vscale_enter();
  msc_unlock();
  msc_write_enable();

  for (size_t i = 0; i < n; i += 4) {
    uint32_t word;
    memcpy(&word, pp + i, 4);
    MSC->ADDRB = addr + i;
    MSC->WDATA = word;
    msc_wait_not_busy();
    if (MSC->STATUS & MSC_STATUS_INVADDR) {
      MSC->WRITECMD = MSC_WRITECMD_WRITEEND;
      msc_write_disable();
      msc_lock();
      msc_vscale_exit();
      osalSysUnlock();
      return FLASH_ERROR_PROGRAM;
    }
  }

  MSC->WRITECMD = MSC_WRITECMD_WRITEEND;
  msc_wait_not_busy();
  msc_write_disable();
  msc_lock();
  msc_vscale_exit();
  osalSysUnlock();

  return FLASH_NO_ERROR;
}

flash_error_t efl_lld_start_erase_all(void *instance) {
  (void)instance;
  /* Mass erase of main flash region 0.  VSCALE2 is held until the erase
     completes (see efl_lld_query_erase/efl_lld_verify_erase). */
  osalSysLock();
  msc_vscale_enter();
  msc_unlock();
  msc_write_enable();
  MSC->WRITECMD = MSC_WRITECMD_ERASEMAIN0;
  osalSysUnlock();
  return FLASH_NO_ERROR;
}

flash_error_t efl_lld_start_erase_sector(void *instance, flash_sector_t sector) {
  (void)instance;
  if (sector >= EFR32_FLASH_PAGES + 1) {
    return FLASH_ERROR_ERASE;
  }

  uint32_t addr;
  if (sector == EFR32_FLASH_PAGES) {
    /* USERDATA page. */
    addr = EFR32_USERDATA_BASE;
  } else {
    addr = EFR32_FLASH_BASE + sector * EFR32_FLASH_PAGE_SIZE;
  }

  osalSysLock();
  msc_vscale_enter();
  msc_unlock();
  msc_write_enable();
  MSC->ADDRB = addr;
  MSC->WRITECMD = MSC_WRITECMD_ERASEPAGE;
  osalSysUnlock();

  return FLASH_NO_ERROR;
}

flash_error_t efl_lld_query_erase(void *instance, uint32_t *msec) {
  (void)instance;
  if (msc_is_busy()) {
    if (msec != NULL) *msec = 1;  /* suggest 1ms poll interval */
    return FLASH_BUSY_ERASING;
  }
  msc_vscale_exit();  /* erase finished — restore the saved vscale level */
  return FLASH_NO_ERROR;
}

flash_error_t efl_lld_verify_erase(void *instance, flash_sector_t sector) {
  (void)instance;
  msc_vscale_exit();  /* no-op unless an erase is still holding VSCALE2 */
  if (sector >= EFR32_FLASH_PAGES + 1) {
    return FLASH_ERROR_VERIFY;
  }

  uint32_t base, size;
  if (sector == EFR32_FLASH_PAGES) {
    base = EFR32_USERDATA_BASE;
    size = EFR32_USERDATA_SIZE;
  } else {
    base = EFR32_FLASH_BASE + sector * EFR32_FLASH_PAGE_SIZE;
    size = EFR32_FLASH_PAGE_SIZE;
  }

  /* Erased flash reads as 0xFF. */
  const volatile uint32_t *p = (const volatile uint32_t *)base;
  for (uint32_t i = 0; i < size / 4; i++) {
    if (p[i] != 0xFFFFFFFFUL) {
      return FLASH_ERROR_VERIFY;
    }
  }
  return FLASH_NO_ERROR;
}

#endif /* HAL_USE_EFL && EFR32_EFL_USE_EFL1 */

/** @} */
