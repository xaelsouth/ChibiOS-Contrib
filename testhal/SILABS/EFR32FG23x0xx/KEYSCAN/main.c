/*
    ChibiOS - Copyright (C) 2026 Xael South
    Apache License 2.0 — see repository root.

    KEYSCAN test: 3 rows x 4 cols matrix. NOTE: PA1/PA2 are SWCLK/SWDIO —
    never drive those. Cols (COLOUT, driven) on PA0/PA3/PA4/PA6, rows
    (ROWSENSE, pulled-up inputs) on PA5/PB1/PA10. No keys are wired, so the
    scan must report NOKEY every cycle and SCANNED must fire repeatedly.
    Verifies: clock enable, CFG rows/cols, pin routing, scan start/stop,
    IRQ callback, STATUS.RUNNING, and clock gating on stop.
*/
#include "ch.h"
#include "hal.h"
#include "rtt_console.h"
#include "hal_keyscan_lld.h"

#define TEST_ROWS   3U
#define TEST_COLS   4U

/* Keep the coil pins PB0/PB3 and the SWD pins PA1/PA2 out of the scan. */
static const uint8_t col_pins[TEST_COLS] = { 0U, 3U, 4U, 6U };   /* port A */
static ioportid_t const row_ports[TEST_ROWS] = { GPIOA, GPIOB, GPIOA };
static const uint8_t row_pins[TEST_ROWS] = { 5U, 1U, 10U };

static volatile uint32_t cb_key, cb_nokey, cb_scanned;

static void keyscan_cb(uint32_t flags, uint8_t row, uint8_t col) {
  (void)row; (void)col;
  if (flags & EFR32_KEYSCAN_IF_KEY)     cb_key++;
  if (flags & EFR32_KEYSCAN_IF_NOKEY)   cb_nokey++;
  if (flags & EFR32_KEYSCAN_IF_SCANNED) cb_scanned++;
}

int main(void) {
  halInit();
  chSysInit();
  console_init();

  PRINTF("\r\nKEYSCAN test: %dx%d matrix, no keys wired\r\n",
         (int)TEST_ROWS, (int)TEST_COLS);

  bool ok = true;

  keyscan_lld_init();
  keyscan_lld_start();
  bool clk = ((CMU->CLKEN1 & CMU_CLKEN1_KEYSCAN) != 0U);
  bool en  = ((KEYSCAN->EN & KEYSCAN_EN_EN) != 0U);
  PRINTF("after start: CLKEN1.KEYSCAN=%d EN=%d %s\r\n", (int)clk, (int)en,
         (clk && en) ? "PASS" : "FAIL");
  ok = ok && clk && en;

  /* Config registers are writable only while EN=0. */
  keyscan_lld_disable();
  keyscan_lld_config(TEST_ROWS, TEST_COLS, 99U, false);
  keyscan_lld_set_delay(8U, 8U);
  uint32_t cfg = KEYSCAN->CFG;
  uint32_t rows_f = (cfg & _KEYSCAN_CFG_NUMROWS_MASK) >> _KEYSCAN_CFG_NUMROWS_SHIFT;
  uint32_t cols_f = (cfg & _KEYSCAN_CFG_NUMCOLS_MASK) >> _KEYSCAN_CFG_NUMCOLS_SHIFT;
  bool cfg_ok = (rows_f == TEST_ROWS - 1U) && (cols_f == TEST_COLS - 1U);
  PRINTF("CFG: NUMROWS field=%lu (expect %d), NUMCOLS field=%lu (expect %d) %s\r\n",
         (unsigned long)rows_f, (int)(TEST_ROWS - 1U),
         (unsigned long)cols_f, (int)(TEST_COLS - 1U), cfg_ok ? "PASS" : "FAIL");
  ok = ok && cfg_ok;

  /* Route columns (driven outputs) and rows (sensed inputs). */
  for (uint8_t c = 0; c < TEST_COLS; c++) {
    palSetPadMode(GPIOA, col_pins[c], EFR32_KEYSCAN_PAD_MODE_COL_OUT);
  }
  for (uint8_t r = 0; r < TEST_ROWS; r++) {
    palSetPadMode(row_ports[r], row_pins[r], PAL_MODE_INPUT_PULLUP);
  }
  for (uint8_t c = 0; c < TEST_COLS; c++)
    keyscan_lld_route_col(c, GPIOA, col_pins[c]);
  for (uint8_t r = 0; r < TEST_ROWS; r++)
    keyscan_lld_route_row(r, row_ports[r], row_pins[r]);
  /* KEYSCAN_E301: unused rows 3-5 must be tied to a used row, otherwise
     they float and read as phantom key presses. */
  for (uint8_t r = TEST_ROWS; r < EFR32_KEYSCAN_MAX_ROWS; r++)
    keyscan_lld_route_row(r, row_ports[TEST_ROWS - 1U], row_pins[TEST_ROWS - 1U]);
  bool route_ok = ((GPIO->KEYSCANROUTE.ROUTEEN & 0x0FU) == 0x0FU)
                  && ((GPIO->KEYSCANROUTE.COLOUT0ROUTE & 0xFFFF0003UL) ==
                      ((0UL << 0) | (col_pins[0] << 16)))
                  && ((GPIO->KEYSCANROUTE.ROWSENSE0ROUTE & 0xFFFF0003UL) ==
                      ((0UL << 0) | (row_pins[0] << 16)));
  PRINTF("route: ROUTEEN=0x%lX COLOUT0=0x%lX ROWSENSE0=0x%lX %s\r\n",
         (unsigned long)GPIO->KEYSCANROUTE.ROUTEEN,
         (unsigned long)GPIO->KEYSCANROUTE.COLOUT0ROUTE,
         (unsigned long)GPIO->KEYSCANROUTE.ROWSENSE0ROUTE,
         route_ok ? "PASS" : "FAIL");
  ok = ok && route_ok;

  keyscan_lld_enable();
  keyscan_lld_set_callback(keyscan_cb);
  keyscan_lld_irq_enable(EFR32_KEYSCAN_IF_KEY | EFR32_KEYSCAN_IF_NOKEY |
                         EFR32_KEYSCAN_IF_SCANNED);
  keyscan_lld_scan_start();

  chThdSleepMilliseconds(500);
  bool running = ((keyscan_lld_get_status() & KEYSCAN_STATUS_RUNNING) != 0U);
  PRINTF("scan running=%d %s\r\n", (int)running, running ? "PASS" : "FAIL");
  ok = ok && running;

  chThdSleepMilliseconds(3000);
  uint32_t scanned = cb_scanned, nokey = cb_nokey, key = cb_key;
  uint8_t row = 0xFF, col = 0xFF;
  bool got_key = keyscan_lld_get_key(&row, &col);
  uint32_t nokey_st = keyscan_lld_get_status() & KEYSCAN_STATUS_NOKEY;
  uint32_t st_raw = keyscan_lld_get_status();
  PRINTF("dbg: rows PA5=%d PB1=%d PA10=%d, STATUS=0x%08lX\r\n",
         (int)palReadPad(GPIOA, 5), (int)palReadPad(GPIOB, 1),
         (int)palReadPad(GPIOA, 10), (unsigned long)st_raw);
  PRINTF("after 3.5 s: SCANNED=%lu NOKEY=%lu KEY=%lu (cb), STATUS.NOKEY=%lu, get_key=%d row=%d col=%d\r\n",
         (unsigned long)scanned, (unsigned long)nokey, (unsigned long)key,
         (unsigned long)(nokey_st != 0U), (int)got_key, (int)row, (int)col);
  bool scan_ok = (scanned > 10U) && (key == 0U) && (nokey_st != 0U) && !got_key;
  PRINTF("scan results (scanned>10, no KEY irq, STATUS.NOKEY=1, get_key=0): %s\r\n",
         scan_ok ? "PASS" : "FAIL");
  ok = ok && scan_ok;

  keyscan_lld_scan_stop();
  keyscan_lld_stop();
  uint32_t clk2 = CMU->CLKEN1 & CMU_CLKEN1_KEYSCAN;
  PRINTF("after stop CLKEN1.KEYSCAN=0x%lX (expect 0) %s\r\n",
         (unsigned long)clk2, (clk2 == 0U) ? "PASS" : "FAIL");
  ok = ok && (clk2 == 0U);

  PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL");

  while (true) { chThdSleepMilliseconds(1000); }
}
