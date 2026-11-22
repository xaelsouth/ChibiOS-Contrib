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

/* Console via SEGGER RTT (up channel 0) — works over J-Link SWD.
   View with: JLinkRTTLoggerExe -Device EFR32FG23B010F512IM48 -If SWD -Speed 4000 -RTTChannel 0 out.log */
#include <string.h>
#include <stdio.h>

typedef struct {
  char           id[16];        /* "SEGGER RTT" */
  uint32_t       num_up;
  uint32_t       num_down;
  struct {
    const char  *name;
    uint8_t     *buf;
    uint32_t     size;
    volatile uint32_t wr;
    volatile uint32_t rd;
    uint32_t     flags;
  } up[1];
} rtt_cb_t;

static rtt_cb_t rtt_cb __attribute__((aligned(32)));
static uint8_t  rtt_buf[1024];

static void console_init(void) {
  memset(&rtt_cb, 0, sizeof(rtt_cb));
  memcpy(rtt_cb.id, "SEGGER RTT", 10);
  rtt_cb.num_up = 1;
  rtt_cb.num_down = 0;
  rtt_cb.up[0].name  = "Terminal";
  rtt_cb.up[0].buf   = rtt_buf;
  rtt_cb.up[0].size  = sizeof(rtt_buf);
  rtt_cb.up[0].flags = 0;
}

static void rtt_write(const char *s, size_t n) {
  uint32_t wp = rtt_cb.up[0].wr;
  for (size_t i = 0; i < n; i++) {
    rtt_buf[wp] = (uint8_t)s[i];
    wp = (wp + 1U) % rtt_cb.up[0].size;
  }
  rtt_cb.up[0].wr = wp;
}

#define PRINTF(...) do {   char _b[256];   int _n = snprintf(_b, sizeof(_b), __VA_ARGS__);   if (_n > 0) rtt_write(_b, (size_t)_n); } while (0)

int main(void) {
  halInit();
  chSysInit();
  console_init();
  PRINTF("\r\nDMA test: LDMA mem-to-mem 256 bytes\r\n");

  static uint8_t src[256], dst[256];
  for (int i = 0; i < 256; i++) { src[i] = (uint8_t)(i ^ 0x5A); dst[i] = 0; }

  const efr32_dma_stream_t *st = dmaStreamAlloc(EFR32_DMA_STREAM_ID_ANY, 4, NULL, NULL);
  if (st == NULL) {
    PRINTF("FAIL: no DMA stream\r\n");
  } else {
    dmaStartMemCopy(st, 0, src, dst, 256);
    dmaWaitCompletion(st);
    dmaStreamFree(st);
    PRINTF("copy verified: %s\r\n", memcmp(src, dst, 256) == 0 ? "PASS" : "FAIL");
  }
  /* Peripheral DMA: EUSART0 (SIOD1) TX via LDMAXBAR request
     SOURCESEL=EUSART0, SIGSEL=TXFL, mem -> &EUSART0->TXDATA. */
  palSetPadMode(GPIOA, 9, PAL_MODE_OUTPUT_PUSHPULL | PAL_MODE_ALTERNATE(EUSART0_TX));
  palSetPadMode(GPIOA, 10, PAL_MODE_INPUT_PULLUP | PAL_MODE_ALTERNATE(EUSART0_RX));
  static const SIOConfig sio_cfg = {
    .baud = 115200U,
    .cfg0 = (0U << 0) | (0U << 5),
    .framecfg = EFR32_SIO_LLD_EUSART_8N1,
  };
  sioStart(&SIOD1, &sio_cfg);

  static const uint8_t txmsg[] = "DMA-TX on EUSART0 PA9\r\n";
  const efr32_dma_stream_t *st2 = dmaStreamAlloc(EFR32_DMA_STREAM_ID_ANY, 4, NULL, NULL);
  if (st2 == NULL) {
    PRINTF("FAIL: no DMA stream for periph\r\n");
  } else {
    dmaStreamSetRequestSource(st2, EFR32_DMA_REQ_EUSART0, EFR32_DMA_SIG_EUSART_TX);
    dmaStreamSetSource(st2, txmsg);
    dmaStreamSetDestination(st2, &EUSART0->TXDATA);
    dmaStreamSetMode(st2,
                     EFR32_DMA_CH_CTRL_DSTMODE_ABSOLUTE |
                     EFR32_DMA_CH_CTRL_SRCMODE_ABSOLUTE |
                     EFR32_DMA_CH_CTRL_SIZE_BYTE       |
                     EFR32_DMA_CH_CTRL_DSTINC_NONE     |
                     EFR32_DMA_CH_CTRL_SRCINC_ONE      |
                     EFR32_DMA_CH_CTRL_REQMODE_BLOCK   |
                     EFR32_DMA_CH_CTRL_BLOCKSIZE(1U)   |
                     EFR32_DMA_CH_CTRL_XFERCNT(sizeof(txmsg) - 1U) |
                     EFR32_DMA_CH_CTRL_DONEIEN);
    dmaStreamEnable(st2);
    /* bounded wait for DMA done flag */
    LDMA->IEN_CLR = (1U << st2->selfindex);
    volatile uint32_t spin = 20000000UL;
    while (((LDMA->IF & (1U << st2->selfindex)) == 0U) && (--spin != 0U)) {}
    bool dmadone = (spin != 0U);
    dmaStreamClearInterrupt(st2);
    /* wait until transmitter fully empty */
    spin = 20000000UL;
    while (((EUSART0->STATUS & EUSART_STATUS_TXC) == 0U) && (--spin != 0U)) {}
    bool txc = (spin != 0U);
    PRINTF("EUSART0 TX DMA %u bytes: dma-done=%s txc=%s -> %s\r\n",
           (unsigned)(sizeof(txmsg) - 1U), dmadone ? "yes" : "no",
           txc ? "yes" : "no", (dmadone && txc) ? "PASS" : "FAIL");
    dmaStreamFree(st2);
  }
  sioStop(&SIOD1);

  while (true) chThdSleepMilliseconds(1000);
}
