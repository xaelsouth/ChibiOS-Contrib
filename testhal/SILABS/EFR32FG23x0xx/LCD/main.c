/*
    ChibiOS - Copyright (C) 2026 Xael South
    Apache License 2.0 — see repository root.

    LCD test: visual pattern on the BRD2600A CE322-1002 glass, looped,
    each phase followed by a 1 s blank:
      1) DEGC icon                             1 s + blank
      2) all dot icons P2..P6                  1 s + blank
      3) "8" walking left->right over 5 digits  5 s (1 s per digit) + blank
      4) all segments on                       1 s + blank
*/
#include "ch.h"
#include "hal.h"
#include "rtt_console.h"
#include "hal_lcd.h"

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nLCD pattern: DEGC 1s, blank, dots 1s, blank, walking 8 5s, blank, all on 1s, blank\r\n");
  lcdStart(&LCDD1, &board_lcd_config);
  while (true) {
    /* 1) DEGC icon (symbol 5), 1 s + blank */
    lcdClear(&LCDD1);
    lcdWriteIcon(&LCDD1, 5, true);
    lcdUpdate(&LCDD1);
    chThdSleepMilliseconds(1000);
    lcdClear(&LCDD1);
    lcdUpdate(&LCDD1);
    chThdSleepMilliseconds(1000);

    /* 2) all dot icons P2..P6 (symbols 0..4), 1 s + blank */
    lcdClear(&LCDD1);
    for (uint32_t icon = 0; icon < 5; icon++)
      lcdWriteIcon(&LCDD1, icon, true);
    lcdUpdate(&LCDD1);
    chThdSleepMilliseconds(1000);
    lcdClear(&LCDD1);
    lcdUpdate(&LCDD1);
    chThdSleepMilliseconds(1000);

    /* 3) "8" walking left (#5, pos 0) -> right (#1, pos 4), 1 s each, + blank */
    for (uint32_t pos = 0; pos < 5; pos++) {
      lcdClear(&LCDD1);
      lcdWriteDigit(&LCDD1, pos, 8);
      lcdUpdate(&LCDD1);
      chThdSleepMilliseconds(1000);
    }
    lcdClear(&LCDD1);
    lcdUpdate(&LCDD1);
    chThdSleepMilliseconds(1000);

    /* 4) all segments on, 1 s + blank */
    lcdClear(&LCDD1);
    for (uint32_t seg = 0; seg < EFR32_LCD_MAX_SEGMENTS; seg++)
      lcdSetSegment(&LCDD1, seg, true);
    lcdUpdate(&LCDD1);
    chThdSleepMilliseconds(1000);
    lcdClear(&LCDD1);
    lcdUpdate(&LCDD1);
    chThdSleepMilliseconds(1000);
  }
}
