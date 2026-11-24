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
 * @file    ringbuf.c
 * @brief   Generic single-producer/single-consumer ring buffer.
 *
 * @addtogroup RINGBUF
 * @{
 */

#include "hal.h"   /* osal.h for osalSysLockFromISR */
#include "ringbuf.h"

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void ringbufObjectInit(ringbuf_t *rb, void *storage, size_t elem_size,
                       size_t capacity, bool overwrite) {
  rb->storage   = (uint8_t *)storage;
  rb->elem_size = elem_size;
  rb->capacity  = capacity;
  rb->overwrite = overwrite;
  ringbufReset(rb);
}

void ringbufReset(ringbuf_t *rb) {
  rb->head = 0U;
  rb->tail = 0U;
}

size_t ringbufCount(const ringbuf_t *rb) {
  size_t head = rb->head;
  size_t tail = rb->tail;
  /* One slot is kept empty to distinguish full from empty. */
  return (head + rb->capacity - tail) % rb->capacity;
}

bool ringbufEmpty(const ringbuf_t *rb) {
  return rb->head == rb->tail;
}

bool ringbufFull(const ringbuf_t *rb) {
  size_t next = (rb->head + 1U) % rb->capacity;
  return next == rb->tail;
}

void *ringbufSlotPtr(const ringbuf_t *rb, size_t index) {
  return rb->storage + (index % rb->capacity) * rb->elem_size;
}

size_t ringbufWrite(ringbuf_t *rb, const void *elem) {
  size_t head = rb->head;
  size_t next = (head + 1U) % rb->capacity;
  if (next == rb->tail) {
    if (!rb->overwrite) {
      return 0U;
    }
    /* Drop the oldest element. */
    rb->tail = (rb->tail + 1U) % rb->capacity;
  }
  uint8_t *slot = rb->storage + head * rb->elem_size;
  for (size_t i = 0U; i < rb->elem_size; i++) {
    slot[i] = ((const uint8_t *)elem)[i];
  }
  rb->head = next;
  return 1U;
}

size_t ringbufRead(ringbuf_t *rb, void *elem) {
  if (rb->head == rb->tail) {
    return 0U;
  }
  uint8_t *slot = rb->storage + rb->tail * rb->elem_size;
  for (size_t i = 0U; i < rb->elem_size; i++) {
    ((uint8_t *)elem)[i] = slot[i];
  }
  rb->tail = (rb->tail + 1U) % rb->capacity;
  return 1U;
}

size_t ringbufPeek(const ringbuf_t *rb, void *elem) {
  if (rb->head == rb->tail) {
    return 0U;
  }
  uint8_t *slot = rb->storage + rb->tail * rb->elem_size;
  for (size_t i = 0U; i < rb->elem_size; i++) {
    ((uint8_t *)elem)[i] = slot[i];
  }
  return 1U;
}

/*===========================================================================*/
/* ISR-context variants.                                                     */
/*===========================================================================*/

size_t ringbufWriteI(ringbuf_t *rb, const void *elem) {
  osalSysLockFromISR();
  size_t r = ringbufWrite(rb, elem);
  osalSysUnlockFromISR();
  return r;
}

size_t ringbufReadI(ringbuf_t *rb, void *elem) {
  osalSysLockFromISR();
  size_t r = ringbufRead(rb, elem);
  osalSysUnlockFromISR();
  return r;
}

void ringbufCommitI(ringbuf_t *rb) {
  osalSysLockFromISR();
  rb->head = (rb->head + 1U) % rb->capacity;
  osalSysUnlockFromISR();
}

/** @} */
