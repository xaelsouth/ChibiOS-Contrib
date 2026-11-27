/*
    ChibiOS - Copyright (C) 2026 Xael South

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0
*/

#ifndef EFR32_STREAM_RX_DMA_H
#define EFR32_STREAM_RX_DMA_H

#define EFR32_STREAM_RX_DMA_SIZE 128U

typedef struct {
  const efr32_dma_stream_t *stream;
  const volatile uint32_t *reg;
  uint32_t source;
  uint8_t data[EFR32_STREAM_RX_DMA_SIZE];
  size_t rd;
  size_t wr;
  bool active;
} efr32_stream_rx_dma_t;

static inline void efr32_stream_rx_sync(efr32_stream_rx_dma_t *rx) {
  if (rx->active) {
    uintptr_t dst = rx->stream->stream->DST;
    rx->wr = (dst - (uintptr_t)rx->data) % EFR32_STREAM_RX_DMA_SIZE;
    __DMB();
  }
}

static inline void efr32_stream_rx_start(efr32_stream_rx_dma_t *rx,
                                         bool single) {
  size_t free, count;

  if (rx->active)
    return;
  free = (rx->rd + EFR32_STREAM_RX_DMA_SIZE - rx->wr - 1U) %
         EFR32_STREAM_RX_DMA_SIZE;
  if (free == 0U)
    return;
  count = EFR32_STREAM_RX_DMA_SIZE - rx->wr;
  if (count > free)
    count = free;
  if (single)
    count = 1U;
  dmaStreamSetRequestSource(rx->stream, rx->source, 0U);
  rx->active = true;
  dmaStartPeriphToMem(rx->stream, rx->reg, &rx->data[rx->wr], count,
                      EFR32_DMA_CH_CTRL_SIZE_BYTE);
}

static inline void efr32_stream_rx_stop(efr32_stream_rx_dma_t *rx) {
  if (rx->active) {
    dmaStreamDisable(rx->stream);
    while ((LDMA->CHBUSY & (1U << rx->stream->selfindex)) != 0U) {}
    efr32_stream_rx_sync(rx);
    rx->active = false;
    LDMA->CHDONE_CLR = 1U << rx->stream->selfindex;
  }
}

static inline void efr32_stream_rx_complete(efr32_stream_rx_dma_t *rx) {
  if (rx->active &&
      (LDMA->CHDONE & (1U << rx->stream->selfindex)) != 0U) {
    dmaStreamDisable(rx->stream);
    efr32_stream_rx_sync(rx);
    rx->active = false;
    LDMA->CHDONE_CLR = 1U << rx->stream->selfindex;
    efr32_stream_rx_start(rx, false);
  }
}

static inline bool efr32_stream_rx_empty(efr32_stream_rx_dma_t *rx) {
  efr32_stream_rx_sync(rx);
  return rx->rd == rx->wr;
}

static inline uint8_t efr32_stream_rx_get(efr32_stream_rx_dma_t *rx) {
  uint8_t value = rx->data[rx->rd];
  rx->rd = (rx->rd + 1U) % EFR32_STREAM_RX_DMA_SIZE;
  return value;
}

static inline void efr32_stream_rx_rearm(efr32_stream_rx_dma_t *rx,
                                         bool idle) {
  efr32_stream_rx_sync(rx);
  if (idle && rx->rd == rx->wr)
    efr32_stream_rx_stop(rx);
  efr32_stream_rx_sync(rx);
  efr32_stream_rx_start(rx, idle && rx->rd == rx->wr);
}

#endif /* EFR32_STREAM_RX_DMA_H */
