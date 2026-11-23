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
 * @file    KEYSCANv1/hal_keyscan_lld.c
 * @brief   EFR32FG23 KEYSCAN (keypad matrix scanner) LLD source.
 *
 * @addtogroup KEYSCAN
 * @{
 */

#include "hal.h"
#include "hal_keyscan_lld.h"

#if (EFR32_KEYSCAN_USE_KEYSCAN == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

static keyscan_lld_callback_t keyscan_cb;

/*===========================================================================*/
/* IRQ handler.                                                              */
/*===========================================================================*/

OSAL_IRQ_HANDLER(EFR32_KEYSCAN_HANDLER) {
  OSAL_IRQ_PROLOGUE();

  uint32_t flags = KEYSCAN->IF & KEYSCAN->IEN;
  uint8_t row = (uint8_t)((KEYSCAN->STATUS & _KEYSCAN_STATUS_ROW_MASK)
                          >> _KEYSCAN_STATUS_ROW_SHIFT);
  uint8_t col = (uint8_t)((KEYSCAN->STATUS & _KEYSCAN_STATUS_COL_MASK)
                          >> _KEYSCAN_STATUS_COL_SHIFT);
  KEYSCAN->IF_CLR = flags;

  if (keyscan_cb != NULL) {
    keyscan_cb(flags, row, col);
  }

  OSAL_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/* Wait until the LF-domain synchronisation of KEYSCAN is done. */
static void keyscan_lld_sync(void) {
  while ((KEYSCAN->STATUS & KEYSCAN_STATUS_SYNCBUSY) != 0U) {
  }
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void keyscan_lld_init(void) {
  keyscan_cb = NULL;
}

void keyscan_lld_start(void) {
  CMU->CLKEN1_SET = CMU_CLKEN1_KEYSCAN;

  /* Make sure the peripheral is disabled before touching config regs
     (config writes bus-fault while EN=1 on xG23). */
  KEYSCAN->EN_CLR = KEYSCAN_EN_EN;
  while ((KEYSCAN->EN & KEYSCAN_EN_DISABLING) != 0U) {
  }

  KEYSCAN->IF_CLR = _KEYSCAN_IF_MASK;
  KEYSCAN->IEN = 0U;

  nvicEnableVector(EFR32_KEYSCAN_NUMBER, EFR32_KEYSCAN_IRQ_PRIORITY);

  keyscan_lld_enable();
}

void keyscan_lld_enable(void) {
  KEYSCAN->EN_SET = KEYSCAN_EN_EN;
  keyscan_lld_sync();
}

void keyscan_lld_disable(void) {
  keyscan_lld_sync();
  KEYSCAN->CMD = KEYSCAN_CMD_KEYSCANSTOP;
  keyscan_lld_sync();
  KEYSCAN->EN_CLR = KEYSCAN_EN_EN;
  while ((KEYSCAN->EN & KEYSCAN_EN_DISABLING) != 0U) {
  }
  keyscan_lld_sync();
}

void keyscan_lld_stop(void) {
  nvicDisableVector(EFR32_KEYSCAN_NUMBER);
  keyscan_lld_disable();
  CMU->CLKEN1_CLR = CMU_CLKEN1_KEYSCAN;
}

void keyscan_lld_config(uint8_t rows, uint8_t cols, uint32_t clkdiv,
                        bool single_press) {
  /* Valid while EN=0 only. rows: 3..6, cols: 1..8 (field value = n-1). */
  osalDbgAssert(rows >= 3U && rows <= EFR32_KEYSCAN_MAX_ROWS,
                "rows out of range");
  osalDbgAssert(cols >= 1U && cols <= EFR32_KEYSCAN_MAX_COLS,
                "cols out of range");

  uint32_t cfg = KEYSCAN->CFG;
  cfg &= ~(_KEYSCAN_CFG_NUMROWS_MASK | _KEYSCAN_CFG_NUMCOLS_MASK |
           _KEYSCAN_CFG_CLKDIV_MASK | _KEYSCAN_CFG_SINGLEPRESS_MASK |
           _KEYSCAN_CFG_AUTOSTART_MASK);
  cfg |= ((uint32_t)(rows - 1U) << _KEYSCAN_CFG_NUMROWS_SHIFT)
       | ((uint32_t)(cols - 1U) << _KEYSCAN_CFG_NUMCOLS_SHIFT)
       | (clkdiv << _KEYSCAN_CFG_CLKDIV_SHIFT);
  if (single_press) {
    cfg |= KEYSCAN_CFG_SINGLEPRESS_SINGLEPRESS;
  }
  KEYSCAN->CFG = cfg;
}

void keyscan_lld_set_delay(uint8_t scandly, uint8_t debdly) {
  /* DELAY: SCANDLY bits 8-11, DEBDLY bits 16-19 (both 2..32, value = n/2). */
  uint32_t dly = KEYSCAN->DELAY;
  dly &= ~(_KEYSCAN_DELAY_SCANDLY_MASK | _KEYSCAN_DELAY_DEBDLY_MASK);
  dly |= ((uint32_t)(scandly >> 1) << _KEYSCAN_DELAY_SCANDLY_SHIFT)
       | ((uint32_t)(debdly >> 1) << _KEYSCAN_DELAY_DEBDLY_SHIFT);
  KEYSCAN->DELAY = dly;
}

void keyscan_lld_route_col(uint8_t col, ioportid_t port, ioportmask_t pad) {
  osalDbgAssert(col < EFR32_KEYSCAN_MAX_COLS, "col out of range");
  volatile uint32_t *route = &GPIO->KEYSCANROUTE.COLOUT0ROUTE;
  route[col] = (GPIO_PORT_INDEX(port) << _GPIO_KEYSCAN_COLOUT0ROUTE_PORT_SHIFT)
             | ((uint32_t)pad << _GPIO_KEYSCAN_COLOUT0ROUTE_PIN_SHIFT);
  /* Columns are driven outputs: enable the pin in ROUTEEN. */
  GPIO->KEYSCANROUTE.ROUTEEN |= (0x1UL << col);
}

void keyscan_lld_route_row(uint8_t row, ioportid_t port, ioportmask_t pad) {
  osalDbgAssert(row < EFR32_KEYSCAN_MAX_ROWS, "row out of range");
  volatile uint32_t *route = &GPIO->KEYSCANROUTE.ROWSENSE0ROUTE;
  route[row] = (GPIO_PORT_INDEX(port) << _GPIO_KEYSCAN_ROWSENSE0ROUTE_PORT_SHIFT)
             | ((uint32_t)pad << _GPIO_KEYSCAN_ROWSENSE0ROUTE_PIN_SHIFT);
  /* Rows are pure inputs — no ROUTEEN bit exists for them. */
}

void keyscan_lld_unroute(void) {
  GPIO->KEYSCANROUTE.ROUTEEN = 0U;
  for (uint8_t i = 0U; i < EFR32_KEYSCAN_MAX_COLS; i++) {
    volatile uint32_t *cr = &GPIO->KEYSCANROUTE.COLOUT0ROUTE;
    cr[i] = 0U;
  }
  for (uint8_t i = 0U; i < EFR32_KEYSCAN_MAX_ROWS; i++) {
    volatile uint32_t *rr = &GPIO->KEYSCANROUTE.ROWSENSE0ROUTE;
    rr[i] = 0U;
  }
}

void keyscan_lld_scan_start(void) {
  keyscan_lld_sync();
  KEYSCAN->IF_CLR = _KEYSCAN_IF_MASK;
  KEYSCAN->CMD = KEYSCAN_CMD_KEYSCANSTART;
  keyscan_lld_sync();
}

void keyscan_lld_scan_stop(void) {
  keyscan_lld_sync();
  KEYSCAN->CMD = KEYSCAN_CMD_KEYSCANSTOP;
  keyscan_lld_sync();
}

uint32_t keyscan_lld_get_status(void) {
  return KEYSCAN->STATUS;
}

bool keyscan_lld_get_key(uint8_t *row, uint8_t *col) {
  uint32_t st = KEYSCAN->STATUS;
  if ((st & KEYSCAN_STATUS_NOKEY) != 0U) {
    return false;
  }
  if (row != NULL) {
    *row = (uint8_t)((st & _KEYSCAN_STATUS_ROW_MASK) >> _KEYSCAN_STATUS_ROW_SHIFT);
  }
  if (col != NULL) {
    *col = (uint8_t)((st & _KEYSCAN_STATUS_COL_MASK) >> _KEYSCAN_STATUS_COL_SHIFT);
  }
  return true;
}

void keyscan_lld_irq_enable(uint32_t mask) {
  KEYSCAN->IEN |= mask;
}

void keyscan_lld_clear_flags(uint32_t mask) {
  KEYSCAN->IF_CLR = mask;
}

void keyscan_lld_set_callback(keyscan_lld_callback_t cb) {
  keyscan_cb = cb;
}

#endif /* EFR32_KEYSCAN_USE_KEYSCAN == TRUE */

/** @} */
