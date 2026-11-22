/*
    ChibiOS - Copyright (C) 2026 Xael South
    Apache License 2.0 — see repository root.

    CRC test: GPCRC hardware CRC-32 (IEEE 802.3) of "123456789"
    against the standard check value 0xCBF43926.

    GPCRC POLYSEL_CRC32 implements the reflected polynomial 0xEDB88320
    (LSB-first). The DATA register holds the raw register; the standard
    CRC-32 (with xorout 0xFFFFFFFF) is ~DATA.
*/
#include "ch.h"
#include "hal.h"
#include "rtt_console.h"
#include "hal_crc_lld.h"

static const char *msg = "123456789";   /* 9 bytes, no NUL */

static uint32_t crc32_hw_byte_fed(void) {
  static const CRCLLDConfig cfg = {
    .poly_sel = EFR32_CRC_POLY_CRC32,
    .poly     = 0,
    .init     = 0xFFFFFFFFUL,
    .bitrev   = false,
    .byterev  = false,
  };
  crc_lld_start(&cfg);
  for (int i = 0; i < 9; i++) {
    crc_lld_write8((uint8_t)msg[i]);
  }
  uint32_t raw = crc_lld_read();
  crc_lld_stop();
  return raw;
}

static const uint8_t msg12[12] = { '1','2','3','4','5','6','7','8','9',0,0,0 };

static uint32_t crc32_hw_word_fed(void) {
  static const CRCLLDConfig cfg = {
    .poly_sel = EFR32_CRC_POLY_CRC32,
    .poly     = 0,
    .init     = 0xFFFFFFFFUL,
    .bitrev   = false,
    .byterev  = false,  /* LE-packed words feed the byte stream in order */
  };
  crc_lld_start(&cfg);
  for (int i = 0; i < 12; i += 4) {
    crc_lld_write32((uint32_t)msg12[i]
                  | ((uint32_t)msg12[i+1] << 8)
                  | ((uint32_t)msg12[i+2] << 16)
                  | ((uint32_t)msg12[i+3] << 24));
  }
  uint32_t raw = crc_lld_read();
  crc_lld_stop();
  return raw;
}

static uint32_t crc32_hw_byte_fed12(void) {
  static const CRCLLDConfig cfg = {
    .poly_sel = EFR32_CRC_POLY_CRC32,
    .poly     = 0,
    .init     = 0xFFFFFFFFUL,
    .bitrev   = false,
    .byterev  = false,
  };
  crc_lld_start(&cfg);
  for (int i = 0; i < 12; i++) {
    crc_lld_write8(msg12[i]);
  }
  uint32_t raw = crc_lld_read();
  crc_lld_stop();
  return raw;
}

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nCRC test: GPCRC hardware CRC-32 of \"123456789\"\r\n");

  crc_lld_init();

  uint32_t raw1 = crc32_hw_byte_fed();
  uint32_t c32a = ~raw1;
  PRINTF("byte-fed : raw=0x%08lX crc=0x%08lX (expect 0xCBF43926) %s\r\n",
         (unsigned long)raw1, (unsigned long)c32a,
         (c32a == 0xCBF43926UL) ? "PASS" : "FAIL");

  uint32_t raw2 = crc32_hw_word_fed();
  uint32_t raw2b = crc32_hw_byte_fed12();
  uint32_t c32b = ~raw2;
  PRINTF("word-fed : raw=0x%08lX crc=0x%08lX (byte-fed 12B raw=0x%08lX) %s\r\n",
         (unsigned long)raw2, (unsigned long)c32b, (unsigned long)raw2b,
         (raw2 == raw2b) ? "PASS" : "FAIL");

  /* one-shot helper */
  uint32_t raw3 = crc_lld_compute32(msg, 9, EFR32_CRC_POLY_CRC32,
                                    EFR32_CRC32_POLY, EFR32_CRC32_INIT);
  uint32_t c32c = ~raw3;
  PRINTF("one-shot : raw=0x%08lX crc=0x%08lX (expect 0xCBF43926) %s\r\n",
         (unsigned long)raw3, (unsigned long)c32c,
         (c32c == 0xCBF43926UL) ? "PASS" : "FAIL");

  /* CRC-16 mode smoke test (16-bit path, deterministic result) */
  static const CRCLLDConfig cfg16 = {
    .poly_sel = EFR32_CRC_POLY_CRC16,
    .poly     = 0x8408UL,   /* reflected 0x1021 */
    .init     = 0xFFFFUL,
    .bitrev   = false,
    .byterev  = false,
  };
  crc_lld_start(&cfg16);
  for (int i = 0; i < 9; i++) {
    crc_lld_write8((uint8_t)msg[i]);
  }
  uint32_t c16 = crc_lld_read() & 0xFFFFUL;
  crc_lld_stop();
  PRINTF("crc16 mode (poly 8408 init FFFF): 0x%04lX (expect 0x6F91 = X-25 pre-xorout) %s\r\n",
         (unsigned long)c16, (c16 == 0x6F91UL) ? "PASS" : "FAIL");

  bool ok = (c32a == 0xCBF43926UL) && (raw2 == raw2b) &&
            (c32c == 0xCBF43926UL) && (c16 == 0x6F91UL);
  PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL");

  while (true) { chThdSleepMilliseconds(1000); }
}
