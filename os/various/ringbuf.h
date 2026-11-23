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

/**
 * @file    ringbuf.h
 * @brief   Generic single-producer/single-consumer ring buffer.
 *
 * @details Typed ring buffer over caller-provided storage. Designed for
 *          ISR/thread or DMA/thread communication:
 *          - ringbufWrite()/ringbufWriteI() are called by the single
 *            producer (thread or ISR, e.g. an LDMA completion handler);
 *          - ringbufRead() is called by the single consumer (thread).
 *          head is written only by the producer, tail only by the
 *          consumer, so no locking is needed for SPSC operation.
 *
 *          A DMA engine can append directly into the storage: build a
 *          descriptor chain over ringbufSlotPtr() slots and let a
 *          completion IRQ advance the producer side (see ringbufCommitI()).
 *
 *          If overwrite is enabled, writing to a full buffer drops the
 *          oldest element; otherwise the write fails (returns 0).
 *
 * @addtogroup RINGBUF
 * @{
 */

#ifndef RINGBUF_H
#define RINGBUF_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Ring buffer structure.
 */
typedef struct {
  uint8_t         *storage;    /**< Element storage (capacity * elem_size bytes). */
  size_t           elem_size;  /**< Size of one element in bytes. */
  size_t           capacity;   /**< Number of elements. */
  volatile size_t  head;       /**< Producer: index of next slot to write. */
  volatile size_t  tail;       /**< Consumer: index of next slot to read. */
  bool             overwrite;  /**< true: overwrite oldest when full. */
} ringbuf_t;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

  void   ringbufObjectInit(ringbuf_t *rb, void *storage, size_t elem_size,
                           size_t capacity, bool overwrite);
  void   ringbufReset(ringbuf_t *rb);
  size_t ringbufWrite(ringbuf_t *rb, const void *elem);
  size_t ringbufRead(ringbuf_t *rb, void *elem);
  size_t ringbufCount(const ringbuf_t *rb);
  bool   ringbufEmpty(const ringbuf_t *rb);
  bool   ringbufFull(const ringbuf_t *rb);
  void  *ringbufSlotPtr(const ringbuf_t *rb, size_t index);
  size_t ringbufPeek(const ringbuf_t *rb, void *elem);

  /* ISR-context variants (use interrupt save/restore internally). */
  size_t ringbufWriteI(ringbuf_t *rb, const void *elem);
  size_t ringbufReadI(ringbuf_t *rb, void *elem);

  /* Producer-side commit for DMA writers: after the DMA engine wrote
     directly into slot (head % capacity), call ringbufCommitI() from the
     DMA completion ISR to publish the element. */
  void   ringbufCommitI(ringbuf_t *rb);

#ifdef __cplusplus
}
#endif

#endif /* RINGBUF_H */

/** @} */
