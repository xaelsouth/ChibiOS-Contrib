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
 * @brief   Generic ring buffer.
 *
 * @details Typed ring buffer over caller-provided storage. Normal operations
 *          acquire the system lock; I-class operations require the caller to
 *          hold it. Initialize the object before it is shared and do not
 *          access its storage directly while it is in use. Data is copied
 *          while locked, so element size affects interrupt latency.
 *
 *          DMA writers must use separate storage and publish completed data
 *          through ringbufWriteI() under the system lock.
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
  size_t           capacity;   /**< Storage slots; one remains unused. */
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
  size_t ringbufPeek(const ringbuf_t *rb, void *elem);

  /* I-class variants: caller must hold the system lock. */
  void   ringbufResetI(ringbuf_t *rb);
  size_t ringbufWriteI(ringbuf_t *rb, const void *elem);
  size_t ringbufReadI(ringbuf_t *rb, void *elem);
  size_t ringbufCountI(const ringbuf_t *rb);
  bool   ringbufEmptyI(const ringbuf_t *rb);
  bool   ringbufFullI(const ringbuf_t *rb);
  size_t ringbufPeekI(const ringbuf_t *rb, void *elem);

#ifdef __cplusplus
}
#endif

#endif /* RINGBUF_H */

/** @} */
