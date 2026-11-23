/*
    ChibiOS - Copyright (C) 2026 Xael South
    Apache License 2.0 — see repository root.

    LCD test: LCD segment controller (LCDv1 LLD on FG23, BRD2600A
    CE322-1002 glass via board_lcd_model/board_lcd_config).
      1) start controller (quadruplex, 1/3 bias, charge pump) -> EN set,
         DISPCTRL/BIASCTRL fields as configured
      2) clear + update                            -> SEGD0..3 read 0
      3) write digit 8 at pos 0 (all 7 seg of #1)   -> SEGD bits per glass map
      4) write digit 0                             -> a..f only
      5) raw segment 79 on                         -> SEGD3 bit19 (0x80000)
      6) icon 0 (P2) on                            -> SEGD0 bit5 (0x20)
      7) stop                                      -> CLKEN1.LCD gated off
      8) restart and switch ALL 80 segments on     -> watch the LCD!
*/
#include "ch.h"
#include "hal.h"
#include "rtt_console.h"
#include "hal_lcd.h"

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nLCD test: LCDv1 segment controller, BRD2600A CE322-1002 glass\r\n");

  bool ok = true;

  lcd_lld_init();
  lcdStart(&LCDD1, &board_lcd_config);
  bool en = ((LCD->EN & LCD_EN_EN) != 0U);
  bool lfrco = ((CMU->CLKEN0 & CMU_CLKEN0_LFRCO) != 0U);
  bool segcom = ((GPIO->LCDSEG != 0U) && (GPIO->LCDCOM != 0U));
  uint32_t dispctrl = LCD->DISPCTRL, biasctrl = LCD->BIASCTRL;
  bool mux_ok  = ((dispctrl & _LCD_DISPCTRL_MUX_MASK)  == EFR32_LCD_MUX_QUADRUPLEX);
  bool bias_ok = ((dispctrl & _LCD_DISPCTRL_BIAS_MASK) == EFR32_LCD_BIAS_ONETHIRD);
  bool wave_ok = ((dispctrl & _LCD_DISPCTRL_WAVE_MASK) == EFR32_LCD_WAVE_LOPOWER);
  bool mode_ok = ((biasctrl & _LCD_BIASCTRL_MODE_MASK) == EFR32_LCD_MODE_CHARGEPUMP);
  bool vlcd_ok = (((biasctrl & _LCD_BIASCTRL_VLCD_MASK) >> _LCD_BIASCTRL_VLCD_SHIFT)
                  == EFR32_LCD_CONTRAST_DEFAULT);
  PRINTF("after start: EN=%d LFRCO=%d LCDSEG=0x%lX LCDCOM=0x%lX %s\r\n",
         (int)en, (int)lfrco, (unsigned long)GPIO->LCDSEG,
         (unsigned long)GPIO->LCDCOM,
         (en && lfrco && segcom) ? "PASS" : "FAIL");
  PRINTF("DISPCTRL=0x%08lX (mux=%d bias=%d wave=%d) BIASCTRL=0x%08lX (mode=%d vlcd=%d) %s\r\n",
         (unsigned long)dispctrl, (int)mux_ok, (int)bias_ok, (int)wave_ok,
         (unsigned long)biasctrl, (int)mode_ok, (int)vlcd_ok,
         (mux_ok && bias_ok && wave_ok && mode_ok && vlcd_ok) ? "PASS" : "FAIL");
  ok = ok && en && lfrco && segcom && mux_ok && bias_ok && wave_ok
       && mode_ok && vlcd_ok && (LCDD1.state == LCD_READY);

  /* 2) clear + update -> SEGD all zero. */
  lcdClear(&LCDD1);
  lcdUpdate(&LCDD1);
  uint32_t s0 = LCD->SEGD0, s1 = LCD->SEGD1, s2 = LCD->SEGD2, s3 = LCD->SEGD3;
  bool clr = (s0 | s1 | s2 | s3) == 0U;
  PRINTF("clear+update: SEGD=%08lX %08lX %08lX %08lX %s\r\n",
         (unsigned long)s0, (unsigned long)s1, (unsigned long)s2,
         (unsigned long)s3, clr ? "PASS" : "FAIL");
  ok = ok && clr;

  /* 3) digit 8 at pos 0 (glass digit #1): font 0x7F -> segments
     {60,41,1,0,20,40,21} set: SEGD0 bits 0,1,20? -> check combined mask. */
  lcdWriteDigit(&LCDD1, 0, 8);
  lcdUpdate(&LCDD1);
  /* glass digit #1 segments {60,41,1,0,20,40,21}: linear idx = com*20+line,
     so SEGD[seg/20] bit (seg%20); all are < 40 -> SEGD0/SEGD1. */
  uint32_t mask8 = 0;
  { const uint8_t segs[7] = { 60, 41, 1, 0, 20, 40, 21 };
    for (int i = 0; i < 7; i++) {
      mask8 |= 1UL << (segs[i] % 20U);
    } }
  uint32_t got8 = (LCD->SEGD0 & 0xFFFFFU) | ((LCD->SEGD1 & 0xFFFFFU) << 20);
  bool d8 = (got8 & mask8) == mask8;
  PRINTF("digit 8 pos0: SEGD0=%08lX SEGD1=%08lX mask=%08lX %s\r\n",
         (unsigned long)LCD->SEGD0, (unsigned long)LCD->SEGD1,
         (unsigned long)mask8, d8 ? "PASS" : "FAIL");
  ok = ok && d8;

  /* 4) digit 0 -> font 0x3F (a..f, no g): segment 21 (g) must be clear. */
  lcdWriteDigit(&LCDD1, 0, 0);
  lcdUpdate(&LCDD1);
  bool d0 = ((LCD->SEGD1 & (1UL << (21 % 20))) == 0U) &&
            ((LCD->SEGD0 & (1UL << 0)) != 0U);
  PRINTF("digit 0 pos0: SEGD0=%08lX SEGD1=%08lX (g=seg21 clear) %s\r\n",
         (unsigned long)LCD->SEGD0, (unsigned long)LCD->SEGD1,
         d0 ? "PASS" : "FAIL");
  ok = ok && d0;

  /* 5) raw segment 79 -> SEGD3 (79/20=3), bit 19 (79%20=19) -> 0x80000. */
  lcdClear(&LCDD1);
  lcdSetSegment(&LCDD1, 79, true);
  lcdUpdate(&LCDD1);
  s3 = LCD->SEGD3;
  PRINTF("segment 79: SEGD3=%08lX (expect 0x80000) %s\r\n",
         (unsigned long)s3, s3 == 0x80000UL ? "PASS" : "FAIL");
  ok = ok && (s3 == 0x80000UL);

  /* 6) icon 0 (P2) -> C0/S03 -> segment 5 -> SEGD0 bit5 = 0x20. */
  lcdClear(&LCDD1);
  lcdWriteIcon(&LCDD1, BOARD_LCD_ICON_P2, true);
  lcdUpdate(&LCDD1);
  s0 = LCD->SEGD0;
  PRINTF("icon P2: SEGD0=%08lX (expect 0x20) %s\r\n",
         (unsigned long)s0, s0 == 0x20UL ? "PASS" : "FAIL");
  ok = ok && (s0 == 0x20UL);

  /* 7) stop -> bus clock gated off. */
  lcdStop(&LCDD1);
  uint32_t clk = CMU->CLKEN1 & CMU_CLKEN1_LCD;
  PRINTF("after stop CLKEN1.LCD=0x%lX (expect 0) %s\r\n",
         (unsigned long)clk, (clk == 0U) ? "PASS" : "FAIL");
  ok = ok && (clk == 0U);

  PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL");

  /* 8) Restart and switch ALL 80 segments on so the whole panel lights up. */
  lcdStart(&LCDD1, &board_lcd_config);
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
