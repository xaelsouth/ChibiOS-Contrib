/*
    ChibiOS - Copyright (C) 2026 Xael South
    Apache License 2.0 — see repository root.

    PULSECAP test: PRS-triggered pulse capture into a ring buffer via LDMA.

    Chain (all EM2-capable on FG23):
      PRS async ch4 (sw-pulse injection) --> PCNT0 S0IN     (count++, never reset)
                                          -> LETIMER0 START (FREE mode; PRS START
                                             while running is a no-op => the
                                             window is NOT restarted by pulses)
      BURTC COMP (window expired, 1 s) --> PRS ch2 (FNSEL A) -> LDMAXBAR DMAREQ0
                                                                    |
                                                                    v
                                            LDMA ch (REQ=PRSREQ0) reads PCNT0->CNT
                                            into the next ring slot (circular
                                            linked-descriptor chain); DONE irq
                                            commits the slot and signals the app.

    PCNT overflow deliberately does NOT trigger DMA: the window sample
    right after a wrap already spans it via 16-bit delta math, and an
    extra overflow-triggered sample would double-count the pulses around
    the wrap. The app computes deltas from the monotonic 16-bit CNT.

    HW notes discovered on target (EFR32FG23B010F512IM48 / DK2600A):
      - PCNT in external-clock mode (CLKSEL=PCNTS0) syncs its LF register
        domain on S0 edges: SYNCBUSY must be pumped with PRS SWPULSE on the
        S0 channel and CMD STARTCNT must be issued, else CNT never moves and
        PCNT writes stall the bus (QEIv1 LLD does this in qei_lld_start).
      - LETIMER PRS start/stop/clear are gated by LETIMER0->PRSMODE
        (PRSSTARTMODE etc.), default NONE = disabled.
      - LETIMER compare events (CH0/CH1) do NOT pulse async PRS on xG23 in
        FREE or ONESHOT mode (IF sets, PRS stays silent) — therefore the
        window timer here is BURTC, whose COMP event does pulse PRS.
      - LDMA->CHEN reads back 0 on xG23; the real enable status is
        LDMA->CHSTATUS.
      - Do NOT write LDMA->LINKLOAD to start a linked list: it immediately
        loads the descriptor at LINK, skipping the first one. Program the
        channel registers directly; the chain continues via LINK after DONE.
      - BLOCKSIZE field is 2^n transfers per request: use BLOCKSIZE(0) for
        one word per request.
      - The DMAv1 LLD ISR already holds the kernel lock around the stream
        callback: callbacks must NOT call osalSysLockFromISR() again.
*/
#include "ch.h"
#include "hal.h"
#include "rtt_console.h"
#include "hal_sigroute.h"
#include "hal_qei.h"
#include "efr32_dma.h"
#include "ringbuf.h"

#define PCAP_RING_CAP     8U
#define PCAP_PERIOD_TICKS 32768U    /* 1 s window at 32768 Hz (app uses 3 s = 98304) */
#define PCAP_PULSE_CH     4U        /* async ch used for sw-pulse injection */
#define PCAP_EVT          0x1U

static uint32_t          ring_storage[PCAP_RING_CAP];
static ringbuf_t         ring;
static volatile uint32_t dma_sample;
static efr32_dma_desc_t  desc[PCAP_RING_CAP] __attribute__((aligned(16)));
static thread_t         *main_tp;
static volatile uint32_t ldma_done_cnt;
static volatile uint32_t ldma_error_cnt;
static volatile uint32_t ring_dropped_cnt;

/* The LDMA callback runs under the system lock held by the DMAv1 ISR. */
static const efr32_dma_stream_t *pcap_stream;

static void dma_isr(void *p, uint32_t flags) {
  (void)p;
  if ((flags & EFR32_DMA_ISR_ERROR_MASK) != 0U) {
    ldma_error_cnt++;
    return;
  }
  uint32_t sample = dma_sample;
  if (ringbufWriteI(&ring, &sample) == 0U) {
    ring_dropped_cnt++;
  }
  ldma_done_cnt++;
  /* xG23: the channel auto-disables after each descriptor DONE; re-enable
     so the circular linked list keeps accepting PRS requests. */
  if (pcap_stream != NULL) {
    LDMA->CHEN_SET = (1U << pcap_stream->selfindex);
  }
  if (main_tp != NULL) {
    chEvtSignalI(main_tp, PCAP_EVT);
  }
}

/* PCNT event counter only (overflow must NOT trigger DMA — see header). */
static volatile uint32_t pcnt_irq_cnt;
static void pcnt_irq_cb(QEIDriver *qp) {
  (void)qp;
  pcnt_irq_cnt++;
}

static void inject_pulses(SIGROUTEDriver *srp, uint32_t n) {
  for (uint32_t i = 0; i < n; i++) {
    sigrouteSwPulse(srp, (1U << PCAP_PULSE_CH));
    chThdSleepMilliseconds(5);
  }
}

static void burtc_sync_wait(void) {
  volatile uint32_t sp = 4000000UL;
  while (((BURTC->SYNCBUSY & _BURTC_SYNCBUSY_MASK) != 0U) && (--sp != 0U)) {}
}

static bool test_ringbuf(void) {
  ringbuf_t rb;
  uint32_t storage[4], value;
  uint32_t one = 1U, two = 2U, three = 3U, four = 4U;
  bool ok = true;

  ringbufObjectInit(&rb, storage, sizeof(storage[0]), 4U, false);
  ok = ringbufWrite(&rb, &one) == 1U && ok;
  ok = ringbufWrite(&rb, &two) == 1U && ok;
  ok = ringbufWrite(&rb, &three) == 1U && ok;
  ok = ringbufFull(&rb) && ringbufCount(&rb) == 3U && ok;
  ok = ringbufWrite(&rb, &four) == 0U && ok;
  ok = ringbufPeek(&rb, &value) == 1U && value == one && ok;
  ok = ringbufRead(&rb, &value) == 1U && value == one && ok;
  ok = ringbufWrite(&rb, &four) == 1U && ok;
  ok = ringbufRead(&rb, &value) == 1U && value == two && ok;
  ok = ringbufRead(&rb, &value) == 1U && value == three && ok;
  ok = ringbufRead(&rb, &value) == 1U && value == four && ok;
  ok = ringbufEmpty(&rb) && ringbufRead(&rb, &value) == 0U && ok;

  osalSysLock();
  ok = ringbufWriteI(&rb, &one) == 1U && ok;
  ok = ringbufPeekI(&rb, &value) == 1U && value == one && ok;
  ok = ringbufCountI(&rb) == 1U && !ringbufFullI(&rb) && ok;
  ok = ringbufReadI(&rb, &value) == 1U && value == one && ok;
  ringbufResetI(&rb);
  ok = ringbufEmptyI(&rb) && ok;
  osalSysUnlock();

  ringbufObjectInit(&rb, storage, sizeof(storage[0]), 4U, true);
  ok = ringbufWrite(&rb, &one) == 1U && ok;
  ok = ringbufWrite(&rb, &two) == 1U && ok;
  ok = ringbufWrite(&rb, &three) == 1U && ok;
  ok = ringbufWrite(&rb, &four) == 1U && ok;
  ok = ringbufRead(&rb, &value) == 1U && value == two && ok;
  ringbufReset(&rb);
  ok = ringbufEmpty(&rb) && ok;
  return ok;
}

int main(void) {
  halInit();
  chSysInit();
  console_init();
  main_tp = chThdGetSelfX();

  PRINTF("\r\nPULSECAP test: PRS->PCNT, BURTC window, LDMA->ringbuf\r\n");

  bool ok = test_ringbuf();
  PRINTF("ring buffer: %s\r\n", ok ? "PASS" : "FAIL");

  /* 1) LDMA samples into staging storage, then the ISR publishes to the ring. */
  ringbufObjectInit(&ring, ring_storage, sizeof(uint32_t), PCAP_RING_CAP, false);
  for (uint32_t i = 0; i < PCAP_RING_CAP; i++) {
    desc[i].CTRL = EFR32_DMA_CH_CTRL_SIZE_WORD        |
                   EFR32_DMA_CH_CTRL_DSTINC_NONE      |
                   EFR32_DMA_CH_CTRL_SRCINC_NONE      |
                   EFR32_DMA_CH_CTRL_REQMODE_BLOCK    |
                   EFR32_DMA_CH_CTRL_BLOCKSIZE(0U)    |  /* 2^0 = 1 word per request */
                   EFR32_DMA_CH_CTRL_XFERCNT(1U)      |  /* 1 word per descriptor */
                   EFR32_DMA_CH_CTRL_DONEIEN;
    desc[i].SRC  = (uint32_t)&PCNT0->CNT;
    desc[i].DST  = (uint32_t)&dma_sample;
    desc[i].LINK = EFR32_DMA_CH_LINK_LINKADDR(&desc[(i + 1U) % PCAP_RING_CAP]) |
                   EFR32_DMA_CH_LINK_LINKMODE_ABSOLUTE;
  }
  const efr32_dma_stream_t *st = dmaStreamAlloc(EFR32_DMA_STREAM_ID_ANY, 2,
                                                dma_isr, NULL);
  osalDbgCheck(st != NULL);
  pcap_stream = st;
  /* LDMA channel request = PRS request 0 (LDMAXBAR DMAREQ0 via PRS). */
  dmaStreamSetRequestSource(st, EFR32_DMA_REQ_SW,
                            _LDMAXBAR_CH_REQSEL_SIGSEL_LDMAXBARPRSREQ0);
  dmaStartLinkedList(st, &desc[0]);
  chThdSleepMilliseconds(5);
  /* xG23: CHEN reads back 0; CHSTATUS holds the real enable state. */
  bool dma_armed = ((LDMA->CHSTATUS & (1U << st->selfindex)) != 0U);
  PRINTF("LDMA armed (CHSTATUS bit%d)=%d %s\r\n", (int)st->selfindex,
         (int)dma_armed, dma_armed ? "PASS" : "FAIL");
  ok = ok && dma_armed;

  /* 2) SIGROUTE routes. */
  sigrouteInit();
  sigrouteObjectInit(&SIGROUTED1);
  sigrouteStart(&SIGROUTED1, NULL);
  /* ch4: sw-pulse injection -> PCNT0 S0IN + LETIMER0 START.
     Source = GPIO PA0 held low; sigrouteSwPulse() XORs a pulse on top. */
  palSetPadMode(GPIOA, 0, PAL_MODE_OUTPUT_PUSHPULL);
  palClearPad(GPIOA, 0);
  sigrouteConnect(&SIGROUTED1, SIGROUTE_PATH_ASYNC, PCAP_PULSE_CH, SIGROUTE_GEN_GPIO_PIN0);
  sigrouteConnectConsumer(&SIGROUTED1, SIGROUTE_CON_PCNT0_S0IN, SIGROUTE_PATH_ASYNC, PCAP_PULSE_CH);
  sigrouteConnectConsumer(&SIGROUTED1, SIGROUTE_CON_LETIMER0_START, SIGROUTE_PATH_ASYNC, PCAP_PULSE_CH);
  /* ch2: BURTC COMP (window end) -> LDMAXBAR DMAREQ0. PCNT overflow is
     intentionally NOT routed (see header comment). */
  sigrouteConnect(&SIGROUTED1, SIGROUTE_PATH_ASYNC, 2, SIGROUTE_GEN_BURTC_COMP);
  sigrouteConnectConsumer(&SIGROUTED1, SIGROUTE_CON_LDMAXBAR_DMAREQ0, SIGROUTE_PATH_ASYNC, 2);
  uint32_t c_pcnt = PRS->CONSUMER_PCNT0_S0IN;
  uint32_t c_let  = PRS->CONSUMER_LETIMER0_START;
  uint32_t c_dma  = PRS->CONSUMER_LDMAXBAR_DMAREQ0;
  bool routes_ok = (c_pcnt == PCAP_PULSE_CH) && (c_let == PCAP_PULSE_CH) &&
                   (c_dma == 2U);
  PRINTF("consumers: PCNT0_S0IN=ch%lu LETIMER0_START=ch%lu DMAREQ0=ch%lu %s\r\n",
         (unsigned long)c_pcnt, (unsigned long)c_let, (unsigned long)c_dma,
         routes_ok ? "PASS" : "FAIL");
  ok = ok && routes_ok;

  /* 3) PCNT0 via QEI LLD: external single clock on S0IN, count up, TOP=0xFFFF.
        The LLD pumps SYNCBUSY with S0 pulses and issues STARTCNT. */
  static const QEIConfig qei_cfg = {
    .cfg  = PCNT_CFG_MODE_EXTCLKSINGLE,   /* count on S0IN edges */
    .ctrl = 0U,                            /* EDGE=POS, no filter */
    .top  = 0xFFFFU,
    .s0prs = PCAP_PULSE_CH,
    .s1prs = 0xFFU,
    .min  = QEI_COUNT_MIN,
    .max  = QEI_COUNT_MAX,
    .overflow = QEI_OVERFLOW_WRAP,
    .overflow_cb = NULL,
    .callback = NULL,
  };
  sigrouteSwLevelSet(&SIGROUTED1, 1U << 3);
  qeiStart(&QEID1, &qei_cfg);
  bool other_level = (PRS->ASYNC_SWLEVEL & (1U << 3)) != 0U;
  PRINTF("unrelated PRS software level preserved: %s\r\n",
         other_level ? "PASS" : "FAIL");
  ok = ok && other_level;
  /* baseline: the LLD's SYNCBUSY pump already fired some counting pulses */
  chThdSleepMilliseconds(10);
  uint32_t cnt_base = (uint32_t)qeiGetCount(&QEID1) & 0xFFFFU;
  bool pcnt_en = ((PCNT0->EN & PCNT_EN_EN) != 0U);
  PRINTF("PCNT0 enabled=%d CLKSEL=0x%lX %s\r\n", (int)pcnt_en,
         (unsigned long)CMU->PCNT0CLKCTRL, pcnt_en ? "PASS" : "FAIL");
  ok = ok && pcnt_en;

  /* 4a) LETIMER0: PRS-start demo (FREE mode). START while running is a
        no-op, so pulses cannot restart it. Its CH0 does not pulse PRS on
        xG23, so it is not part of the LDMA trigger chain. */
  CMU->CLKEN0_SET = CMU_CLKEN0_LETIMER0;
  LETIMER0->EN_CLR = LETIMER_EN_EN;      /* configure while disabled: */
  { volatile uint32_t sp = 4000000UL;   /* TOP/COMP0/CTRL are buffered and  */
    while (((LETIMER0->SYNCBUSY & 0xFFFFFFFFUL) != 0U) && (--sp != 0U)) {} }  /* only safe then */
  LETIMER0->IEN = 0U;
  LETIMER0->IF_CLR = _LETIMER_IF_MASK;
  LETIMER0->CTRL = LETIMER_CTRL_REPMODE_FREE;
  LETIMER0->COMP0 = 0xFFFFFFU - PCAP_PERIOD_TICKS;
  LETIMER0->TOP   = 0xFFFFFFU;
  LETIMER0->TOPBUFF = 0xFFFFFFU;
  LETIMER0->PRSMODE = LETIMER_PRSMODE_PRSSTARTMODE_RISING;  /* enable PRS START */
  { volatile uint32_t sp = 4000000UL;
    while (((LETIMER0->SYNCBUSY & 0xFFFFFFFFUL) != 0U) && (--sp != 0U)) {} }
  LETIMER0->EN_SET = LETIMER_EN_EN;     /* enable last so TOP is loaded */
  { volatile uint32_t sp = 4000000UL;
    while (((LETIMER0->SYNCBUSY & 0xFFFFFFFFUL) != 0U) && (--sp != 0U)) {} }
  bool let_stopped = ((LETIMER0->STATUS & LETIMER_STATUS_RUNNING) == 0U);
  PRINTF("LETIMER0 armed (PRS-start armed), running=%d %s\r\n",
         (int)!let_stopped, let_stopped ? "PASS" : "FAIL");
  ok = ok && let_stopped;

  /* 4b) BURTC window timer: free-running from 0 on LFXO (32768 Hz),
        COMP = 1 s. COMP event -> PRS ch2 -> DMAREQ0 -> LDMA.
        BURTC runs in EM2/EM3. */
  CMU->CLKEN0_SET = CMU_CLKEN0_BURTC;
  BURTC->CMD_CLR = BURTC_CMD_STOP;
  burtc_sync_wait();
  BURTC->EN_CLR = BURTC_EN_EN;
  while ((BURTC->EN & BURTC_EN_DISABLING) == BURTC_EN_DISABLING) {}
  BURTC->CFG = 0U;                      /* prescaler DIV1 */
  BURTC->CNT_CLR = _BURTC_CNT_CNT_MASK; /* CNT = 0 */
  burtc_sync_wait();
  BURTC->COMP = PCAP_PERIOD_TICKS;      /* window = 1 s */
  BURTC->IF_CLR = _BURTC_IF_MASK;
  BURTC->IEN_CLR = _BURTC_IEN_MASK;     /* PRS only, no NVIC needed */
  BURTC->EN_SET = BURTC_EN_EN;
  burtc_sync_wait();
  BURTC->CMD_SET = BURTC_CMD_START;
  burtc_sync_wait();
  chThdSleepMilliseconds(50);
  uint32_t bc0 = BURTC->CNT;
  chThdSleepMilliseconds(50);
  uint32_t bc1 = BURTC->CNT;
  bool burtc_running = (bc1 > bc0);
  PRINTF("BURTC running (CNT %lu->%lu) COMP=%lu %s\r\n",
         (unsigned long)bc0, (unsigned long)bc1, (unsigned long)BURTC->COMP,
         burtc_running ? "PASS" : "FAIL");
  ok = ok && burtc_running;

  /* 5) Inject 7 pulses. Expect: CNT delta == 7, LETIMER started on the
        first pulse only (CNT keeps decreasing across later pulses). */
  inject_pulses(&SIGROUTED1, 6U);
  chThdSleepMilliseconds(20);
  bool running = ((LETIMER0->STATUS & LETIMER_STATUS_RUNNING) != 0U);
  uint32_t let_a = LETIMER0->CNT;
  inject_pulses(&SIGROUTED1, 1U);
  chThdSleepMilliseconds(20);
  uint32_t let_b = LETIMER0->CNT;
  uint32_t cnt7 = (uint32_t)qeiGetCount(&QEID1) & 0xFFFFU;
  bool no_restart = running && (let_b < let_a);
  uint32_t pulses_seen = (cnt7 - cnt_base) & 0xFFFFU;
  PRINTF("pulses: CNT=%lu base=%lu seen=%lu (expect 7) LETIMER running=%d CNT %lu->%lu (no restart) %s\r\n",
         (unsigned long)cnt7, (unsigned long)cnt_base, (unsigned long)pulses_seen,
         (int)running, (unsigned long)let_a, (unsigned long)let_b,
         (pulses_seen == 7U && no_restart) ? "PASS" : "FAIL");
  ok = ok && (pulses_seen == 7U) && no_restart;

  /* 6) Sleep until the BURTC window expires; LDMA must sample CNT into the
        staging storage while the CPU is idle; the ISR publishes the sample. */
  PRINTF("waiting for BURTC window (1 s)...\r\n");
  for (int i = 0; (i < 250) && (ringbufCount(&ring) == 0U); i++) {
    chThdSleepMilliseconds(10);   /* 2.5 s timeout */
  }
  uint32_t n1 = ringbufCount(&ring);
  uint32_t v1 = 0;
  bool got1 = (n1 > 0U) && (ringbufRead(&ring, &v1) != 0U);
  PRINTF("after window: ring entries=%lu first=%lu (expect %lu) ldma_done=%lu %s\r\n",
         (unsigned long)n1, (unsigned long)v1, (unsigned long)cnt7,
         (unsigned long)ldma_done_cnt,
         (got1 && v1 == cnt7 && ldma_done_cnt >= 1U) ? "PASS" : "FAIL");
  ok = ok && got1 && (v1 == cnt7) && (ldma_done_cnt >= 1U);

  /* 7) PCNT overflow must NOT trigger DMA (else pulses around a wrap are
        double-counted together with the next window sample). Restart PCNT
        with TOP=2, inject 3 pulses -> wrap (OF irq fires, CNT wraps), but
        no ring entry may appear. Then re-arm a short BURTC window and check
        that the next window sample matches the reconfigured counter. */
  qeiStop(&QEID1);
  static const QEIConfig qei_cfg_of = {
    .cfg  = PCNT_CFG_MODE_EXTCLKSINGLE,
    .ctrl = 0U,
    .top  = 2U,
    .s0prs = PCAP_PULSE_CH,
    .s1prs = 0xFFU,
    .min  = QEI_COUNT_MIN,
    .max  = QEI_COUNT_MAX,
    .overflow = QEI_OVERFLOW_WRAP,
    .overflow_cb = NULL,
    .callback = pcnt_irq_cb,
  };
  qeiStart(&QEID1, &qei_cfg_of);
  other_level = (PRS->ASYNC_SWLEVEL & (1U << 3)) != 0U;
  PRINTF("PRS software level preserved after restart: %s\r\n",
         other_level ? "PASS" : "FAIL");
  ok = ok && other_level;
  chThdSleepMilliseconds(10);
  uint32_t c0 = (uint32_t)qeiGetCount(&QEID1) & 0xFFFFU;
  uint32_t irq0 = pcnt_irq_cnt;
  uint32_t ringn0 = ringbufCount(&ring);
  uint32_t done0 = ldma_done_cnt;
  inject_pulses(&SIGROUTED1, 3U);
  chThdSleepMilliseconds(300);
  uint32_t c1 = (uint32_t)qeiGetCount(&QEID1) & 0xFFFFU;
  bool wrapped = (c1 == ((c0 + 3U) % 3U)) && ((c0 + 3U) > 2U);  /* TOP+1 = 3 */
  bool of_irq  = (pcnt_irq_cnt > irq0);
  bool no_dma  = (ringbufCount(&ring) == ringn0) && (ldma_done_cnt == done0);
  PRINTF("overflow: CNT %lu->%lu (wrapped=%d) of_irq=%d dma_untouched=%d %s\r\n",
         (unsigned long)c0, (unsigned long)c1, (int)wrapped, (int)of_irq, (int)no_dma,
         (wrapped && of_irq && no_dma) ? "PASS" : "FAIL");
  ok = ok && wrapped && of_irq && no_dma;

  /* re-arm a short BURTC window (~100 ms) and check the next sample */
  BURTC->IF_CLR = _BURTC_IF_MASK;
  BURTC->COMP = BURTC->CNT + 3277U;    /* ~100 ms at 32768 Hz */
  burtc_sync_wait();
  for (int i = 0; (i < 150) && (ringbufCount(&ring) == 0U); i++) {
    chThdSleepMilliseconds(10);
  }
  uint32_t v2 = 0xFFFFFFFFU;
  bool got2 = (ringbufRead(&ring, &v2) != 0U);
  uint32_t c2 = (uint32_t)qeiGetCount(&QEID1) & 0xFFFFU;  /* CNT at/after sample */
  /* PCNT CNT is LF-domain: an LDMA read right at a wrap can lag the CPU
     readback by one sync tick, so accept v2 == c2 or c2-1 (mod 2^16). */
  bool near = (v2 == c2) || (v2 == ((c2 - 1U) & 0xFFFFU));
  bool of_ok = got2 && near;
  PRINTF("wrap sample: ring v2=%lu (CNT now %lu) %s\r\n",
         (unsigned long)v2, (unsigned long)c2,
         of_ok ? "PASS" : "FAIL");
  ok = ok && of_ok;

  if (ldma_error_cnt != 0U || ring_dropped_cnt != 0U) {
    PRINTF("LDMA errors=%lu ring drops=%lu\r\n",
           (unsigned long)ldma_error_cnt, (unsigned long)ring_dropped_cnt);
    ok = false;
  }
  PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL");

  while (true) { chThdSleepMilliseconds(1000); }
}
