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
 * @file    KEYSCANv1/hal_keyscan_lld.h
 * @brief   EFR32FG23 KEYSCAN (keypad matrix scanner) LLD header.
 *
 * @details The xG23 KEYSCAN peripheral drives up to 8 column outputs
 *          (COLOUT0..7) and senses up to 6 row inputs (ROWSENSE0..5),
 *          giving a matrix of up to 6x8 = 48 keys. Pin assignment is done
 *          through GPIO->KEYSCANROUTE (columns need a ROUTEEN bit, rows are
 *          pure inputs). The scanner is started with CMD.KEYSCANSTART and
 *          reports results in STATUS (ROW/COL/NOKEY) plus IF (KEY/NOKEY/
 *          SCANNED/WAKEUP).
 *
 *          This is an LLD-only driver (no portable HAL keypad model
 *          exists); enable with EFR32_KEYSCAN_USE_KEYSCAN TRUE in halconf.
 *
 * @addtogroup KEYSCAN
 * @{
 */

#ifndef HAL_KEYSCAN_LLD_H
#define HAL_KEYSCAN_LLD_H

#if !defined(EFR32_KEYSCAN_USE_KEYSCAN) || defined(__DOXYGEN__)
#define EFR32_KEYSCAN_USE_KEYSCAN   FALSE
#endif

#if !defined(EFR32_KEYSCAN_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define EFR32_KEYSCAN_IRQ_PRIORITY  3
#endif

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/** Maximum number of column outputs. */
#define EFR32_KEYSCAN_MAX_COLS      8
/** Maximum number of row inputs. */
#define EFR32_KEYSCAN_MAX_ROWS      6

/** Pad mode for a KEYSCAN-driven COLOUT pin: WIRED-AND with DOUT=1
    (the KEYSCAN pulls the scanned column low; a pressed key shorts it to
    the row). Rows are INPUTPULL with DOUT=1 (pull-up). */
#define EFR32_KEYSCAN_PAD_MODE_COL_OUT  (PAL_EFR32_MODE_WIREDAND | \
                                         PAL_EFR32_DOUT_HIGH)

/** Interrupt flags (KEYSCAN_IF/IEN bit masks). */
#define EFR32_KEYSCAN_IF_NOKEY      KEYSCAN_IF_NOKEY
#define EFR32_KEYSCAN_IF_KEY        KEYSCAN_IF_KEY
#define EFR32_KEYSCAN_IF_SCANNED    KEYSCAN_IF_SCANNED
#define EFR32_KEYSCAN_IF_WAKEUP     KEYSCAN_IF_WAKEUP
#define EFR32_KEYSCAN_IF_ALL        (KEYSCAN_IF_NOKEY | KEYSCAN_IF_KEY | \
                                     KEYSCAN_IF_SCANNED | KEYSCAN_IF_WAKEUP)

#ifdef __cplusplus
extern "C" {
#endif

  /* Callback: flags = KEYSCAN_IF_* that fired, row/col valid when KEY set. */
  typedef void (*keyscan_lld_callback_t)(uint32_t flags, uint8_t row, uint8_t col);

  void     keyscan_lld_init(void);
  void     keyscan_lld_start(void);
  void     keyscan_lld_stop(void);
  void     keyscan_lld_enable(void);
  void     keyscan_lld_disable(void);
  /* Config registers are only writable while EN=0 (xG23 bus-faults otherwise):
     call keyscan_lld_disable() first, then config, then keyscan_lld_enable(). */
  void     keyscan_lld_config(uint8_t rows, uint8_t cols, uint32_t clkdiv,
                              bool single_press);
  void     keyscan_lld_set_delay(uint8_t scandly, uint8_t debdly);
  void     keyscan_lld_route_col(uint8_t col, ioportid_t port, ioportmask_t pad);
  void     keyscan_lld_route_row(uint8_t row, ioportid_t port, ioportmask_t pad);
  void     keyscan_lld_unroute(void);
  void     keyscan_lld_scan_start(void);
  void     keyscan_lld_scan_stop(void);
  uint32_t keyscan_lld_get_status(void);
  bool     keyscan_lld_get_key(uint8_t *row, uint8_t *col);
  void     keyscan_lld_irq_enable(uint32_t mask);
  void     keyscan_lld_clear_flags(uint32_t mask);
  void     keyscan_lld_set_callback(keyscan_lld_callback_t cb);

#ifdef __cplusplus
}
#endif

#endif /* HAL_KEYSCAN_LLD_H */

/** @} */
