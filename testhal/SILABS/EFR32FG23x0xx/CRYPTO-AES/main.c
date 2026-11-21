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

#include <string.h>
#include "ch.h"
#include "hal.h"
#include "chprintf.h"

/* VCOM console on EUSART2 (SIOD2), PA9=TX / PA10=RX, 115200 8N1. */
static const SIOConfig console_cfg = {
  .baud = 115200U,
  .cfg0 = (0U << 0) | (0U << 5),
  .framecfg = EFR32_SIO_LLD_EUSART_8N1,
};

static void console_init(void) {
  palSetPadMode(GPIOA, 9, PAL_MODE_OUTPUT_PUSHPULL | PAL_MODE_ALTERNATE(EUSART1_TX));
  palSetPadMode(GPIOA, 10, PAL_MODE_INPUT_PULLUP | PAL_MODE_ALTERNATE(EUSART1_RX));
  sioStart(&SIOD2, &console_cfg);
}
#define PRINTF(...) chprintf((BaseSequentialStream *)&SIOD2, __VA_ARGS__)

static int fails = 0;

static void hex2bin(const char *h, uint8_t *b, int n) {
  for (int i = 0; i < n; i++) {
    unsigned v; sscanf(h + 2 * i, "%2x", &v); b[i] = (uint8_t)v;
  }
}
static void check(const char *name, const uint8_t *got, const char *want, int n) {
  char gh[130];
  for (int i = 0; i < n; i++) sprintf(gh + 2 * i, "%02x", got[i]);
  gh[2 * n] = 0;
  bool ok = strcmp(gh, want) == 0;
  if (!ok) fails++;
  PRINTF("%-24s %s\r\n", name, ok ? "PASS" : "FAIL");
}

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nCRYPTO-AES test: RADIOAES NIST vectors\r\n");

  cryStart(&CRYD1, NULL);

  uint8_t key[32], in[64], out[64], iv[16];

  hex2bin("000102030405060708090a0b0c0d0e0f", key, 16);
  hex2bin("00112233445566778899aabbccddeeff", in, 16);
  cryLoadAESTransientKey(&CRYD1, 16, key);
  cryEncryptAES_ECB(&CRYD1, 0, 16, in, out);
  check("ECB-128 enc", out, "69c4e0d86a7b0430d8cdb78070b4c55a", 16);

  hex2bin("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", key, 32);
  cryLoadAESTransientKey(&CRYD1, 32, key);
  cryEncryptAES_ECB(&CRYD1, 0, 16, in, out);
  check("ECB-256 enc", out, "8ea2b7ca516745bfeafc49904b496089", 16);

  hex2bin("2b7e151628aed2a6abf7158809cf4f3c", key, 16);
  hex2bin("000102030405060708090a0b0c0d0e0f", iv, 16);
  hex2bin("6bc1bee22e409f96e93d7e117393172a"
          "ae2d8a571e03ac9c9eb76fac45af8e51"
          "30c81c46a35ce411e5fbc1191a0a52ef"
          "f69f2445df4f9b17ad2b417be66c3710", in, 64);
  cryLoadAESTransientKey(&CRYD1, 16, key);
  cryEncryptAES_CBC(&CRYD1, 0, 64, in, out, iv);
  check("CBC-128 enc", out,
        "7649abac8119b246cee98e9b12e9197d"
        "5086cb9b507219ee95db113a917678b2"
        "73bed6b8e3c1743b7116e69e22229516"
        "3ff1caa1681fac09120eca307586e1a7", 64);

  hex2bin("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff", iv, 16);
  cryEncryptAES_CTR(&CRYD1, 0, 64, in, out, iv);
  check("CTR-128 enc", out,
        "874d6191b620e3261bef6864990db6ce"
        "9806f66b7970fdff8617187bb9fffdff"
        "5ae4df3edbd5d35e5b4f09020db03eab"
        "1e031dda2fbe03d1792170a0f3009cee", 64);

  cryStop(&CRYD1);
  PRINTF("result: %s (%d failures)\r\n", fails ? "FAIL" : "ALL PASS", fails);
  while (true) chThdSleepMilliseconds(1000);
}
