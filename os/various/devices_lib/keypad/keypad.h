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
 * @file    devices_lib/keypad/keypad.h
 * @brief   Generic matrix keypad device driver.
 *
 * @details Scans an n_rows x n_cols key matrix. Rows are GPIO lines driven
 *          low one at a time (others hi-Z); columns are read through an
 *          ADC channel each (column line pulled up to the ADC reference,
 *          a pressed key pulls its column near ground). The ADC is accessed
 *          through a caller-supplied read callback, so the driver is fully
 *          generic (works with any ADC - e.g. the i.MX23 LRADC via
 *          imx23_lradc_read(), or any HAL ADC driver).
 *
 *          Wiring schematic in readme.md.
 *
 * @addtogroup KEYPAD
 * @{
 */

#ifndef KEYPAD_H
#define KEYPAD_H

#include "hal.h"

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

#define KEYPAD_MAX_ROWS     8U
#define KEYPAD_MAX_COLS     8U

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   ADC column read callback: read channel ch, return the raw value.
 */
typedef uint32_t (*keypad_adc_read_fn)(void *ctx, uint8_t ch,
                                       sysinterval_t timeout);

/**
 * @brief   Matrix keypad object.
 */
typedef struct {
  keypad_adc_read_fn adc_read;              /**< column ADC read callback    */
  void      *adc_ctx;                       /**< callback context            */
  ioline_t   rows[KEYPAD_MAX_ROWS];         /**< row GPIO lines              */
  uint8_t    cols[KEYPAD_MAX_COLS];         /**< column ADC channels         */
  uint8_t    nrows;                         /**< active rows                 */
  uint8_t    ncols;                         /**< active columns              */
  uint16_t   press_threshold;               /**< raw below => pressed        */
  const char *keymap;                       /**< nrows*ncols chars, 0=none   */
} Keypad;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void keypadInit(Keypad *kp, keypad_adc_read_fn adc_read, void *adc_ctx,
                  const ioline_t *rows, uint8_t nrows,
                  const uint8_t *cols, uint8_t ncols,
                  const char *keymap, uint16_t press_threshold);
  /* Scan once; returns the keymap char of the pressed key, or 0. */
  char keypadScan(Keypad *kp);
#ifdef __cplusplus
}
#endif

#endif /* KEYPAD_H */

/** @} */
