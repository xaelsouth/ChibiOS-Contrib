/*
    ChibiOS - Copyright (C) 2026 Xael South
    Apache License 2.0 — see repository root.

    LCD test: LCD segment controller (LCDv1 LLD on FG23).
      1) start controller (quadruplex mux)         -> EN bit set
      2) clear + update                            -> SEGD0..3 read 0
      3) write digit 8 at pos 0 (all 7 seg + DP)   -> SEGD0 == 0xFF
      4) write digit 0                             -> SEGD0 == 0x3F
      5) raw segment 79 on                         -> SEGD3 bit19 (0x80000)
      6) icon 0 on (segment 8)                     -> SEGD0 bit8
      7) stop                                      -> CLKEN1.LCD gated off
*/
#include "ch.h"
#include "hal.h"
#include "rtt_console.h"
#include "hal_lcd.h"

/* Test screen model: 1 digit (segments 0..7 = a..g,DP), 1 icon (segment 8). */
static const uint8_t test_digit_map[1][8] = {
  { 0, 1, 2, 3, 4, 5, 6, 7 }
};
static const uint8_t test_icon_map[1] = { 8 };
static const lcd_model_t test_model = {
  .num_digits = 1,
  .num_icons  = 1,
  .digit_map  = test_digit_map,
  .icon_map   = test_icon_map,
};

static const LCDConfig lcd_cfg = {
  .mux        = EFR32_LCD_MUX_QUADRUPLEX,
  .wave       = LCD_DISPCTRL_WAVE_TYPEA,
  .frame_rate = 0,
  .contrast   = 0,
  .seg_enable = 0x000FFFFFU,   /* all 20 segment outputs */
  .com_enable = 0x0000000FU,   /* all 4 COM outputs */
  .model      = &test_model,
  .callback   = NULL,
  .callback_arg = NULL,
};

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nLCD test: LCDv1 segment controller\r\n");

  bool ok = true;

  lcd_lld_init();
  lcdStart(&LCDD1, &lcd_cfg);
  bool en = ((LCD->EN & LCD_EN_EN) != 0U);
  bool lfrco = ((CMU->CLKEN0 & CMU_CLKEN0_LFRCO) != 0U);
  bool segcom = ((GPIO->LCDSEG != 0U) && (GPIO->LCDCOM != 0U));
  PRINTF("after start: EN=%d state=%d LFRCO=%d LCDSEG=0x%lX LCDCOM=0x%lX %s\r\n",
         (int)en, (int)LCDD1.state, (int)lfrco,
         (unsigned long)GPIO->LCDSEG, (unsigned long)GPIO->LCDCOM,
         (en && lfrco && segcom) ? "PASS" : "FAIL");
  ok = ok && en && lfrco && segcom && (LCDD1.state == LCD_READY);

  /* 2) clear + update -> SEGD all zero. */
  lcdClear(&LCDD1);
  lcdUpdate(&LCDD1);
  uint32_t s0 = LCD->SEGD0, s1 = LCD->SEGD1, s2 = LCD->SEGD2, s3 = LCD->SEGD3;
  bool clr = (s0 | s1 | s2 | s3) == 0U;
  PRINTF("clear+update: SEGD=%08lX %08lX %08lX %08lX %s\r\n",
         (unsigned long)s0, (unsigned long)s1, (unsigned long)s2,
         (unsigned long)s3, clr ? "PASS" : "FAIL");
  ok = ok && clr;

  /* 3) digit 8 -> font[8]=0x7F (segments a..g, no DP) -> SEGD0 = 0x7F. */
  lcdWriteDigit(&LCDD1, 0, 8);
  lcdUpdate(&LCDD1);
  s0 = LCD->SEGD0;
  PRINTF("digit 8: SEGD0=%08lX (expect 0x7F) %s\r\n",
         (unsigned long)s0, (s0 & 0xFFU) == 0x7FU ? "PASS" : "FAIL");
  ok = ok && ((s0 & 0xFFU) == 0x7FU);

  /* 4) digit 0 -> font 0x3F. */
  lcdWriteDigit(&LCDD1, 0, 0);
  lcdUpdate(&LCDD1);
  s0 = LCD->SEGD0;
  PRINTF("digit 0: SEGD0=%08lX (expect 0x3F) %s\r\n",
         (unsigned long)s0, (s0 & 0xFFU) == 0x3FU ? "PASS" : "FAIL");
  ok = ok && ((s0 & 0xFFU) == 0x3FU);

  /* 5) raw segment 79 -> SEGD3 (79/20=3), bit 19 (79%20=19) -> 0x80000. */
  lcdClear(&LCDD1);
  lcdSetSegment(&LCDD1, 79, true);
  lcdUpdate(&LCDD1);
  s3 = LCD->SEGD3;
  PRINTF("segment 79: SEGD3=%08lX (expect 0x80000) %s\r\n",
         (unsigned long)s3, s3 == 0x80000UL ? "PASS" : "FAIL");
  ok = ok && (s3 == 0x80000UL);

  /* 6) icon 0 -> segment 8 -> SEGD0 bit8. */
  lcdClear(&LCDD1);
  lcdWriteIcon(&LCDD1, 0, true);
  lcdUpdate(&LCDD1);
  s0 = LCD->SEGD0;
  PRINTF("icon 0: SEGD0=%08lX (expect 0x100) %s\r\n",
         (unsigned long)s0, s0 == 0x100UL ? "PASS" : "FAIL");
  ok = ok && (s0 == 0x100UL);

  /* 7) stop -> bus clock gated off. */
  lcdStop(&LCDD1);
  uint32_t clk = CMU->CLKEN1 & CMU_CLKEN1_LCD;
  PRINTF("after stop CLKEN1.LCD=0x%lX (expect 0) %s\r\n",
         (unsigned long)clk, (clk == 0U) ? "PASS" : "FAIL");
  ok = ok && (clk == 0U);

  PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL");

  /* Restart and switch ALL 80 segments on so the whole panel lights up. */
  lcdStart(&LCDD1, &lcd_cfg);
  for (uint32_t seg = 0; seg < EFR32_LCD_MAX_SEGMENTS; seg++) {
    lcdSetSegment(&LCDD1, seg, true);
  }
  lcdUpdate(&LCDD1);
  uint32_t a0 = LCD->SEGD0, a1 = LCD->SEGD1, a2 = LCD->SEGD2, a3 = LCD->SEGD3;
  bool all_on = ((a0 & 0xFFFFFU) == 0xFFFFFU) && ((a1 & 0xFFFFFU) == 0xFFFFFU) &&
                ((a2 & 0xFFFFFU) == 0xFFFFFU) && ((a3 & 0xFFFFFU) == 0xFFFFFU);
  PRINTF("all segments on: SEGD=%08lX %08lX %08lX %08lX %s (watch the LCD)\r\n",
         (unsigned long)a0, (unsigned long)a1, (unsigned long)a2, (unsigned long)a3,
         all_on ? "PASS" : "FAIL");
  ok = ok && all_on;
  PRINTF("final result: %s\r\n", ok ? "PASS" : "FAIL");

  while (true) { chThdSleepMilliseconds(1000); }
}
