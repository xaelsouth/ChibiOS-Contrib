/*
    ChibiOS - Copyright (C) 2026 Xael South.

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
 * @file    demos/IMX23/RT-IMX23-HELLO/net/sio_sio.c
 * @brief   lwIP sio_* serial shim over ChibiOS SIO drivers.
 *
 * @addtogroup NET_SERVICES
 * @{
 */

#include "hal.h"

#include "lwip/opt.h"
#include "lwip/sio.h"
#include "sio_sio.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

#define SIO_SIO_MAX_SLOTS       4U

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

typedef struct {
  u8_t            devnum;     /* lwIP sio device number                    */
  SIODriver       *siop;      /* ChibiOS SIO driver                        */
  volatile bool   abort;      /* sio_read_abort() pending flag             */
} sio_sio_slot_t;

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

static sio_sio_slot_t slots[SIO_SIO_MAX_SLOTS];

/*===========================================================================*/
/* Module exported functions (lwIP sio interface).                           */
/*===========================================================================*/

sio_fd_t sio_open(u8_t devnum) {
  unsigned i;

  for (i = 0U; i < SIO_SIO_MAX_SLOTS; i++) {
    if ((slots[i].siop != NULL) && (slots[i].devnum == devnum)) {
      slots[i].abort = false;
      return (sio_fd_t)&slots[i];
    }
  }
  return (sio_fd_t)NULL;
}

void sio_send(u8_t c, sio_fd_t fd) {
  sio_sio_slot_t *s = (sio_sio_slot_t *)fd;

  (void)sioAsyncWrite(s->siop, &c, 1U);
}

u8_t sio_recv(sio_fd_t fd) {
  u8_t c = 0U;

  (void)sio_read(fd, &c, 1U);
  return c;
}

u32_t sio_read(sio_fd_t fd, u8_t *data, u32_t len) {
  sio_sio_slot_t *s = (sio_sio_slot_t *)fd;
  u32_t got = 0U;

  while ((got < len) && !s->abort) {
    got += (u32_t)sioAsyncRead(s->siop, &data[got], len - got);
    if (got < len) {
      chThdSleepMilliseconds(1);
    }
  }
  s->abort = false;
  return got;
}

u32_t sio_tryread(sio_fd_t fd, u8_t *data, u32_t len) {
  sio_sio_slot_t *s = (sio_sio_slot_t *)fd;

  return (u32_t)sioAsyncRead(s->siop, data, len);
}

u32_t sio_write(sio_fd_t fd, u8_t *data, u32_t len) {
  sio_sio_slot_t *s = (sio_sio_slot_t *)fd;
  u32_t sent = 0U;

  while (sent < len) {
    sent += (u32_t)sioAsyncWrite(s->siop, &data[sent], len - sent);
  }
  return sent;
}

void sio_read_abort(sio_fd_t fd) {
  sio_sio_slot_t *s = (sio_sio_slot_t *)fd;

  s->abort = true;
}

/*===========================================================================*/
/* Module exported functions (registration).                                 */
/*===========================================================================*/

void sio_sio_register(u8_t devnum, SIODriver *siop) {
  unsigned i;
  int free_slot = -1;

  osalDbgCheck(siop != NULL);

  for (i = 0U; i < SIO_SIO_MAX_SLOTS; i++) {
    if (slots[i].siop == siop) {
      slots[i].devnum = devnum;   /* re-register: update number */
      return;
    }
    if ((slots[i].siop == NULL) && (free_slot < 0)) {
      free_slot = (int)i;
    }
  }
  if (free_slot >= 0) {
    slots[free_slot].devnum = devnum;
    slots[free_slot].siop   = siop;
    slots[free_slot].abort  = false;
  }
}

SIODriver *sio_sio_lookup(u8_t devnum) {
  unsigned i;

  for (i = 0U; i < SIO_SIO_MAX_SLOTS; i++) {
    if ((slots[i].siop != NULL) && (slots[i].devnum == devnum)) {
      return slots[i].siop;
    }
  }
  return NULL;
}

/** @} */
