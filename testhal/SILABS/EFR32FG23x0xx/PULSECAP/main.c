/*
    ChibiOS - Copyright (C) 2026 Xael South
    Apache License 2.0 — see repository root.

    PULSECAP test: PRS-triggered pulse capture into a ring buffer via LDMA.

    Chain (all EM2-capable on FG23):
      PRS async ch4 (sw-pulse injection) --> PCNT0 S0IN     (count++, never reset)
                                          -> LETIMER0 START (FREE mode; PRS START
                                             while running is a no-op => the
                                             window is NOT restarted by pulses)
      BURTC COMP (window expired, 1 s) --\
                                        +--> PRS ch2 (FNSEL A|B) -> LDMAXBAR DMAREQ0
      PCNT0 UFOF (counter overflow) ----/                          |
                                                                    v
                                            LDMA ch (REQ=PRSREQ0) reads PCNT0->CNT
                                            into the next ring slot (circular
                                            linked-descriptor chain); DONE irq
                                            commits the slot and signals the app.

    The app computes deltas from the monotonic 16-bit CNT itself.

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
static efr32_dma_desc_t  desc[PCAP_RING_CAP] __attribute__((aligned(16)));
static thread_t         *main_tp;
static volatile uint32_t ldma_done_cnt;

/* LDMA per-descriptor DONE ISR: the engine already wrote CNT into the
   slot; commit it to the ring and wake the app. Runs under the lock held
   by the DMAv1 LLD ISR — do not lock again here. */
static const efr32_dma_stream_t *pcap_stream;

static void dma_isr(void *p, uint32_t flags) {
  (void)p; (void)flags;
  ring.head = (ring.head + 1U) % ring.capacity;   /* = ringbufCommitI() */
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

/* PCNT event (overflow/underflow) -> PRS ch2 pulse -> DMAREQ0 -> LDMA.
   Used instead of the PCNT0_UFOF PRS producer: with PCNT clocked by the S0
   PRS signal itself, the overflow event pulse is too short for the async
   PRS synchronizer, so the event is translated to a SWPULSE from the IRQ. */
static void pcnt_evt_cb(QEIDriver *qp) {
  (void)qp;
  sigrouteSwPulse(&SIGROUTED1, (1U << 2));
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

int main(void) {
  halInit();
  chSysInit();
  console_init();
  main_tp = chThdGetSelfX();

  PRINTF("\r\nPULSECAP test: PRS->PCNT, BURTC window, LDMA->ringbuf\r\n");

  bool ok = true;

  /* 1) Ring buffer + circular LDMA descriptor chain over its slots. */
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
    desc[i].DST  = (uint32_t)&ring_storage[i];
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
  /* ch1: PCNT0 overflow; ch2: BURTC COMP (window end), OR-combined with ch1. */
  sigrouteConnect(&SIGROUTED1, SIGROUTE_PATH_ASYNC, 1, SIGROUTE_GEN_PCNT0_UFOF);
  sigrouteConnect(&SIGROUTED1, SIGROUTE_PATH_ASYNC, 2, SIGROUTE_GEN_BURTC_COMP);
  sigrouteSetLogic(&SIGROUTED1, 2, SIGROUTE_FN_OR, 1);
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
  qeiStart(&QEID1, &qei_cfg);
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
  bool burtc_running = ((BURTC->CNT != 0U) || true); /* CNT may still read 0 */
  chThdSleepMilliseconds(50);
  uint32_t bc0 = BURTC->CNT;
  chThdSleepMilliseconds(50);
  uint32_t bc1 = BURTC->CNT;
  burtc_running = (bc1 > bc0);
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
        ring while the CPU is idle (kernel sleep / WFI in idle thread). */
  PRINTF("waiting for BURTC window (1 s)...\r\n");
  for (int i = 0; (i < 250) && (ringbufCount(&ring) == 0U); i++) {
    chThdSleepMilliseconds(10);   /* 2.5 s timeout; DONE irq commits slot */
  }
  uint32_t n1 = ringbufCount(&ring);
  uint32_t v1 = 0;
  bool got1 = (n1 > 0U) && (ringbufRead(&ring, &v1) != 0U);
  PRINTF("after window: ring entries=%lu first=%lu (expect %lu) ldma_done=%lu %s\r\n",
         (unsigned long)n1, (unsigned long)v1, (unsigned long)cnt7,
         (unsigned long)ldma_done_cnt,
         (got1 && v1 == cnt7 && ldma_done_cnt >= 1U) ? "PASS" : "FAIL");
  ok = ok && got1 && (v1 == cnt7) && (ldma_done_cnt >= 1U);

  /* 7) PCNT overflow path: restart PCNT with TOP=2, inject 3 pulses ->
        CNT 0,1,2,wrap->0, OF irq -> pcnt_evt_cb -> PRS ch2 -> DMAREQ0
        -> LDMA -> ring entry v2=0; app-side delta vs previous sample is
        (0 - cnt7) mod 65536. */
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
    .callback = pcnt_evt_cb,
  };
  qeiStart(&QEID1, &qei_cfg_of);
  chThdSleepMilliseconds(10);
  inject_pulses(&SIGROUTED1, 3U);
  for (int i = 0; (i < 100) && (ringbufCount(&ring) == 0U); i++) {
    chThdSleepMilliseconds(10);
  }
  uint32_t v2 = 0xFFFFFFFFU;
  bool got2 = (ringbufRead(&ring, &v2) != 0U);
  uint32_t delta = (v2 - cnt7) & 0xFFFFU;  /* monotonic 16-bit delta */
  uint32_t exp_delta = (0U - cnt7) & 0xFFFFU;
  bool of_ok = got2 && (v2 == 0U) && (delta == exp_delta);
  PRINTF("overflow: ring v2=%lu delta=%lu (expect v2=0 delta=%lu) %s\r\n",
         (unsigned long)v2, (unsigned long)delta, (unsigned long)exp_delta,
         of_ok ? "PASS" : "FAIL");
  ok = ok && of_ok;

  PRINTF("result: %s\r\n", ok ? "PASS" : "FAIL");

  while (true) { chThdSleepMilliseconds(1000); }
}
