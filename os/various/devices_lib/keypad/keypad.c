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
 * @file    devices_lib/keypad/keypad.c
 * @brief   Generic matrix keypad device driver.
 *
 * @addtogroup KEYPAD
 * @{
 */

#include "hal.h"

#include <string.h>

#include "keypad.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

#define KEYPAD_SETTLE_MS        2U      /* row settle after switching        */

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

void keypadInit(Keypad *kp, keypad_adc_read_fn adc_read, void *adc_ctx,
                const ioline_t *rows, uint8_t nrows,
                const uint8_t *cols, uint8_t ncols,
                const char *keymap, uint16_t press_threshold) {
  uint8_t i;

  memset(kp, 0, sizeof(*kp));
  kp->adc_read = adc_read;
  kp->adc_ctx  = adc_ctx;
  kp->nrows = nrows > KEYPAD_MAX_ROWS ? KEYPAD_MAX_ROWS : nrows;
  kp->ncols = ncols > KEYPAD_MAX_COLS ? KEYPAD_MAX_COLS : ncols;
  for (i = 0U; i < kp->nrows; i++) {
    kp->rows[i] = rows[i];
    palSetLineMode(rows[i], PAL_MODE_INPUT);    /* rows idle hi-Z */
  }
  for (i = 0U; i < kp->ncols; i++) {
    kp->cols[i] = cols[i];
  }
  kp->keymap = keymap;
  kp->press_threshold = press_threshold;
}

char keypadScan(Keypad *kp) {
  uint8_t r, c;

  for (r = 0U; r < kp->nrows; r++) {
    /* Drive this row low, others hi-Z. */
    palSetLineMode(kp->rows[r], PAL_MODE_OUTPUT_OPENDRAIN);
    palClearLine(kp->rows[r]);
    chThdSleepMilliseconds(KEYPAD_SETTLE_MS);
    for (c = 0U; c < kp->ncols; c++) {
      uint32_t v = kp->adc_read(kp->adc_ctx, kp->cols[c], TIME_MS2I(10));
      if (v < kp->press_threshold) {
        palSetLineMode(kp->rows[r], PAL_MODE_INPUT);    /* restore row */
        if (kp->keymap != NULL) {
          return kp->keymap[r * kp->ncols + c];
        }
        return (char)('1' + r * kp->ncols + c);
      }
    }
    palSetLineMode(kp->rows[r], PAL_MODE_INPUT);
  }
  return 0;
}

/** @} */
