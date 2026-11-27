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
 * @brief   Generic ring buffer.
 *
 * @addtogroup RINGBUF
 * @{
 */

#include "hal.h"
#include "ringbuf.h"

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void ringbufObjectInit(ringbuf_t *rb, void *storage, size_t elem_size,
                       size_t capacity, bool overwrite) {
  osalDbgCheck(rb != NULL && storage != NULL && elem_size > 0U &&
               capacity >= 2U);
  rb->storage   = (uint8_t *)storage;
  rb->elem_size = elem_size;
  rb->capacity  = capacity;
  rb->overwrite = overwrite;
  rb->head = 0U;
  rb->tail = 0U;
}

void ringbufReset(ringbuf_t *rb) {
  osalSysLock();
  ringbufResetI(rb);
  osalSysUnlock();
}

size_t ringbufCount(const ringbuf_t *rb) {
  osalSysLock();
  size_t count = ringbufCountI(rb);
  osalSysUnlock();
  return count;
}

bool ringbufEmpty(const ringbuf_t *rb) {
  osalSysLock();
  bool empty = ringbufEmptyI(rb);
  osalSysUnlock();
  return empty;
}

bool ringbufFull(const ringbuf_t *rb) {
  osalSysLock();
  bool full = ringbufFullI(rb);
  osalSysUnlock();
  return full;
}

size_t ringbufWrite(ringbuf_t *rb, const void *elem) {
  osalSysLock();
  size_t written = ringbufWriteI(rb, elem);
  osalSysUnlock();
  return written;
}

size_t ringbufRead(ringbuf_t *rb, void *elem) {
  osalSysLock();
  size_t read = ringbufReadI(rb, elem);
  osalSysUnlock();
  return read;
}

size_t ringbufPeek(const ringbuf_t *rb, void *elem) {
  osalSysLock();
  size_t read = ringbufPeekI(rb, elem);
  osalSysUnlock();
  return read;
}

/*===========================================================================*/
/* I-class variants.                                                         */
/*===========================================================================*/

void ringbufResetI(ringbuf_t *rb) {
  osalDbgCheckClassI();
  rb->head = 0U;
  rb->tail = 0U;
}

size_t ringbufCountI(const ringbuf_t *rb) {
  osalDbgCheckClassI();
  /* One slot is kept empty to distinguish full from empty. */
  return (rb->head + rb->capacity - rb->tail) % rb->capacity;
}

bool ringbufEmptyI(const ringbuf_t *rb) {
  osalDbgCheckClassI();
  return rb->head == rb->tail;
}

bool ringbufFullI(const ringbuf_t *rb) {
  osalDbgCheckClassI();
  return (rb->head + 1U) % rb->capacity == rb->tail;
}

size_t ringbufWriteI(ringbuf_t *rb, const void *elem) {
  osalDbgCheckClassI();
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

size_t ringbufReadI(ringbuf_t *rb, void *elem) {
  osalDbgCheckClassI();
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

size_t ringbufPeekI(const ringbuf_t *rb, void *elem) {
  osalDbgCheckClassI();
  if (rb->head == rb->tail) {
    return 0U;
  }
  uint8_t *slot = rb->storage + rb->tail * rb->elem_size;
  for (size_t i = 0U; i < rb->elem_size; i++) {
    ((uint8_t *)elem)[i] = slot[i];
  }
  return 1U;
}

/** @} */
