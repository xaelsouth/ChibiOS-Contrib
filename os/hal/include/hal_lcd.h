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
 * @file    hal_lcd.h
 * @brief   LCD segment controller driver macros and structures.
 *
 * @addtogroup LCD
 * @{
 */

#ifndef HAL_LCD_H
#define HAL_LCD_H

#if (HAL_USE_LCD == TRUE) || defined(__DOXYGEN__)

#include "hal_lcd_lld.h"

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Starts the LCD driver.
 */
#define lcdStart(lcdp, config)              lcd_lld_start(lcdp, config)

/**
 * @brief   Stops the LCD driver (disables controller, frees clock).
 */
#define lcdStop(lcdp)                       lcd_lld_stop(lcdp)

/**
 * @brief   Clears all segments in the shadow buffer (does not write HW yet).
 */
#define lcdClear(lcdp)                      lcd_lld_clear(lcdp)

/**
 * @brief   Writes shadow buffer to LCD SEGD registers (CPU).
 */
#define lcdUpdate(lcdp)                     lcd_lld_update(lcdp)

/**
 * @brief   Writes shadow buffer to LCD SEGD registers via DMA.
 */
#define lcdUpdateDMA(lcdp)                  lcd_lld_update_dma(lcdp)

/**
 * @brief   Sets/clears a raw segment in the shadow buffer.
 */
#define lcdSetSegment(lcdp, seg, on)        lcd_lld_set_segment(lcdp, seg, on)

/**
 * @brief   Writes a hex digit (0-F) at a digit position.
 */
#define lcdWriteDigit(lcdp, pos, digit)     lcd_lld_write_digit(lcdp, pos, digit)

/**
 * @brief   Sets/clears an icon element.
 */
#define lcdWriteIcon(lcdp, icon, on)        lcd_lld_write_icon(lcdp, icon, on)

#endif /* HAL_USE_LCD == TRUE */

#endif /* HAL_LCD_H */

/** @} */
